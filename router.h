/////////////////////////////
// Chip links related defines

// X/Y coordinate shifts
#define x1 (1<<8)
#define y1 1

// Mask defintion for router
#define MASK_ALL 0xffffffff

//Link aliases
#define NORTH  2
#define SOUTH  5
#define EAST   0
#define WEST   3
#define NEAST  1
#define SWEST  4

#define N_LINK    MC_LINK_ROUTE(NORTH)
#define S_LINK    MC_LINK_ROUTE(SOUTH)
#define E_LINK    MC_LINK_ROUTE(EAST)
#define W_LINK    MC_LINK_ROUTE(WEST)
#define NE_LINK   MC_LINK_ROUTE(NEAST)
#define SW_LINK   MC_LINK_ROUTE(SWEST)


//Core aliases
#define CORE1  MC_CORE_ROUTE(1)
#define CORE2  MC_CORE_ROUTE(2)
#define CORE3  MC_CORE_ROUTE(3)
#define CORE4  MC_CORE_ROUTE(4)
#define CORE5  MC_CORE_ROUTE(5)
#define CORE6  MC_CORE_ROUTE(6)
#define CORE7  MC_CORE_ROUTE(7)
#define CORE8  MC_CORE_ROUTE(8)
#define CORE9  MC_CORE_ROUTE(9)
#define CORE10 MC_CORE_ROUTE(10)
#define CORE11 MC_CORE_ROUTE(11)
#define CORE12 MC_CORE_ROUTE(12)
#define CORE13 MC_CORE_ROUTE(13)
#define CORE14 MC_CORE_ROUTE(14)
#define CORE15 MC_CORE_ROUTE(15)
#define CORE16 MC_CORE_ROUTE(16)

//Boundary definition for 48 chips board
//Bit 6: Link 5
//Bit 5: Link 4
//Bit 4: Link 3
//Bit 3: Link 2
//Bit 2: Link 1
//Bit 1: Link 0
//Bit 0: Chip exists
#ifdef MAP_7X7_48CHIPS
  #define XCHIPS 8
  #define YCHIPS 8
  uint chip_map[XCHIPS][YCHIPS] =
    {
      {0x0f, 0x1f, 0x1f, 0x1f, 0x1d, 0,    0,    0},
      {0x4f, 0x7f, 0x7f, 0x7f, 0x7f, 0x3d, 0,    0},
      {0x4f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x3d, 0},
      {0x47, 0x1f, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x39},
      {0,    0x67, 0x7f, 0x7f, 0x7f, 0x7f, 0x7f, 0x79},
      {0,    0,    0x67, 0x7f, 0x7f, 0x7f, 0x7f, 0x79},
      {0,    0,    0,    0x67, 0x7f, 0x7f, 0x7f, 0x79},
      {0,    0,    0,    0,    0x63, 0x73, 0x73, 0x71}
    };
#endif

// 3 board torus
#ifdef MAP_12X12_144CHIPS
  #define XCHIPS 12
  #define YCHIPS 12
//  uint chip_map[XCHIPS][YCHIPS] =
#endif

// 24 board torus
#ifdef MAP_34X34_1152CHIPS
  #define XCHIPS 34
  #define YCHIPS 34
//  uint chip_map[XCHIPS][YCHIPS] =
#endif

// Chip links related defines
/////////////////////////////
