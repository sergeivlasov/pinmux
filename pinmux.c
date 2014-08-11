
/*
 * pinmux.c: Simple program to show Sitara pinmux settings.
 *
 *  Copyright (C) 2014
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#define CLIENT_REVISION_NUMBER (9)

// will try creating patch
// TODO:
// 1. 
// 2. 
// 3. activity test                 -- Rx must be enabled!
// 4. drive probe (inputs, outputs) -- Rx must be enabled!


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "pinmux.h"
#include "mux33xx.h"

int
filter(int i, int access_type, int read_result)
{
    int pinmux_index;

    pinmux_index = am335x_pinmux_table[i];
    if(pinmux_index == 0xFF) return 0;  // no such CPU pin

    if(0 != (access_type & ACCESS_FLAG_ALL)) return 1;  // force all
    if(0 != (access_type & ACCESS_FLAG_GPIO)){          // filter out a "not GPIO"
        if((read_result & 0x07) != 0x07)
            return 0;
        if(0 != (access_type & ACCESS_FLAG_RECEIVER)){
            if((read_result & 0x20) == 0x00)
                return 0;
        }
        if(0 != (access_type & ACCESS_FLAG_PULLUP)){
            if((read_result & 0x18) != 0x10)
                return 0;
        }
        if(0 != (access_type & ACCESS_FLAG_PULLDOWN)){
            if((read_result & 0x18) != 0x00)
                return 0;
        }
    }
    if(0 != (access_type & ACCESS_FLAG_NOT_GPIO)){
        if((read_result & 0x07) == 0x07)
            return 0;
    }

    if(0 != (access_type & ACCESS_FLAG_BBB)){
        if(bbb_pinmux_table[i] == 0xFF) return 0;   // not exist
        if(bbb_pinmux_table[i] == 0xBB) return 0;   // not pinned
    }

    if(0 != (access_type & ACCESS_FLAG_OPMA335X)){
        if(opma335x_pinmux_table[i] == 0xFF) return 0;
        if(opma335x_pinmux_table[i] == 0xBB) return 0;   // not pinned
    }

    if(0 != (access_type & ACCESS_FLAG_GPIO0)){
        if(0 == i/32) return 1;
    }

    if(0 != (access_type & ACCESS_FLAG_GPIO1)){
        if(1 == i/32) return 1;
    }

    if(0 != (access_type & ACCESS_FLAG_GPIO2)){
        if(2 == i/32) return 1;
    }

    if(0 != (access_type & ACCESS_FLAG_GPIO3)){
        if(3 == i/32) return 1;
    }
    return 0; // do not print.
} // filter

void
print_pin_cfg(int i, int read_result)
{
    if(0xFF != am335x_pinmux_table[i])
        printf("%s ", am33xx_muxmodes[am335x_pinmux_table[i]].muxnames[read_result&0x07]);
} // print_pin_cfg

void print_gpio(int read_result)
{
    if(read_result & 0x40) printf("Slow ");
    else                   printf("Fast ");

    if(read_result & 0x20) printf("isRX ");
    if(0 == (read_result & 0x08)){
        if(read_result & 0x10) printf("Pull-up ");
        else                   printf("Pull-down ");
    }
} // print_gpio

void
print_gpio_state(int i, int fd)
{
    int target, read_result;
    void *map_base, *virt_addr;

    if( 0 == (i & 0x60) ) target = GPIO0;
    else if( 0x20 == (i & 0x60) ) target = GPIO1;
    else if( 0x40 == (i & 0x60) ) target = GPIO2;
    else if( 0x60 == (i & 0x60) ) target = GPIO3;
    else return;

    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
    if(map_base == (void *) -1){
        printf("map? ");
        return;
    }
    virt_addr = map_base + ((target + GPIO_DATAIN) & MAP_MASK);
    read_result = *((unsigned long *) virt_addr);
    read_result &= (1 << (i & 0x1F));

//    printf("state=0x%08X ", read_result);
    if(0 == read_result) printf("Low ");
    if(0 != read_result) printf("High ");

    munmap(map_base, MAP_SIZE);
} // print_gpio_state

void
print_bbb_pin(int i)
{
    int slot = (bbb_pinmux_table[i] >> 7) +8;
    int pin = bbb_pinmux_table[i] & 0x3F;
    printf("bbb.P%d.%d ", slot, pin);
} // print_bbb_pin

void
print_opma335x_pin(int i)
{
    printf("opma.%d (%dv%d) ", opma335x_pinmux_table[i], opma335x_pwr_domain_table[i]/10, opma335x_pwr_domain_table[i]%10);
} // print_opma335x_pin


int
main(int argc, char **argv) {
    int fd;
    void *map_base, *virt_addr;
    unsigned long read_result;
    off_t target;
    int access_type = 0;
    int verbose, i, j;
    verbose = 0;

    for(i=1; i<argc; i++){
        for(j=0; j<strlen(argv[i]); j++){
            switch(tolower(argv[i][j])){
                case '0': access_type |= ACCESS_FLAG_GPIO0;     break;
                case '1': access_type |= ACCESS_FLAG_GPIO1;     break;
                case '2': access_type |= ACCESS_FLAG_GPIO2;     break;
                case '3': access_type |= ACCESS_FLAG_GPIO3;     break;
                case 'a': access_type |= ACCESS_FLAG_ALL;       break;
                case 'b': access_type |= ACCESS_FLAG_BBB;       break;
                case 'd': access_type |= ACCESS_FLAG_PULLDOWN;  break;
                case 'g': access_type |= ACCESS_FLAG_GPIO;      break;
                case 'n': access_type |= ACCESS_FLAG_NOT_GPIO;  break;
                case 'o': access_type |= ACCESS_FLAG_OPMA335X;  break;
                case 'r': access_type |= ACCESS_FLAG_RECEIVER;  break;
                case 's': access_type |= ACCESS_FLAG_SENSE_GPIO;break;
                case 't': access_type |= ACCESS_FLAG_TEST_GPIO; break;
                case 'u': access_type |= ACCESS_FLAG_PULLUP;    break;
                case 'v': access_type |= ACCESS_FLAG_VERBOSE;   verbose += 1; break;
                default:  access_type |= ACCESS_FLAG_HELP;      break;
            } // switch
        } // for j
    } // for i
    if(0 == access_type) access_type |= ACCESS_FLAG_HELP;

    if(verbose > 0)
        printf("pinmux -- access_type=0x%08X.\n", access_type);

    if(0 != (access_type & ACCESS_FLAG_HELP)){
        fprintf(stderr, "\nPinmux utility rev %d.  Usage:\t%s {filter flags}\n"
            "\tfilter flags   : 0 1 2 3 a b d g n o r s t u v\n"
            "\t a=all  b=BBB  o=opma335x 0=gpio0 1=gpio1 2=gpio2 3=gpio3 g=GPIO  n=NotGPIO\n"
            "\t v=verbose   h=help  d=pulled down   u=pulled up    s=show level  t=test r=receiver\n\n"
            "\tValid examples : ./pinmux g0rvb  -- show GPIO-muxed BBB pins receiver enabled, verbouse print\n"
            "\t                 ./pinmux n23 -- show Sitara GPIO2,GPIO3 domain pins, pinmuxed not as a GPIO\n", CLIENT_REVISION_NUMBER,
            argv[0]);
        exit(1);
    } // if(access_type)

    if( (0 != (access_type & ACCESS_FLAG_GPIO)) && (0 != (access_type & ACCESS_FLAG_NOT_GPIO)) ){
        printf("pinmux -- ambiguous filter flags combination: g n.\n");
        exit(1);
    }
    if( (0 != (access_type & ACCESS_FLAG_SENSE_GPIO)) && (0 != (access_type & ACCESS_FLAG_NOT_GPIO)) ){
        printf("pinmux -- ambiguous filter flags combination: s n.\n");
        exit(1);
    }
    if( (0 != (access_type & ACCESS_FLAG_TEST_GPIO)) && (0 != (access_type & ACCESS_FLAG_NOT_GPIO)) ){
        printf("pinmux -- ambiguous filter flags combination: t n.\n");
        exit(1);
    }
    if( (0 != (access_type & ACCESS_FLAG_BBB)) && (0 != (access_type & ACCESS_FLAG_OPMA335X)) ){
        printf("pinmux -- ambiguous filter flags combination: b o.\n");
        exit(1);
    }

    if( (0 != (access_type & ACCESS_FLAG_ALL)) && (0 != (access_type & ACCESS_FLAG_OPMA335X)) ){
        printf("pinmux -- ambiguous filter flags combination: a o.\n");
        exit(1);
    }

    if( (0 != (access_type & ACCESS_FLAG_ALL)) && (0 != (access_type & ACCESS_FLAG_BBB)) ){
        printf("pinmux -- ambiguous filter flags combination: a b.\n");
        exit(1);
    }

    if(verbose > 1){
        printf("at=%c argc = %d\n", access_type, argc);
        if(argc>=1) printf("argv1 = %s\n", argv[0]);
        if(argc>=2) printf("argv2 = %s\n", argv[1]);
        if(argc>=3) printf("argv3 = %s\n", argv[2]);
        fflush(stdout);
    }

    if(0 == (access_type & // if no domain filter then all domains to be included
        (ACCESS_FLAG_GPIO0 | ACCESS_FLAG_GPIO1 | ACCESS_FLAG_GPIO2 | ACCESS_FLAG_GPIO3)) ){
        access_type |=
        (ACCESS_FLAG_GPIO0 | ACCESS_FLAG_GPIO1 | ACCESS_FLAG_GPIO2 | ACCESS_FLAG_GPIO3);
    }

    if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
    if(verbose > 0){
        printf("/dev/mem opened.\n");
        fflush(stdout);
    }
    target = PINMUX_BASE;

    /* Map one page */
    map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);

    if(map_base == (void *) -1) FATAL;
    if(verbose > 0){
        printf("Memory mapped at address %p.\n", map_base);
        fflush(stdout);
    }

    for( i = 0; i < 128; i++){ // loop through pinmux table
        virt_addr = map_base + ((target + (am335x_pinmux_table[i]*4)) & MAP_MASK);
        read_result = *((unsigned long *) virt_addr);

        if(filter(i, access_type, read_result)){
            printf("GPIO%d_%02d ", i/32, i%32);
            if(verbose > 0){
                printf("Pinmux 0x%02X ", (unsigned int)am335x_pinmux_table[i]);
                print_pin_cfg(i, 0);
            }
            printf( "Mux=0x%02X ", (unsigned int)read_result);
            if(((read_result & 0x07) == 0x07)) {
                print_gpio(read_result);
                if(access_type & ACCESS_FLAG_SENSE_GPIO)
                    print_gpio_state(i, fd);
            }else print_pin_cfg(i, read_result);

            if(ACCESS_FLAG_BBB & access_type) print_bbb_pin(i);
            if(ACCESS_FLAG_OPMA335X & access_type) print_opma335x_pin(i);

            printf( "\n");
            fflush(stdout);
        }
    }// for

    if(munmap(map_base, MAP_SIZE) == -1) FATAL;
    close(fd);
    return 0;
} // main
//
// pinmux.c
//-----------------------------------------------------------------------------

