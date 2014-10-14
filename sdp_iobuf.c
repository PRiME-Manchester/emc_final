void encode_decode(uint none1, uint none2)
{
  // From sark_io.c (make this an extern??)
  typedef struct iobuf
  {
    struct iobuf *next;
    uint unix_time;
    uint time_ms;
    uint ptr;
    uchar buf[];
  } iobuf_t;

  int i, j;
  int t1, t_e;
  int vbase, vsize; //size
  vcpu_t *vcpu;
  iobuf_t *iobuf;
  //int err=0;
  char s[100];

  for(i=0; i<TRIALS; i++)
  {
    // Send trial no. to host
    if (chipIDx==0 && chipIDy==0 && leadAp)
    {
      t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
      io_printf(s, "Time: %ss. Trial: %d", ftoa(t,1), i+1);
      send_msg(s);
    }

    if (coreID>=1 && coreID<=DECODE_ST_SIZE)
    {
      io_printf(IO_DEF, "Trial: %d\n", i+1);
      
      //All chips
      info_tx->trial_num = i+1;
      info_tx->progress[coreID-1] = 0;
      finish[coreID-1] = 0;
      
      /*******************************************************/
      /* Encode array                                        */
      /*******************************************************/
      io_printf(IO_DEF, "Encoding...\n");
      encode();

      /*******************************************************/
      /* Decode array (locally)                              */
      /*******************************************************/
      io_printf(IO_DEF, "\nDecoding...\n");
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
        io_printf(IO_DEF, "Ticks: %d\n", spin1_get_simulation_time());

        // *io_buf Points to SDRAM buffer
        vbase = (int)sv->vcpu_base;
        //size  = (int)(sv->iobuf_size);
        vsize = sizeof(*sv->vcpu_base);

        vcpu  = (vcpu_t *)(vbase + vsize*coreID);
        iobuf = (iobuf_t *)vcpu->iobuf;
        
        t1 = spin1_get_simulation_time();

/*
        int s_pos, len;
        char *s, s1[100];

        // Send 2 messages per core
        for(i=0; i<2; i++)
        {
          strcpy(s1, "ChipNum: ");
          strcat(s1, itoa(chipNum));
          strcat(s1, " CoreID: ");
          strcat(s1, itoa(coreID));
          send_msg(s1, s_len);
//          spin1_delay_us(100);
        }
*/

/*
        while (iobuf!=NULL)
        {
          s     = iobuf->buf;
          s_len = iobuf->ptr;
          s_pos = 0;
          i=0;
          while(i<s_len)
          {
            s_pos = i;
            while(s[i++]!='\n' && i<s_len);
            if (s[i-1]=='\n')
              len = i-s_pos-1;
            else
              len = i-s_pos;

            send_msg(s+s_pos, len);
            spin1_delay_us(150);
          }
          iobuf = iobuf->next;
        }
*/        
        t_e = spin1_get_simulation_time() - t1;
        io_printf(IO_DEF, "t_e (ticks) = %d\n", t_e);
      }
      ////////////////////////////////////////

      finish[coreID-1] = 1;

#ifdef TX_PACKETS
      for(j=0; j<TX_REPS; j++)
      {
        io_printf(IO_DEF, "Transmitting packets (Rep %d) ...\n", j+1);

        if (chipIDx==0 && chipIDy==0 && leadAp)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "Time: %ss. Transmitting packets (rep %d) ...", ftoa(t,1), j+1);
          send_msg(s);
        }
        tx_packets(j);

        // Transmit packets TX_REPS times
        decode_done = 0;
        io_printf(IO_DEF, "Waiting for decode to finish\n");
        if (chipID==1 && coreID==1)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "Time: %ss. Waiting for decode to finish", ftoa(t,1));
          send_msg(s);
        }

        // Wait for decode_done signal
        while(!decode_done);
        io_printf(IO_DEF, "Decode done received\n");
        if (chipID==1 && coreID==1)
        {
          t = (float)spin1_get_simulation_time()*TIMER_TICK_PERIOD/1000000;
          io_printf(s, "Time: %ss. Decode done received", ftoa(t,1));
          send_msg(s);
        }

        decode_done = 0;
      } 
#endif

    }
  }
}


