/*
 * jakestering.h:
 *  GPIO access library for raspberry pi zero v1.3
 *
 * Copyright (c) 2023 Jacob Kellum <jkellum819@gmail.com>
 *************************************************************************
 * This file is apart of Jakestering:
 *    https://github.com/McCoy1701/Jakestering
 *
 * Jakestering is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Jakestering is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Jakestering; if not, see <http://www.gnu.org/licenses/>.
 * ***********************************************************************
 */

#ifndef __JAKESTERING_H__
#define __JAKESTERING_H__

#define BCM2835_BASE 0x20000000
#define GPIO_BASE ( BCM2835_BASE + 0x200000 )

#define PAGE_SIZE  ( 4 * 1024 )
#define BLOCK_SIZE ( 4 * 1024 )

#define INP_GPIO( g ) *( gpio + ( ( g ) / 10 ) ) &= ~( 7 << ( ( (g) % 10 ) * 3 ) )
#define OUT_GPIO( g ) *( gpio + ( ( g ) / 10 ) ) |=  ( 1 << ( ( (g) % 10 ) * 3 ) )
#define SET_GPIO_ALT( g, a ) *( gpio + ( ( ( g ) / 10 ) ) ) |= ( ( (a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << ( ( (g) % 10 ) * 3 ) )

#define GPIO_SET *( gpio + 7 )
#define GPIO_CLR *( gpio + 10 )

#define GPIO_LEV( g ) ( *( gpio + 13 ) & ( 1 << g ) )

#define GPIO_EDS  *( gpio + 16 )
#define GPIO_REN  *( gpio + 19 )
#define GPIO_FEN  *( gpio + 22 )
#define GPIO_HEN  *( gpio + 25 )
#define GPIO_LEN  *( gpio + 28 )
#define GPIO_AREN *( gpio + 31 )
#define GPIO_AFEN *( gpio + 34 )

#define GPIO_PULL *( gpio + 37 )
#define GPIO_PULLCLK0 *( gpio + 38 )

#define HIGH 1
#define LOW  0

#define INPUT  0
#define OUTPUT 1

#define PUD_DISABLE   0
#define PUD_DOWN      1
#define PUD_UP        2

#define RISING_EDGE        0
#define FALLING_EDGE       1
#define BOTH_EDGE          2
#define HIGH_DETECT        3
#define LOW_DETECT         4
#define ASYNC_RISING_EDGE  5
#define ASYNC_FALLING_EDGE 6

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MAX_BUFFER 32 

extern int memFd; // /dev/mem

extern void* gpioMap; //Pointer to the GPIO memory map

extern volatile unsigned *gpio; //Access to the input / output

void setupIO(); //Setup function to create memory regions to access the GPIO

void delay(int milliSeconds);

void delayMicro(int microSeconds);

void pinMode( const int pin, const int mode );

void pudController( const int pin, const int PUD );

void digitalWrite( const int pin, const int value );

int digitalRead( const int pin );

void digitalWriteByte( const int value, const int pinStart, const int pinEnd );

static void *interrupt_handler(void *arg);

int wait_for_interrupt( const int pin, const int ms );
int wait_for_interrupt_to_close( const int pin );
int interrupt_init( const int pin, const int mode );
int jakestering_ISR( const int pin, const int mode, void (*function)(void) );

int piHiPri (const int pri);
#endif

