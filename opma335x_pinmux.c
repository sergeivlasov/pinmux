/*
 * opma335x_pinmux.c
 * 
 * An opma335x connector pins table
 * 
 */

#if defined (__cplusplus)
extern "C" {
#endif

// Notes: 0xFF = no this pin.
//        0xBB table data means "No this GPIO pinned to connector".
//        Other values denote SODIMM200 pin numbers.
const unsigned char
opma335x_pinmux_table[128] = {    // Table_data = SODIMM200 pin number.
 // GPIO0
 0xBB,0xBB,  53,  51,   67,  65, 161,  72, 198,   55,  52,   54,   93,  95,   89,  91,
   61,  98,   3,  33,   35,0xBB,  76,  78,0xFF, 0xFF, 127,  153, 0xBB,  39, 162, 166,
 // GPIO1
 0xBB,0xBB,0xBB,0xBB, 0xBB,0xBB,0xBB,0xBB, 110,  112,  92,   94,  142, 144,  118,  59,
  164, 148,  68,  84,  189, 187, 123, 125,  86,   88, 195,  193,  163,0xBB,  128, 132,
 //16   17   18   19    20   21   22   23   24    25   26    27    28   29    30   31
 // GPIO2
 // 0    1    2    3     4    5    6    7    8     9   10    11    12   13    14   15
  134, 136,0xBB,0xBB, 0xBB,0xBB,  22,  24,  28,   30,  34,   36,   38,  40,   42,  46,
   48,  50, 124, 126, 0xBB,0xBB, 100, 114, 199,   41,  83,   85,   81,  87,  138, 140,
 // GPIO3
  160,0xBB,0xBB,0xBB,  194,  47,  45, 115, 119,   82, 191, 0xFF, 0xFF,   7,   64,  66,
  146,  74, 150, 152,  154, 156,0xFF,0xFF,0xFF, 0xFF,0xFF, 0xFF, 0xFF,0xFF, 0xFF,0xFF,
}; // opma335x_pinmux_table


const unsigned char
opma335x_pwr_domain_table[128] = {    // Table_data = 33==3v3, 18==1v8 domain, 0 = unknown
 // GPIO0
 0xBB, 0xBB, 0xBB, 0xBB,    33,   33,   33,   33,    33,   33,   33,   33,  0xBB, 0xBB, 0xBB, 0xBB,
   33,   33,   33,   33,    33, 0xBB,   18,   18,  0xFF, 0xFF,   18,   18,    33,   33,   33,   33,
 // GPIO1 ----------------------- 5/21--------------------------10/26--------12/28-------14/30-----
 0xBB, 0xBB, 0xBB, 0xBB,  0xBB, 0xBB, 0xBB, 0xBB,    33,   33,   33,   33,    18,   18,   18,   18,
   33,   33,   33,   33,    33,   33,   33,   33,    33,   33,   33,   33,    33, 0xBB,   18,   18,
 // GPIO2
   18,   18, 0xBB, 0xBB,  0xBB, 0xBB,   33,   33,    33,   33,   33,   33,    33,   33,   33,   33,
   33,   33,   33,   33,  0xBB, 0xBB,   33,   33,    33,   33,   33,   33,    33,   33,   33,   33,
 // GPIO3
   33, 0xBB, 0xBB, 0xBB,    33,   33,   33,   33,    33,   33,   33, 0xFF,  0xFF,   33,   33,   33,
   33,   33,   33,   33,    33,   33, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,  0xFF, 0xFF, 0xFF, 0xFF,
}; // opma335x_pwr_domain_table

//-------------------------- Sitara pinmux register format ---------------------------------//
//  31..07  |      6       |       5       |         4        |      3      |      2..0     //
//----------|--------------|---------------|------------------|-----------------------------//
// reserved | slew: 0=fast | RX: 1=enabled | putype: 1=pullup | puen: 0=en  | mcode: 7=GPIO //
//------------------------------------------------------------------------------------------//

#if defined (__cplusplus)
}
#endif
//
// opma335x_pinmux.c
//-----------------------------------------------------------------------------
