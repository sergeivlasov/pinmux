/*
 * pinmux.h
 * 
 * Platform dependency definitions:
 *   the client code might use these GPIO to IPMB, and GPIO to LEDs assignments, 
 *   for driver initialization.
 * These are safe defaults, good for use on the Beaglebone Black, and initially 
 *   defined for ipmb_test sample code.
 * For other platforms/clients the GPIO map should be specified elsewhere (ini-file, h-file, etc) 
 * 
 */
#ifndef _PINMUX_H
#define _PINMUX_H

#if defined (__cplusplus)
extern "C" {
#endif

#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)

#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)
#define PINMUX_BASE 0x44e10800

#define GPIO0                           0x44E07100      // 0x100 offset
#define GPIO1                           0x4804C100      //
#define GPIO2                           0x481AC100      //
#define GPIO3                           0x481AE100      //
#define GPIO_CLEARDATAOUT               0x90            // GPIOx relative offset (+0x100)
#define GPIO_SETDATAOUT                 0x94            // GPIOx relative offset
#define GPIO_OE                         0x34            // GPIOx relative offset
#define GPIO_DATAIN                     0x38            // GPIOx relative offset
#define GPIO_DATAOUT                    0x3C            // GPIOx relative offset


#define USER_LED_0  53  // GPIO1_21 (BBB USR0)
#define USER_LED_1  54  // GPIO1_22 (BBB USR1)

#define ACCESS_FLAG_HELP        0x00001
#define ACCESS_FLAG_GPIO0       0x00002
#define ACCESS_FLAG_GPIO1       0x00004
#define ACCESS_FLAG_GPIO2       0x00008
#define ACCESS_FLAG_GPIO3       0x00010
#define ACCESS_FLAG_GPIO        0x00020
#define ACCESS_FLAG_ALL         0x00040
#define ACCESS_FLAG_OPMA335X    0x00080
#define ACCESS_FLAG_VERBOSE     0x00100
#define ACCESS_FLAG_PULLUP      0x00200
#define ACCESS_FLAG_PULLDOWN    0x00400
// #define ACCESS_FLAG_    0x00800
#define ACCESS_FLAG_BBB         0x01000
#define ACCESS_FLAG_NOT_GPIO    0x02000
#define ACCESS_FLAG_RECEIVER    0x04000
#define ACCESS_FLAG_TEST_GPIO   0x08000
#define ACCESS_FLAG_SENSE_GPIO  0x10000


extern const unsigned char am335x_pinmux_table[128];
extern const unsigned char opma335x_pinmux_table[128];
extern const unsigned char opma335x_pwr_domain_table[128];
extern const unsigned char bbb_pinmux_table[128];
// extern const unsigned char am335x_pinmux_signals[128][8][16];

#if defined (__cplusplus)
}
#endif
#endif // _PINMUX_H
//-----------------------------------------------------------------------------
