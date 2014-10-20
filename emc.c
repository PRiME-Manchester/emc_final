#include <string.h>
#include "spin1_api.h"
#include "lzss.h"
#include "router.h"
#include "jtag.h"

#define abs(x) ((x)<0 ? -(x) : (x))

#define NO_DEBUG   // rename to DEBUG to enable more verbose debugging on iobuf
#define NO_FAULT_TESTING // rename to FAULT_TESTING when injecting faults in the Spinn Links
#define BOARDS24    // rename to BOARDS24 when working with the 24 board machine, BOARDS3 when using 3 board machine
#define TX_PACKETS // rename to disable transmission of packets between chips

#define TIMER_TICK_PERIOD  10000 // 10ms
#define SDRAM_BUFFER       500000
#define SDRAM_BUFFER_X     (SDRAM_BUFFER*1.2)
#define LZSS_EOF           -1
#define DELAY              2 //us delay 
// #define NODELAY // this variable removes any delays, takes precedence over DELAY

#ifdef BOARDS1
  #define NUMBER_OF_CHIPS 48
  #define XCHIPS 8
  #define YCHIPS 8
  #define XCHIPS_BOARD 8
  #define YCHIPS_BOARD 8
#endif

#ifdef BOARDS3
  #define NUMBER_OF_CHIPS  144
  #define XCHIPS           12
  #define YCHIPS           12
  #define XCHIPS_BOARD     8
  #define YCHIPS_BOARD     8
#endif 

#ifdef BOARDS24
  #define NUMBER_OF_CHIPS  1152
  #define XCHIPS           48
  #define YCHIPS           24
  #define XCHIPS_BOARD     8
  #define YCHIPS_BOARD     8
#endif

#define CHIPS_TX_N         6
#define CHIPS_RX_N         6
#define DECODE_ST_SIZE     6 // this should be 6, set to 12 only for testing the SDRAM used by all 12 cores
#define TRIALS             100 //using a buffer of 500000, trials=100, tx_reps=50 results in a run time of 8hrs 
#define TX_REPS            50 

// Address values
#define FINISH             (SPINN_SDRAM_BASE + 0)                // size: 12 ints ( 0..11)
#define DECODE_STATUS_CHIP (SPINN_SDRAM_BASE + 12*sizeof(uint))  // size: 6 ints  (12..17)
#define RX_PACKETS_STATUS  (SPINN_SDRAM_BASE + 18*sizeof(uint))  // size: 6 ints  (18..23)
#define CHIP_INFO_TX       (SPINN_SDRAM_BASE + 24*sizeof(uint))  // size: 19*6=114 bytes (24..137)
#define CHIP_INFO_RX       (SPINN_SDRAM_BASE + 138*sizeof(uint)) // size: 19*6=114 bytes (138..137


volatile uint packets     = 0;
volatile uint bit_buffer  = 0;
volatile uint bit_mask    = 128;
volatile uint decode_done = 0;
volatile uint codecount;
volatile uint textcount;
volatile uint buffer[N*2];

uint coreID;
uint chipID, chipBoardID, ethBoardID;
uint chipIDx, chipIDy, chipNum;
uint chipBoardIDx, chipBoardIDy, chipBoardNum;
uint decode_status = 0;
uint eof_sent = 0;
uint timeout = 0;

volatile float t;
uint t_int, t_frac;
uint error_pkt;

// N.B.
// static (global scope): only declared within this file
// volatile:              compiler does not cache variable
// *const:                compiler uses this value directly as an addresses
//                        (only done for efficienct reasons)
typedef struct
{ 
  volatile uint trial_num;
  volatile uint progress[12];
  volatile uint org_array_size;
  volatile uint enc_array_size;
  volatile uint dec_array_size;
  volatile ushort encode_time;
  volatile ushort decode_time;
  volatile uchar enc_dec_match; // yes/no
} info_tx_t;
static volatile info_tx_t *const info_tx = (info_tx_t *) CHIP_INFO_TX;

typedef struct
{
  volatile uint trial_num;
  volatile uint stream_num;
  volatile uint org_array_size;
  volatile uint enc_array_size;
  volatile uint dec_array_size;
  volatile ushort decode_time;
  volatile uchar enc_dec_match; // yes/no
} info_rx_t;
static volatile info_rx_t *const info_rx               = (info_rx_t *) CHIP_INFO_RX;

static volatile uint   *const finish             = (uint *) FINISH; 
static volatile uint   *const decode_status_chip = (uint *) DECODE_STATUS_CHIP;
static volatile uint   *const rx_packets_status  = (uint *) RX_PACKETS_STATUS;

sdp_msg_t my_msg;

typedef struct
{
  uint size;
  unsigned char *buffer;
} sdram_tx_t;
sdram_tx_t data_orig, data_enc, data_dec;

typedef struct
{
  uint orig_size;
  uint enc_size;
  uint stream_end;
  unsigned char *buffer;
} sdram_rx_t;
sdram_rx_t data;


typedef struct
{
  int chipIDx, chipIDy;
  int coreID;  //1-6 when testing the drop_pkt mechanism, 7-12 when testing the drop_ack
  int trialNum, repNum; // repNum is only used with drop_ack
  int orig_size, enc_size, drop_eof;
  int orig_drop_pkt, orig_mod_pkt, orig_mod_val;
  int enc_drop_pkt,  enc_mod_pkt,  enc_mod_val;
  int drop_ack;
} fault_t;
fault_t fault[3];


// Spinnaker function prototypes
void router_setup(void);
void allocate_memory(void);
void gen_random_data(void);
void encode_decode(uint none1, uint none2);
void store_packets(uint key, uint payload);
void report_status(uint ticks, uint null);
void decode_rx_packets(uint none1, uint none2);
void report_buffer_error(uint none1, uint none2);
void app_done(void);
void sdp_init(void);
char *itoa(uint n);
char *ftoa(float num, int precision);
int  count_chars(char *str);
void check_data(int trial_num);
int mod(int x, int m);
uint spin1_get_chip_board_id(void);
uint spin1_get_eth_board_id(void);

void send_msg(char *s);
int frac(float num, uint precision);

// Tx/Rx packets function prototypes
void count_packets(uint key, uint payload);
void tx_packets(int trialNum);

// Fault testing
void fault_test_init(void);
void ijtag_init(void);

//Reporting
void report_system_setup(void);

int c_main(void)
{
  // Get core and chip IDs
  coreID = spin1_get_core_id();
  chipID = spin1_get_chip_id();
  chipBoardID = spin1_get_chip_board_id();
  ethBoardID  = spin1_get_eth_board_id();

  io_printf(IO_BUF, "Eth_boardID  = (%d,%d)\n", ethBoardID>>8,  ethBoardID&255);
  
  // get this chip's coordinates for core map
  chipIDx = chipID>>8;
  chipIDy = chipID&255;
  chipNum = (chipIDy * YCHIPS) + chipIDx;

  chipBoardIDx = chipBoardID>>8;
  chipBoardIDy = chipBoardID&255;
  chipBoardNum = (chipBoardIDy * YCHIPS_BOARD) + chipBoardIDx;

  // Reset JTAG controller if leadAp
  if (leadAp)
    ijtag_init();

  io_printf(IO_BUF, "ChipID (%d,%d), chipID %d coreID %d\n", chipIDx, chipIDy, chipNum, coreID);
  io_printf(IO_BUF, "Chip_boardID (%d,%d), ChipBoardNum %d\n", chipBoardIDx, chipBoardIDy, chipBoardNum);

  // Initialize variables
  error_pkt = 0;
  for(int i=0; i<12; i++)
  {
    info_tx->trial_num=0;
    info_tx->progress[i]=0;
  }

  // fault testing initialization
  #ifdef FAULT_TESTING
    fault_test_init();
  #endif

  // initialise SDP message buffer
  sdp_init();

  // Send system info to host
  report_system_setup();

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

  gen_random_data(); // *** Pass the trial number to change the random numbers on each trial ***

  // Go
  spin1_start(SYNC_WAIT);

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
    e = rtr_alloc(24);
    if (!e)
      rt_error(RTE_ABORT);

    ////////////////////////////////////////////////////
    // Set up external links
    //         entry key                mask      route             
    rtr_mc_set(e,    (chipID<<8)+NORTH, MASK_ALL, N_LINK);
    rtr_mc_set(e+1,  (chipID<<8)+SOUTH, MASK_ALL, S_LINK);
    rtr_mc_set(e+2,  (chipID<<8)+EAST,  MASK_ALL, E_LINK);
    rtr_mc_set(e+3,  (chipID<<8)+WEST,  MASK_ALL, W_LINK);
    rtr_mc_set(e+4,  (chipID<<8)+NEAST, MASK_ALL, NE_LINK);
    rtr_mc_set(e+5,  (chipID<<8)+SWEST, MASK_ALL, SW_LINK);

    // Set up chip routes
    //         entry key                                                                  mask      route             
    rtr_mc_set(e+6,  (((chipIDx<<8) + mod(chipIDy+1, YCHIPS))<<8)                + SOUTH, MASK_ALL, CORE9);  // from S  9
    rtr_mc_set(e+7,  (((chipIDx<<8) + mod(chipIDy-1, YCHIPS))<<8)                + NORTH, MASK_ALL, CORE12); // from N  12
    rtr_mc_set(e+8,  (((mod(chipIDx+1, XCHIPS)<<8) + chipIDy)<<8)                + WEST,  MASK_ALL, CORE7);  // from W  7
    rtr_mc_set(e+9,  (((mod(chipIDx-1, XCHIPS)<<8) + chipIDy)<<8)                + EAST,  MASK_ALL, CORE10); // from E  10
    rtr_mc_set(e+10, (((mod(chipIDx+1, XCHIPS)<<8) + mod(chipIDy+1, YCHIPS))<<8) + SWEST, MASK_ALL, CORE8);  // from SW 8
    rtr_mc_set(e+11, (((mod(chipIDx-1, XCHIPS)<<8) + mod(chipIDy-1, YCHIPS))<<8) + NEAST, MASK_ALL, CORE11); // from NE 11

    //////////////////////////////////////////////////////
    // Set up external links for the decoding 'DONE' message
    //         entry key                mask      route             
    rtr_mc_set(e+12,  (chipID<<8)+NORTH+6, MASK_ALL, N_LINK);
    rtr_mc_set(e+13,  (chipID<<8)+SOUTH+6, MASK_ALL, S_LINK);
    rtr_mc_set(e+14,  (chipID<<8)+EAST +6, MASK_ALL, E_LINK);
    rtr_mc_set(e+15,  (chipID<<8)+WEST +6, MASK_ALL, W_LINK);
    rtr_mc_set(e+16,  (chipID<<8)+NEAST+6, MASK_ALL, NE_LINK);
    rtr_mc_set(e+17,  (chipID<<8)+SWEST+6, MASK_ALL, SW_LINK);

    // Set up chip routes
    //         entry key                                                                      mask      route             
    rtr_mc_set(e+18, (((chipIDx<<8) + mod(chipIDy+1, YCHIPS))<<8)                + SOUTH + 6, MASK_ALL, CORE3); // from S
    rtr_mc_set(e+19, (((chipIDx<<8) + mod(chipIDy-1, YCHIPS))<<8)                + NORTH + 6, MASK_ALL, CORE6); // from N
    rtr_mc_set(e+20, (((mod(chipIDx+1, XCHIPS)<<8) + chipIDy)<<8)                + WEST  + 6, MASK_ALL, CORE1); // from W
    rtr_mc_set(e+21, (((mod(chipIDx-1, XCHIPS)<<8) + chipIDy)<<8)                + EAST  + 6, MASK_ALL, CORE4); // from E
    rtr_mc_set(e+22, (((mod(chipIDx+1, XCHIPS)<<8) + mod(chipIDy+1, YCHIPS))<<8) + SWEST + 6, MASK_ALL, CORE2); // from SW
    rtr_mc_set(e+23, (((mod(chipIDx-1, XCHIPS)<<8) + mod(chipIDy-1, YCHIPS))<<8) + NEAST + 6, MASK_ALL, CORE5); // from NE
    
    // No longer needed
    // Drop all packets which don't have routes (used to drop packets which
    // wraparound for 3-board and 24-board machines and don't have proper routes configures)
    //rtr_mc_set(e+12, 0, 0, 0);
  }

  /* ------------------------------------------------------------------- */
  /* initialize the application processor resources                      */
  /* ------------------------------------------------------------------- */
  io_printf (IO_BUF, "Routes configured\n", coreID);

}

// Alternative mod function which wraps around in case of negative numbers
// Eg. in case of mod 3
// mod(-3,3) = 0
// mod(-2,3) = 1
// mod(-1,3) = 2
// mod( 0,3) = 0
// mod( 1,3) = 1
// mod( 2,3) = 2
// mod( 3,3) = 0
// mod( 4,3) = 1
int mod(int x, int m)
{
    return (x%m + m)%m;
}

// Allocate the SDRAM memory for the transmit as well as the receive chips
void allocate_memory(void)
{
  int  i;
  //int s_len;
  //char str[100];

  // Transmit and receive chips memory allocation

  // Allocate memory for TX chips
  if (coreID>=1 && coreID<=CHIPS_TX_N)
  {
    finish[coreID-1] = 0;
    decode_status_chip[coreID-1] = 0;
    rx_packets_status[coreID-1] = 0;

    // Welcome message
    io_printf (IO_BUF, "LZSS Enc/Dec Test\n");

    /*******************************************************/
    /* Allocate memory                                     */
    /*******************************************************/
    
    // Original array
    if (!(data_orig.buffer   = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER*sizeof(char), 0, ALLOC_LOCK)))
    {
      io_printf(IO_BUF, "Unable to allocate memory (Orig)!\n");
      rt_error(RTE_ABORT);
    }      
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER; i++)
      data_orig.buffer[i] = 0;

    // Compressed array
    if (!(data_enc.buffer = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER_X*sizeof(char), 0, ALLOC_LOCK)))
    {
      io_printf(IO_BUF, "Unable to allocate memory (Enc)!\n");
      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER_X; i++)
      data_enc.buffer[i] = 0;

    // Decompressed array
    if (!(data_dec.buffer = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER*sizeof(char), 0, ALLOC_LOCK)))
    {
      io_printf(IO_BUF, "Unable to allocate memory (Dec)!\n");
      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER; i++)
      data_dec.buffer[i] = 0;
  }

  // Allocate memory for RX chips
  if(coreID>=CHIPS_TX_N+1 && coreID<=CHIPS_TX_N+CHIPS_RX_N)
  {
    if (!(data.buffer   = (unsigned char *)sark_xalloc (sv->sdram_heap, (SDRAM_BUFFER+SDRAM_BUFFER_X+8)*sizeof(char), 0, ALLOC_LOCK)))
    {
      io_printf(IO_BUF, "Unable to allocate memory (Rx)!\n");
      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER+SDRAM_BUFFER_X+8; i++)
      data.buffer[i] = 0;

    // Decompressed array
    if (!(data_dec.buffer = (unsigned char *)sark_xalloc (sv->sdram_heap, SDRAM_BUFFER*sizeof(char), 0, ALLOC_LOCK)))
    {
      io_printf(IO_BUF, "Unable to allocate memory (Dec)!\n");
      rt_error(RTE_ABORT);
    }
    //Initialize buffer
    for(i=0; i<SDRAM_BUFFER; i++)
      data_dec.buffer[i] = 0;
  
    data.stream_end = 1;
  }

}

// Generate the random data array for the transmit chips
void gen_random_data(void)
{
  if (coreID>=1 && coreID<=CHIPS_TX_N)
  {
    //Seed random number generate
    //sark_srand(chipID + coreID);
    sark_srand(35);

    //Initialize buffer
    for(uint i=0; i<SDRAM_BUFFER; i++)
      data_orig.buffer[i] = sark_rand()%256;

    //Assign data size
    data_orig.size = SDRAM_BUFFER;
  }
}


// Encode randomly generated data using the LZSS compression algorithm
// Decode the compressed data and compare with the original data
//
// Since the LZSS algorithm is trying to compress random data, the result is
// that the output array is bigger than the input array by approximately 12%
void encode_decode(uint none1, uint none2)
{
  int i, j;
  //int t1, t_e;
  //int err=0;
  char s[100];

  for(i=0; i<TRIALS; i++)
  {
    // Send trial no. to host
    if (chipIDx==0 && chipIDy==0 && leadAp)
    {
      t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
      io_printf(s, "T: %ss. Trial: %d", ftoa(t,1), i+1);
      send_msg(s);
    }

    if (coreID>=1 && coreID<=DECODE_ST_SIZE)
    {
      io_printf(IO_BUF, "Trial: %d\n", i+1);
      
      //All chips
      info_tx->trial_num = i+1;
      info_tx->progress[coreID-1] = 0;
      finish[coreID-1] = 0;
      
      /*******************************************************/
      /* Encode array                                        */
      /*******************************************************/
      io_printf(IO_BUF, "Enc...\n");
      encode();

      /*******************************************************/
      /* Decode array (locally)                              */
      /*******************************************************/
      io_printf(IO_BUF, "\nDec...\n");
      decode();

      check_data(i+1);

      ////////////////////////////////////////
      // Send IO_BUF to host
      // Stagger output so no conflicts arise
      // Wait for turn to send data to host to avoid conflicts
      if (coreID>=1 && coreID<=6)
      {
        while ((spin1_get_simulation_time() % (XCHIPS_BOARD * YCHIPS_BOARD * 6))
                  != chipBoardNum*6+coreID-1);
        
        io_printf(IO_BUF, "Ticks:%d\n", spin1_get_simulation_time());

        //t1 = spin1_get_simulation_time();
        //t_e = spin1_get_simulation_time() - t1;
        //io_printf(IO_BUF, "t_e (ticks) = %d\n", t_e);
      }
      ////////////////////////////////////////

      finish[coreID-1] = 1;

#ifdef TX_PACKETS
      for(j=0; j<TX_REPS; j++)
      {
        io_printf(IO_BUF, "TX pkt (Rep %d)...\n", j+1);

        if (chipID==0 && coreID==1)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "T: %ss. Transmitting packets (rep %d) ...", ftoa(t,1), j+1);
          send_msg(s);
        }
        
        decode_done = 0;
        
        // Transmit packets TX_REPS times
        tx_packets(j);

        io_printf(IO_BUF, "Waiting for dec\n");
        if (chipID==0 && coreID==1)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "T: %ss. Waiting for decode to finish", ftoa(t,1));
          send_msg(s);
        }

        // Wait for decode_done signal
        while(!decode_done);
        io_printf(IO_BUF, "Dec done Rx\n");
        if (chipID==0 && coreID==1)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "T: %ss. Decode done received", ftoa(t,1));
          send_msg(s);
        }

        decode_done = 0;
      } 
#endif

    }
  }
}

// Transmit packets to neighbouring chips
// Cores 1-6 will send packets to the neighbors on the N, S, E, W, NE, SW links
// Cores 1-6 are already inferred from the calling function (encode_decode)
// Cores 7-12 receive
void tx_packets(int trialNum)
{
  int i, num, shift;

#ifdef FAULT_TESTING
    int j;
    uchar drop_pkt, mod_pkt;
#endif

  // Sending orig array size
#ifdef FAULT_TESTING
    mod_pkt = 0;
    for(j=0;j<sizeof(fault); j++)
    {
      mod_pkt = fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].orig_size;
      if (mod_pkt)
        break;
    }

    if(mod_pkt)
    {
      io_printf(IO_BUF, "Orig. size modified!\n");
      shift = 0;
      for (i=0; i<4; i++)
      {
        num = (fault[j].orig_size>>shift) & 255;
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
        spin1_delay_us(DELAY);
  #endif
        shift+=8;
      }
    }
    else
    {
      shift = 0;
      for (i=0; i<4; i++)
      {
        num = (data_orig.size>>shift) & 255;
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
        spin1_delay_us(DELAY);
  #endif
        shift+=8;
      }
    }
#else  
    shift = 0;
    for (i=0; i<4; i++)
    {
      num = (data_orig.size>>shift) & 255;
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
      spin1_delay_us(DELAY);
  #endif
      shift+=8;
    }
#endif

  // Sending encoded array size
#ifdef FAULT_TESTING
    mod_pkt = 0;
    for(j=0;j<sizeof(fault); j++)
    {
      mod_pkt = fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].enc_size;
      if (mod_pkt)
        break;
    }

    if (mod_pkt)
    {
      io_printf(IO_BUF, "Enc. size modified!\n");
      shift=0;
      for (i=0; i<4; i++)
      {
        num = (fault[j].enc_size>>shift) & 255;
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
        spin1_delay_us(DELAY);
  #endif
        shift+=8;
      }
    }
    else
    {
      shift=0;
      for (i=0; i<4; i++)
      {
        num = (data_enc.size>>shift) & 255;
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
        spin1_delay_us(DELAY);
  #endif
        shift+=8;
      }
    }
#else
    shift=0;
    for (i=0; i<4; i++)
    {
      num = (data_enc.size>>shift) & 255;
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, num, WITH_PAYLOAD));
  #ifndef NODELAY
      spin1_delay_us(DELAY);
  #endif
      shift+=8;
    }
#endif

  // Sending original stream
  for(i=0; i<data_orig.size; i++)
  {
#ifdef FAULT_TESTING
      drop_pkt = 0;
      for(j=0;j<sizeof(fault); j++)
        drop_pkt |= fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                    fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].orig_drop_pkt==i;
      
      mod_pkt = 0;
      for(j=0;j<sizeof(fault); j++)
      {
        mod_pkt = fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                  fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].orig_mod_pkt==i;
        if (mod_pkt)
          break;
      }

      if (!drop_pkt && !mod_pkt)
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_orig.buffer[i], WITH_PAYLOAD));
      else if (mod_pkt)
      {
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, fault[j].orig_mod_val, WITH_PAYLOAD));
        io_printf(IO_BUF, "Pkt %d modified (Orig)!\n", fault[j].orig_mod_pkt);
      }
      else
        io_printf(IO_BUF, "Pkt %d dropped (Orig)!\n", i);

#else
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_orig.buffer[i], WITH_PAYLOAD));
#endif

    spin1_delay_us(DELAY);
  }

  // Sending encoded stream
  for(i=0; i<data_enc.size; i++)
  {
#ifdef FAULT_TESTING
      drop_pkt = 0;
      for(j=0; j<sizeof(fault); j++)
        drop_pkt |= fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                    fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].enc_drop_pkt==i;

      mod_pkt = 0;
      for(j=0;j<sizeof(fault); j++)
      {
        mod_pkt = fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                  fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].enc_mod_pkt==i;
        if (mod_pkt)
          break;
      }

      if (!drop_pkt && !mod_pkt)
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_enc.buffer[i], WITH_PAYLOAD));
      else if (mod_pkt)
      {
        while(!spin1_send_mc_packet((chipID<<8)+coreID-1, fault[j].enc_mod_val, WITH_PAYLOAD));
        io_printf(IO_BUF, "Pkt %d modified (Enc)!\n", fault[j].enc_mod_pkt);
      }
      else
        io_printf(IO_BUF, "Pkt %d dropped (Enc)!\n", i);

#else
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, data_enc.buffer[i], WITH_PAYLOAD));
#endif

#ifndef NODELAY
    spin1_delay_us(DELAY);
#endif
  }

#ifdef FAULT_TESTING
  drop_pkt = 0;
  for(j=0; j<sizeof(fault); j++)
    drop_pkt |= fault[j].chipIDx==chipIDx && fault[j].chipIDy==chipIDy &&
                fault[j].coreID==coreID && fault[j].trialNum==trialNum && fault[j].drop_eof;

  // Drop 1 out of 2 packets
  if (!drop_pkt)
    while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));
  else
    io_printf(IO_BUF, "One EOF marker packet dropped!\n");

  while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));
#else
  // Send the EOF stream market twice to increase robustness
  while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));
  
  #ifndef NODELAY
  spin1_delay_us(DELAY);
  #endif
  while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xffffffff, WITH_PAYLOAD));

  #ifndef NODELAY
  spin1_delay_us(DELAY);
  #endif

#endif

  // eof_sent is used in the ack received timeout
  eof_sent = 1;
}

// Count the packets received
void store_packets(uint key, uint payload)
{
  if (payload==0xffffffff && !data.stream_end)
  {  
    error_pkt = 0;
    spin1_schedule_callback(decode_rx_packets, 0, 0, 2);
  }
  else if (payload==0xefffffff)
  {
    decode_done = 1;
    eof_sent    = 0;
  }
  else if (!error_pkt)
  {
    data.buffer[packets++] = payload;
    data.stream_end = 0;
  }

  if (packets==SDRAM_BUFFER+SDRAM_BUFFER_X+8)
  {
    error_pkt = 1;
    spin1_schedule_callback(report_buffer_error, 0, 0, 2);
  }

}

// Send SDP packet to host (for reporting purposes)
void send_msg(char *s)
{
  int s_len;

  s_len = strlen(s);
  spin1_memcpy(my_msg.data, (void *)s, s_len);
  
  my_msg.length = sizeof(sdp_hdr_t) + sizeof(cmd_hdr_t) + s_len;

  // Send SDP message
  while(!spin1_send_sdp_msg(&my_msg, 1)); // 10ms timeout
}

int count_chars(char *str)
{
  int i=0;

  while(str[i++]!='\0');

  return i-1;
}

void sdp_init(void)
{
  my_msg.tag       = 1;             // IPTag 1
  my_msg.dest_port = PORT_ETH;      // Ethernet
  my_msg.dest_addr = sv->eth_addr;  // Eth connected chip on this board

  my_msg.flags     = 0x07;          // Flags = 7
  my_msg.srce_port = spin1_get_core_id ();  // Source port
  my_msg.srce_addr = spin1_get_chip_id ();  // Source addr
}

void report_buffer_error(uint none1, uint none2)
{
  char s[80];

  io_printf(IO_BUF, "Pkt buf exceeded (%d)!\n", packets);
  strcpy(s, "Packet buffer exceeded!");
  send_msg(s);
}

void decode_rx_packets(uint none1, uint none2)
{
  static int trial_num=0;
  char s[100];

  if (packets>0)
  {
    io_printf(IO_BUF, "\nTrial %d\n", trial_num+1);

    data.orig_size = (data.buffer[3]<<24) + (data.buffer[2]<<16) + (data.buffer[1]<<8) + data.buffer[0];
    data.enc_size  = (data.buffer[7]<<24) + (data.buffer[6]<<16) + (data.buffer[5]<<8) + data.buffer[4];
    io_printf(IO_BUF, "Pkt Exp Orig:%d Enc:%d Tot:%d\n", data.orig_size, data.enc_size, data.orig_size+data.enc_size);
    io_printf(IO_BUF, "Pkt Rx(Tot):%d(%d)\n", packets-8, packets);
    if (packets-8 != data.orig_size+data.enc_size)
    {
      rx_packets_status[coreID-7] = 2;

      io_printf(IO_BUF, "ERROR! Rx!=Exp pkt!\n");
      io_printf(s, "ERROR! Trial: %d Rx packets (%d) != Expected packets (%d)!", trial_num, packets-8, data.orig_size+data.enc_size);
      send_msg(s);

      // Decoding done
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xefffffff, WITH_PAYLOAD));
    }
    else
    {
      rx_packets_status[coreID-7] = 1;
      data_orig.size   = data.orig_size;
      data_orig.buffer = data.buffer + 8;
      
      data_enc.size   = data.enc_size;
      data_enc.buffer = data.buffer + data.orig_size + 8;

      decode();

      check_data(trial_num+1);

      // Decoding done
      while(!spin1_send_mc_packet((chipID<<8)+coreID-1, 0xefffffff, WITH_PAYLOAD));

      io_printf(IO_BUF, "Rx=Exp pkt!\n");
    }

    trial_num++;
    packets = 0;
    data.stream_end = 1;
  }
}


void report_status(uint ticks, uint null)
{
  uint finish_tmp=0;
  char s[100];
  static int done = 0;
  static int tmp = -1;

  if (chipID==0 && coreID==13 && (ticks % (XCHIPS_BOARD * YCHIPS_BOARD))==chipBoardNum )
  {
    if (tmp!=info_tx->progress[0])
    {
      t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1e6;
      io_printf(s, "T: %ss. Trial: %d Progress: %d%%", ftoa(t,1), info_tx->trial_num, info_tx->progress[0]);
      send_msg(s);

      tmp = info_tx->progress[0];
    }
  }

  if (coreID>=1 && coreID<=6 && eof_sent)
  {
    timeout++;
    if (timeout==1000) // if TIMER_TICK_PERIOD=10ms, this translates to a timeout of 10s 
    {
      timeout = 0;
      eof_sent = 0;
      io_printf(s, "Acknowledge timeout!");
      send_msg(s);
    }
  }

/*
  decode_status_chip[coreID-1] = decode_status;

  // send results to host
  // only the lead application core does this
  // Spread out the reporting to avoid SDP packet drop
  if (coreID==13 && !done && (ticks % (XCHIPS * YCHIPS)) == chipNum)
  {
    for(i=0; i<DECODE_ST_SIZE; i++)
      finish_tmp+=decode_status_chip[i];

    if (finish_tmp==DECODE_ST_SIZE)
    {
      strcpy(s, "Status: ");
      for(i=0; i<DECODE_ST_SIZE; i++)
      {
        strcat(s, itoa(decode_status_chip[i]));
        // Don't put a space at the end of the string
        if (i<DECODE_ST_SIZE-1)
          strcat(s, " ");
      }
      // Send SDP message
      send_msg(s);
    }

    done = 1;
  }
*/

}


/***********************************/
/*   LZSS Encode Algorithm         */
/*   Adapted from Karuhiko Okumura */
/***********************************/
void encode(void)
{
  int i, j, f1, x, y, r, s, bufferend, c;
  int progress, progress_tmp = 0;

  // Initialise values
  bit_mask   = 128;
  bit_buffer = 0;

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
          
          io_printf(IO_BUF, "%d%% T:%ss\n", progress, ftoa(t,1));
          info_tx->progress[coreID-1] = progress;
          progress_tmp = progress;
        }

      }
    }
  }

  flush_bit_buffer();
  data.stream_end = 1;

  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
  io_printf(IO_BUF, "Orig:%d Enc:%d(%d%%)\n", textcount, codecount, (codecount*100)/textcount);
  io_printf(IO_BUF, "T:%ss\n", ftoa(t,1));
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
void decode(void)
{
  int i, j, k, r, c;

  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
  io_printf(IO_BUF, "T:%ss\n", ftoa(t,1));

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

      if (textcount<SDRAM_BUFFER)
        data_dec.buffer[textcount++] = c;
      else
        io_printf(IO_BUF, "Data_dec array index out of bounds!\n");

      if (r<2*N)
        buffer[r++] = c;
      else
        io_printf(IO_BUF, "Buffer array index out of bounds!\n");

      r &= (N - 1);
    }
    else
    {
      if ((i = getbit(EI)) == LZSS_EOF) break;
      if ((j = getbit(EJ)) == LZSS_EOF) break;
      
      for (k = 0; k <= j+1; k++)
      {
        c = buffer[(i+k) & (N-1)];

        if (textcount<SDRAM_BUFFER)
          data_dec.buffer[textcount++] = c;
        else
          io_printf(IO_BUF, "Data_dec array index out of bounds!\n");

        if (r<2*N)
          buffer[r++] = c;
        else
          io_printf(IO_BUF, "Buffer array index out of bounds!\n");

        r &= (N-1);
      }
    }
  }

  data_dec.size = textcount;
}

void check_data(int trial_num)
{
  int i, err = 0;
  char s[100];

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
    if (data_orig.buffer[i]!=data_dec.buffer[i])
    {
      io_printf(IO_BUF, "ERROR:%d i=%d, Orig:%d Enc:%d Dec:%d\n", err, i, data_orig.buffer[i], data_enc.buffer[i], data_dec.buffer[i]);
      err++;
    }

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
    io_printf(IO_BUF, "ERROR! Orig&Dec DO NOT match!!!\n");

    // Send SDP message
    io_printf(s, "ERROR! Original and Decoded Outputs do not match!!! Trial: %d, Errors:%d", trial_num, err);
    send_msg(s);

    decode_status_chip[coreID-1] = 2;
  }
  else
  {
    io_printf(IO_BUF, "Orig&Dec Match!\n");
    decode_status_chip[coreID-1] = 1;      
  }

  t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1e6;
  io_printf(IO_BUF, "Orig:%d Enc:%d Dec:%d\n", data_orig.size, data_enc.size, data_dec.size);
  io_printf(IO_BUF, "T:%ss\n", ftoa(t,1));
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

// Return fractional part
int frac(float num, uint precision)
{ 
  int m=1;

  if (precision>0)
    for (int i=0; i<precision; i++)
      m*=10;
      
  return (int)((num-(int)num)*m);
}

char *itoa(uint n)
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

char *ftoa(float num, int precision)
{
  static char s[20];

  strcpy(s, itoa((int)num));
  strcat(s, ".");
  strcat(s, itoa(frac(num, precision)));

  return s;
}


void app_done(void)
{
  // report simulation time
  if(coreID>=1 && coreID<=CHIPS_TX_N + CHIPS_RX_N)
    io_printf(IO_BUF, "Sim lasted %d ticks.\n\n", spin1_get_simulation_time());
}


uint spin1_get_chip_board_id(void)
{
  return (uint)sv->board_addr;
}

uint spin1_get_eth_board_id(void)
{
  return (uint)sv->eth_addr;
}

// Reset JTAG controller
void ijtag_init(void)
{
   // reset the jtag signals
   sc[GPIO_CLR] = JTAG_TDI + JTAG_TCK + JTAG_TMS + JTAG_NTRST;

   // select internal jtag signals
   sc[SC_MISC_CTRL] |= JTAG_INT;
}

void report_system_setup(void)
{
  char s[100];
  int boards=3;

  if (chipIDx==0 && chipIDy==0 && leadAp)
  {
#ifdef BOARDS24
    boards = 24;
#endif
    io_printf(s, "System setup - Boards:%d, Chips:%d, XChips:%d, YChips:%d, Trials:%d, Reps:%d, Bytes:%d",
                                 boards, NUMBER_OF_CHIPS, XCHIPS, YCHIPS, TRIALS, TX_REPS, SDRAM_BUFFER);
    send_msg(s);
  }
}

void fault_test_init(void)
{
  // Fault 1
  fault[0].chipIDx        = 0;
  fault[0].chipIDy        = 0;
  fault[0].coreID         = 6;
  fault[0].trialNum       = 2;
  fault[0].repNum         = -1;
  fault[0].orig_size      = 25;
  fault[0].enc_size       = 68;
  fault[0].drop_eof       = 1;

  fault[0].orig_drop_pkt  = -1; // packet no. to drop (orig. stream)
  fault[0].orig_mod_pkt   = -1; // packet no. to modify
  fault[0].orig_mod_val   = -1; // packet modify value
  fault[0].enc_drop_pkt   = -1; // packet no. to drop (enc. stream)
  fault[0].enc_mod_pkt    = -1; // packet no. to modify
  fault[0].enc_mod_val    = -1; // packet modify value
  fault[0].drop_ack       = -1; // drop ack

  // Fault 2
  fault[1].chipIDx       = 1;
  fault[1].chipIDy       = 1;
  fault[1].coreID        = 7;
  fault[1].trialNum      = 2;
  fault[1].repNum        = 5;
  fault[1].orig_size     = 0;
  fault[1].enc_size      = 0;
  fault[1].drop_eof      = 0;
  fault[1].orig_drop_pkt = 6;  // packet no. to drop (orig. stream)
  fault[1].orig_mod_pkt  = 12; // packet no. to modify
  fault[1].orig_mod_val  = 1;  // packet modify value
  fault[1].enc_drop_pkt  = 17; // packet no. to drop (enc. stream)
  fault[1].enc_mod_pkt   = 12; // packet no. to modify
  fault[1].enc_mod_val   = 1;  // packet modify value
  fault[1].drop_ack      = -1; // drop ack

  // Fault 3
  fault[2].chipIDx       = -1;
  fault[2].chipIDy       = -1;
  fault[2].coreID        = -1;
  fault[2].trialNum      = -1;
  fault[2].repNum        = -1;
  fault[2].repNum        = -1;
  fault[2].orig_drop_pkt = 6;  // packet no. to drop (orig. stream)
  fault[2].orig_mod_pkt  = 12; // packet no. to modify
  fault[2].orig_mod_val  = 1;  // packet modify value
  fault[2].enc_drop_pkt  = 17; // packet no. to drop (enc. stream)
  fault[2].enc_mod_pkt   = 12; // packet no. to modify
  fault[2].enc_mod_val   = 1;  // packet modify value
  fault[2].drop_ack      = -1; // drop ack
}


