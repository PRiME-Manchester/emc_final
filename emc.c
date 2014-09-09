#include <string.h>
#include "spin1_api.h"
#include "lzss.h"
#include "router.h"

#define NODEBUG
// Switch between iobuf and iostd from this define
#define IO_DEF IO_BUF

#define abs(x) ((x)<0 ? -(x) : (x))

#define TIMER_TICK_PERIOD  100000 // 100ms
#define TOTAL_TICKS        200 // 100ms*30000 = 300s = 5min
#define SDRAM_BUFFER       100000
#define SDRAM_BUFFER_X     (SDRAM_BUFFER*1.2)
#define LZSS_EOF           -1
#define PACKETS_NUM        1000000
#define NUMBER_OF_CHIPS    48
#define NUMBER_OF_XCHIPS   8
#define NUMBER_OF_YCHIPS   8
#define CHIPS_TX_N         6
#define CHIPS_RX_N         6
#define DECODE_ST_SIZE     6

uint end_marker = 0;
uint enc_stream = 0;
uint packets = 0;
uint bit_buffer = 0;
uint bit_mask = 128;
uint codecount;
uint textcount;
uint buffer[N*2];

uint coreID;
uint chipID;
uint chipIDx, chipIDy, chipNum;
uint decode_status=0;

float t;
uint t_int, t_frac;

// Remember that these are just base addresses and not memory allocations
static volatile uint *const finish =
            (uint *) (SPINN_SDRAM_BASE + 0); // size: 6 ints

static volatile uint *const decode_status_chip =
            (uint *) (SPINN_SDRAM_BASE + 6*sizeof(uint)); // size: 6 ints

sdp_msg_t my_msg;

typedef struct sdram
{
  uint size;
  unsigned char *buffer;
} sdram_t;

sdram_t data_orig, data_enc, data_dec;

// Spinnaker function prototypes
void router_setup(void);
void allocate_memory(void);
void gen_random_data(uint trial_num);
void encode_decode(uint none1, uint none2);
void store_packets(uint key, uint payload);
void report_status(uint ticks, uint null);
void app_done();
void sdp_init();
char *itoa (uint n);
int  count_chars(char *str);

void send_msg(char *s, uint s_len);
int intg(float num);
int frac(float num, uint precision);

// Tx/Rx packets function prototypes
void count_packets(uint key, uint payload);
void tx_packets(uint none1, uint none2);

int c_main(void)
{
  uint trial_num;

  // Get core and chip IDs
  coreID = spin1_get_core_id();
  chipID = spin1_get_chip_id();

  // get this chip's coordinates for core map
  chipIDx = chipID>>8;
  chipIDy = chipID&255;
  chipNum = (chipIDx * NUMBER_OF_YCHIPS) + chipIDy;

  // initialise SDP message buffer
  sdp_init();
  *finish=0;

  // set timer tick value (in microseconds)
  spin1_set_timer_tick(TIMER_TICK_PERIOD);

  // Rx packet callback. High priority.
  // capture rx packets and store them in SDRAM
  spin1_callback_on(MCPL_PACKET_RECEIVED, store_packets, -1);

  // Timer callback which reports status to the host
  spin1_callback_on(TIMER_TICK, report_status, 1);

  // Encode and decode the previously generated random data and store in SDRAM
  spin1_schedule_callback(encode_decode, 0, 0, 2);

  // -----------------------
  // Initialization phase
  // -----------------------
  // Setup router links
  router_setup();
  
  // Allocate SDRAM memory for the original, encoded and decoded arrays
  allocate_memory();

    trial_num = 0;
//  while(trial_num<=1)
//  {
    // Generate the random data for encoding/decoding and trasmitting to the
    // neighbouring chips
    gen_random_data(trial_num); // *** Pass the trial number to change the random numbers on each trial ***

    // Go
    // *** Where should this be placed??
    spin1_start(SYNC_WAIT);

    // Increment trial number
//    trial_num++;
//  }

  // report results
  app_done();

  return 0;
}


/* ------------------------------------------------------------------- */
/* initialise routing entries                                          */
/* ------------------------------------------------------------------- */
void router_setup(void)
{
  uint e;
    
  if (coreID==1) // core 1 of any chip
  {
    e = rtr_alloc(12);
    if (!e)
      rt_error(RTE_ABORT);

    // Set up external links
    //         entry key             mask      route             
    rtr_mc_set(e,    (chipID<<8)+NORTH, MASK_ALL, N_LINK);
    rtr_mc_set(e+1,  (chipID<<8)+SOUTH, MASK_ALL, S_LINK);
    rtr_mc_set(e+2,  (chipID<<8)+EAST,  MASK_ALL, E_LINK);
    rtr_mc_set(e+3,  (chipID<<8)+WEST,  MASK_ALL, W_LINK);
    rtr_mc_set(e+4,  (chipID<<8)+NEAST, MASK_ALL, NE_LINK);
    rtr_mc_set(e+5,  (chipID<<8)+SWEST, MASK_ALL, SW_LINK);

    // Set up chip routes
    //         entry key                        mask      route             
    rtr_mc_set(e+6,  ((chipID+y1)<<8)    + SOUTH, MASK_ALL, CORE7);  // from S
    rtr_mc_set(e+7,  ((chipID-y1)<<8)    + NORTH, MASK_ALL, CORE8);  // from N
    rtr_mc_set(e+8,  ((chipID+x1)<<8)    + WEST,  MASK_ALL, CORE9);  // from W
    rtr_mc_set(e+9,  ((chipID-x1)<<8)    + EAST,  MASK_ALL, CORE10); // from E
    rtr_mc_set(e+10, ((chipID+x1+y1)<<8) + SWEST, MASK_ALL, CORE11); // from SW
    rtr_mc_set(e+11, ((chipID-x1-y1)<<8) + NEAST, MASK_ALL, CORE12); // from NE
  }

  /* ------------------------------------------------------------------- */
  /* initialize the application processor resources                      */
  /* ------------------------------------------------------------------- */
  io_printf (IO_BUF, "[Core %d] -- Routes configured\n", coreID);

}


// Allocate the SDRAM memory for the transmit as well as the receive chips
void allocate_memory(void)
{
  int  i, s_len;
  char str[100];

  // Transmit and receive chips memory allocation
  if (coreID>=1 && coreID<=CHIPS_TX_N+CHIPS_RX_N)
  {
    finish[coreID-1] = 0;

    // Welcome message
    io_printf (IO_DEF, "[chip (%d,%d) core %d] LZSS Encode/Decode Test\n", chipID>>8, chipID&255, coreID);

    /*******************************************************/
    /* Allocate memory                                     */
    /*******************************************************/
    
    // Original array
    if (!(data_orig.buffer   = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER*sizeof(char), 0, ALLOC_LOCK)))
    {
      // Send SDP message
      strcpy(str, "Unable to allocate memory!\n");
      s_len = count_chars(str);
      send_msg(str, s_len);

      rt_error(RTE_ABORT);
    }      
    //Initialize buffer
    //for(i=0; i<SDRAM_BUFFER_X; i++)
    //  data_orig.buffer[i] = 0;

    // Compressed array
    if (!(data_enc.buffer = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER_X*sizeof(char), 0, ALLOC_LOCK)))
    {
      // Send SDP message
      strcpy(str, "Unable to allocate memory!\n");
      s_len = count_chars(str);
      send_msg(str, s_len);

      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER_X; i++)
      data_enc.buffer[i] = 0;

    // Decompressed array
    if (!(data_dec.buffer = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER*sizeof(char), 0, ALLOC_LOCK)))
    {
      // Send SDP message
      strcpy(str, "Unable to allocate memory!\n");
      s_len = count_chars(str);
      send_msg(str, s_len);

      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER; i++)
      data_dec.buffer[i] = 0;
  }  
}

// Generate the random data array for the transmit chips
void gen_random_data(uint trial_num)
{
  if (coreID>=1 && coreID<=CHIPS_TX_N+CHIPS_RX_N)
  {
    //Seed random number generate
    sark_srand(chipID + coreID);

    if (coreID>=1 && coreID<=CHIPS_TX_N)
    {
      //Initialize buffer
      for(uint i=0; i<SDRAM_BUFFER; i++)
        data_orig.buffer[i] = sark_rand()%256;

      //Assign data size
      data_orig.size = SDRAM_BUFFER;
    }
  }
}


// Encode randomly generated data using the LZSS compression algorithm
// Decode the compressed data and compare with the original data
//
// Since the LZSS algorithm is trying to compress random data, the result is
// that the output array is bigger than the input array by approximately 12%
void encode_decode(uint none1, uint none2)
{
  //int  tmp, i, err=0;
  //char str[100];

  //All chips
  if (coreID>=1 && coreID<=DECODE_ST_SIZE)
  {
    /*******************************************************/
    /* Encode array                                        */
    /*******************************************************/
    io_printf(IO_DEF, "[chip (%d,%d) core %d] Encoding...\n", chipID>>8, chipID&255, coreID);
    encode(0, 0);

    /*******************************************************/
    /* Decode array (locally)                              */
    /*******************************************************/
    io_printf(IO_DEF, "[chip (%d,%d) core %d] Decoding...\n", chipID>>8, chipID&255, coreID);
    decode(0, 0);

    // Print out results
    #ifdef DEBUG
      io_printf(IO_BUF, "\nOutput of original array, encoded and decoded output\n");
      io_printf(IO_BUF, "--|--------------------\n");

      if (i<data_enc.size)
        for(i=data_orig.size; i<data_enc.size; i++)
          io_printf(IO_BUF, "%2d|       %3d\n", i, data_enc.buffer[i]);
    #endif

    finish[coreID-1] = 1;

    // Schedule transmission
    spin1_schedule_callback(tx_packets, 0, 0, 2);

  }
}


// Transmit packets to neighbouring chips
// Cores 1-6 will send packets to the neighbors on the N, S, E, W, NE, SW links
// Cores 1-6 are already inferred from the calling function (encode_decode)
// Cores 7-12 receive
void tx_packets(uint none1, uint none2)
{
  int i;

  for(i=0; i<data_orig.size; i++)
  {
    while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_orig.buffer[i], WITH_PAYLOAD));
    // 1 us delay
    spin1_delay_us(1);
  }
  // Send end of stream marker (NB payload is 32-bit, and I'm only sending a byte at a time)
  while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));

  for(i=0; i<data_enc.size; i++)
  {
    while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_enc.buffer[i], WITH_PAYLOAD));
    // 1 us delay
    spin1_delay_us(1);
  }
  // Send end of stream marker (NB payload is 32-bit, and I'm onyl sending a byte at a time)
  while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));
}

// Count the packets received
void store_packets(uint key, uint payload)
{
  uint s_len;
  char s[80];

  packets++;

  if (payload!=0xffffffff)
  {
/*    if (!enc_stream)
      data_orig.buffer[packets-1] = payload & 255;
    else
      data_enc.buffer[packets-1] = payload & 255;
*/

    if (!enc_stream && packets<=SDRAM_BUFFER)
      data_orig.buffer[packets-1] = payload & 255;
    else if (!enc_stream && packets>SDRAM_BUFFER)
    {
      // Send SDP message
      strcpy(s, "Error! Array index out of bounds!\n");
      s_len = count_chars(s);
      send_msg(s, s_len);
    }
    else if (enc_stream && packets<=SDRAM_BUFFER_X)
      data_enc.buffer[packets-1] = payload & 255;
    else if (enc_stream && packets>SDRAM_BUFFER_X)
    {
      // Send SDP message
      strcpy(s, "Error! Array index out of bounds!\n");
      s_len = count_chars(s);
      send_msg(s, s_len);
    }

  }
  else
  {
    if (!enc_stream)
    {
      data_orig.size = packets-1;
      packets    = 0;
      enc_stream = 1;
    }
    else
    {
      data_enc.size = packets-1;
      packets    = 0;
      enc_stream = 0;

      io_printf(IO_DEF, "Decoding neighbour data\n");

      // Decode data from neighbouring chips
      decode(0,0);
    }
  }

  // report packet value
  //io_printf(IO_BUF, "* Key:(%d,%d) Payload:%d\n", (key>>16), (key>>8)&255, payload);
}

// Send SDP packet to host (for reporting purposes)
void send_msg(char *s, uint s_len)
{
  spin1_memcpy(my_msg.data, (void *)s, s_len);
  
  //s_len = sizeof(uint);
  spin1_memcpy(my_msg.data, (void *)s, s_len);
  my_msg.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + s_len;

  // Send SDP message
  (void)spin1_send_sdp_msg(&my_msg, 10); // 10ms timeout
}

int count_chars(char *str)
{
  int i=0;

  while(str[i++]!='\0');

  return i-1;
}

char *itoa (uint n)
{
    char s[32];
    static char rv[32];
    int i = 0, j;
// pop one decimal value at a time starting
// with the least significant
    do {
        s[i++] = '0' + n%10;
        n /= 10;
    } while (n>0);

// digits will be in reverse order
    for (j = 0; j < i; j++)
      rv[j] = s[i-j-1];

    rv[j] = '\0';
    return rv;
}
 

void sdp_init()
{
  my_msg.tag       = 1;             // IPTag 1
  my_msg.dest_port = PORT_ETH;      // Ethernet
  my_msg.dest_addr = sv->dbg_addr;  // Root chip

  my_msg.flags     = 0x07;          // Flags = 7
  my_msg.srce_port = spin1_get_core_id ();  // Source port
  my_msg.srce_addr = spin1_get_chip_id ();  // Source addr
}


void report_status(uint ticks, uint null)
{
  uint i, s_len, finish_tmp=0;
  char s[100];
  static uint done = 0;

  // stop if all cores finished encode/decode process
  for(i=0; i<DECODE_ST_SIZE; i++)
    finish_tmp += finish[i];

  //decode_status_chip[coreID-1] = decode_status;

  // send results to host
  // only the lead application core does this
  if (coreID==1 && finish_tmp==DECODE_ST_SIZE && !done)
  {
    // Spread out the reporting to avoid SDP packet drop
    if ((ticks % (NUMBER_OF_XCHIPS * NUMBER_OF_YCHIPS)) == chipNum)
    {

      strcpy(s, "Status: ");
      for(i=0; i<DECODE_ST_SIZE; i++)
      {
        strcat(s, itoa(decode_status_chip[i]));
        // Don't put a space at the end of the string
        if (i<DECODE_ST_SIZE-1)
          strcat(s, " ");
      }
      s_len = count_chars(s);

      // Send SDP message
      send_msg(s, s_len);

      io_printf(IO_DEF, "%s\n", s);
      done = 1;
    }
  }

// Exit
//  if (*msg_sent && finish_tmp == DECODE_ST_SIZE)
//    spin1_exit(0);
  
  // Stop if desired number of ticks reached
  if (ticks >= TOTAL_TICKS)
    spin1_exit (0);
}


/***********************************/
/*   LZSS Encode Algorithm         */
/*   Adapted from Karuhiko Okumura */
/***********************************/
void encode(uint none1, uint none2)
{
  int i, j, f1, x, y, r, s, bufferend, c;
  int progress, progress_tmp = 0;

  textcount=0;
  codecount=0;

  for (i=0; i<N-F; i++)
    buffer[i] = ' ';
  
  for (i=N-F; i<N*2; i++)
  {
    if (textcount==data_orig.size) break;
    
    c = data_orig.buffer[textcount++];
    buffer[i] = c;
  }

  bufferend = i;
  r = N-F;
  s = 0;

  while (r<bufferend)
  {
    f1 = (F <= bufferend-r) ? F : bufferend-r;

    x = 0;
    y = 1;
    c = buffer[r];
    for (i=r-1; i>=s; i--)
    {   
      if (buffer[i] == c)
      {
        for (j=1; j<f1; j++)
        {
          if (buffer[i+j] != buffer[r+j]) break;
        }
        
        if (j > y)
        {
          x = i;
          y = j;
        }
      }
    }

    if (y <= P)
      output1(c);
    else
      output2(x & (N-1), y-2);
    
    r += y; 
    s += y;
    if (r >= N*2-F)
    {
      for (i=0; i<N; i++)
        buffer[i] = buffer[i+N];
      

      bufferend -= N;
      r -= N;
      s -= N;
      while (bufferend < N*2)
      {
        if (textcount==data_orig.size) break;

        c = data_orig.buffer[textcount++];
        buffer[bufferend++] = c;

        // Display encode progress
        progress = (int)((textcount*100)/data_orig.size);
        if (progress%10==0 && progress!=progress_tmp)
        {  
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          t_int  = intg(t);
          t_frac = frac(t,1);
          io_printf(IO_DEF, "[chip (%d,%d) core %d] %d%% Time: %d.%ds\n",
                      chipID>>8, chipID&255, coreID, progress, t_int, t_frac);

          progress_tmp = progress;
        }

      }
    }
  }

  flush_bit_buffer();

/*  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
  t_int  = intg(t);
  t_frac = frac(t,1);
*/
  io_printf(IO_DEF, "[chip (%d,%d) core %d] Original array: %d bytes\n", chipID>>8, chipID&255, coreID, textcount);
  io_printf(IO_DEF, "[chip (%d,%d) core %d] Encoded array:  %d bytes (%d%%)\n", chipID>>8, chipID&255, coreID, codecount, (codecount*100)/textcount);
  io_printf(IO_DEF, "Time: %d.%ds\n", t_int, t_frac);
}

void flush_bit_buffer(void)
{
    if (bit_mask != 128) {
      data_enc.buffer[codecount++] = bit_buffer;
    }
    data_enc.size = codecount;
}

inline void output1(int c)
{
  int mask;
  
  putbit1();
  mask = 256;
  while (mask >>= 1)
  {
    if (c & mask)
      putbit1();
    else
      putbit0();
  }
}

inline void output2(int x, int y)
{
  int mask;
  
  putbit0();
  mask = N;
  while (mask >>= 1)
  {
    if (x & mask)
      putbit1();
    else
      putbit0();
  }
  mask = (1 << EJ);
  while (mask >>= 1)
  {
    if (y & mask)
      putbit1();
    else
      putbit0();
  }
}

inline void putbit0(void)
{
  if ((bit_mask >>= 1) == 0)
  {
    data_enc.buffer[codecount++] = bit_buffer;

    bit_buffer = 0;
    bit_mask = 128;
  }
}

inline void putbit1(void)
{
  bit_buffer |= bit_mask;
  if ((bit_mask >>= 1) == 0)
  {
    data_enc.buffer[codecount++] = bit_buffer;

    bit_buffer = 0;
    bit_mask = 128;
  }
}

/***********************************/
/*   LZSS Decode Algorithms        */
/*   Adapted from Karuhiko Okumura */
/***********************************/
// This decode should only be called on cores 7-12 (depending on the multicast packet routes)
void decode(uint none1, uint none2)
{
  int i, j, k, r, c;
  int err=0, tmp; //, s_len;
  //unsigned char s[80];

  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
  t_int  = intg(t);
  t_frac = frac(t,1);
  io_printf(IO_DEF, "Time: %d.%ds\n", t_int, t_frac);

  textcount=0;
  codecount=0;

  for (i=0; i<N-F; i++)
    buffer[i] = ' ';

  r = N-F;
  while ((c = getbit(1)) != LZSS_EOF)
  {
    if (c)
    {
      if ((c = getbit(8)) == LZSS_EOF)
        break;

      // Error check
/*      if (textcount>SDRAM_SIZE)
      {
        // Send SDP message
        strcpy(s, "Error! Array index out of bounds!\n");
        s_len = count_chars(s);
        send_msg(s, s_len);
        err = 1;

        break;
      }*/

      data_dec.buffer[textcount++] = c;
      buffer[r++] = c;
      r &= (N - 1);
    }
    else
    {
      if ((i = getbit(EI)) == LZSS_EOF) break;
      if ((j = getbit(EJ)) == LZSS_EOF) break;
      
      for (k = 0; k <= j+1; k++)
      {
        c = buffer[(i+k) & (N-1)];

        // Error check
/*        if (textcount>SDRAM_SIZE)
        {
          // Send SDP message
          strcpy(s, "Error! Array index out of bounds!\n");
          s_len = count_chars(s);
          send_msg(s, s_len);
          err = 1;

          break;
        }*/

        data_dec.buffer[textcount++] = c;
        buffer[r++] = c;
        r &= (N-1);
      }
    }
  }

  if (!err)
    data_dec.size = textcount;
  else
    data_dec.size = 0;


  // *** Put this decode checking in a separate function ***
  #ifdef DEBUG
    // Print out results
    io_printf(IO_BUF, "\nOutput of original/encoded transmitted array\n");
    io_printf(IO_BUF, "Original: %d bytes, Encoded: %d, Decoded: %d bytes\n", data_orig.size, data_enc.size, data_dec.size);
    io_printf(IO_BUF, "--|--------------------\n");
  #endif

  err=0;
  for(i=0; i<data_orig.size; i++)
  {  
    tmp = abs(data_orig.buffer[i] - data_dec.buffer[i]);
      err+=tmp;

    #ifdef DEBUG
      if (i<data_enc.size)
        io_printf(IO_BUF, "%2d|  %3d  %3d  %3d  %3d\n", i, data_orig.buffer[i], data_enc.buffer[i], data_dec.buffer[i], err);
      else
        io_printf(IO_BUF, "%2d|  %3d       %3d  %3d\n", i, data_orig.buffer[i], data_dec.buffer[i], err);
    #endif
  }

  #ifdef DEBUG
    if (i<data_enc.size)
      for(i=data_orig.size; i<data_enc.size; i++)
        io_printf(IO_BUF, "%2d|       %3d\n", i, data_enc.buffer[i]);

  #endif

  // This is an encode/decode check on the same core (for data received from neighnouring chips)
  if (err)
  {
    io_printf(IO_DEF, "[chip (%d,%d) core %d] ERROR! Original and Decoded Outputs do not match!!!\n", chipID>>8, chipID&255, coreID);
    decode_status_chip[coreID-1] = 2;
  }
  else
  {
    io_printf(IO_DEF, "[chip (%d,%d) core %d] Original and Decoded Outputs Match!\n", chipID>>8, chipID&255, coreID);
    decode_status_chip[coreID-1] = 1;      
  }

  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
  t_int  = intg(t);
  t_frac = frac(t,1);

  io_printf(IO_DEF, "[chip (%d,%d) core %d] Original array: %d bytes\n", chipID>>8, chipID&255, coreID, data_orig.size);
  io_printf(IO_DEF, "[chip (%d,%d) core %d] Encoded array:  %d bytes\n", chipID>>8, chipID&255, coreID, data_enc.size);
  io_printf(IO_DEF, "[chip (%d,%d) core %d] Decoded array:  %d bytes\n", chipID>>8, chipID&255, coreID, data_dec.size);
  io_printf(IO_DEF, "Total time elapsed: %d.%d s\n", t_int, t_frac);
}

inline int getbit(int n) /* get n bits */
{
  int i, x;
  static int buf, mask = 0;
  
  x = 0;
  for (i = 0; i < n; i++)
  {
    if (mask == 0)
    {
      if (codecount == data_enc.size)
        return LZSS_EOF;

      buf = data_enc.buffer[codecount++];
      mask = 128;
    }
    x <<= 1;
    if (buf & mask) x++;
    mask >>= 1;
  }
  return x;
}

// Return integer part
int intg(float num)
{
  return (int)num;
}

// Return fractional part
int frac(float num, uint precision)
{ 
  int m=1;

  if (precision>0)
    for (int i=0; i<precision; i++)
      m*=10;
      
  return (int)((num-(int)num)*m);
}


void app_done ()
{
  // report simulation time
  if(coreID>=1 && coreID<=CHIPS_TX_N + CHIPS_RX_N)
    io_printf(IO_DEF, "[chip (%d,%d) core %d] Simulation lasted %d ticks.\n\n",chipID>>8, chipID&255, coreID, spin1_get_simulation_time());
}

