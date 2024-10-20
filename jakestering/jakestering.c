/*
 * jakestering.c:
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include <sched.h>

#include "jakestering.h"

int memFd;
void* gpioMap;
volatile unsigned *gpio;

static volatile int pin_pass = -1;
static pthread_mutex_t pin_mutex;

static int chip_FD = -1;
static int pin_FDs[32] = {-1};
static void(*isr_functions[32])(void);
static pthread_t isr_threads[32];
static int isr_modes[32];
static int isr_function_called[32] = {1};

/*
 * Sets up the GPIO memory address space to be modified
 *
 * Parameters:
 *  void
 * 
 * Return:
 *  void
 **************************************************************
 */

void setupIO( void )
{
  if ( ( memFd = open ( "/dev/mem", O_RDWR|O_SYNC ) ) < 0)
  {
    printf( "can't open /dev/mem \n" );
    exit( -1 );
  }

  gpioMap = mmap( NULL, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, memFd, GPIO_BASE );

  close( memFd );

  if ( gpioMap == MAP_FAILED )
  {
    printf( "mmap error %d\n", (int)gpioMap );
    exit( -1 );
  }

  gpio = ( volatile unsigned* )gpioMap;
}

/*
 * Delay in milli seconds, but if milliSecond is higer than 1000 delay in seconds
 *
 * Parameters:
 *  milliSeconds: amount of time to delay
 * 
 * Return:
 *  void
 **************************************************************
 */

void delay( int milliSeconds )
{
  if ( ( milliSeconds % 1000 ) == 0 )
  {
    sleep(milliSeconds / 1000);
  }
  
  else 
  {
    usleep( ( milliSeconds % 1000 ) * 1000 );
  }
}

/*
 * Delay in micro seconds
 *
 * Parameters:
 *  microSeconds: amount of time to delay
 * 
 * Return:
 *  void
 **************************************************************
 */

void delayMicro( int microSeconds )
{
  usleep( microSeconds );
}

/*
 * Pin mode sets the INPUT/OUTPUT state of a pin 
 *
 * Parameters:
 *  pin : GPIO pin to set
 *  mode: INPUT/OUTPUT 0/1
 * 
 * Return:
 *  void
 **************************************************************
 */

void pinMode( const int pin, const int mode )
{
  if ( mode == INPUT )
  {
    INP_GPIO( pin );
  }

  else if ( mode == OUTPUT )
  {
    INP_GPIO( pin );
    OUT_GPIO( pin );
  }
}

/*
 * Pull up/down controller for the built in resistors on each GPIO pin
 *
 * Parameters:
 *  pin: GPIO pin to set
 *  PUD: DISABLE/PULL_DOWN/PULL_UP 0/1/2
 * 
 * Return:
 *  void
 **************************************************************
 */

void pudController( const int pin, const int PUD )
{
  GPIO_PULL = PUD & 0b11;
  usleep( 5 );
  GPIO_PULLCLK0 = 1 << pin;
  usleep( 5 );

  GPIO_PULL = 0;
  GPIO_PULLCLK0 = 0;
}

/*
 * write a value to a given pin
 *
 * Parameters:
 *  pin  : GPIO pin to set
 *  value: LOW/HIGH 0/1 
 * 
 * Return:
 *  void
 **************************************************************
 */

void digitalWrite( const int pin, const int value )
{
  if ( value == LOW )
  {
    GPIO_CLR = 1 << pin;
  }

  else if ( value == HIGH )
  {
    GPIO_SET = 1 << pin;
  }
}

/*
 * read the value of a given pin
 *
 * Parameters:
 *  pin: GPIO pin to set
 * 
 * Return:
 *  value of the pin read (HIGH/LOW 1/0)
 **************************************************************
 */

int digitalRead( const int pin )
{
  if ( GPIO_LEV( pin ))
    return HIGH;
  else
    return LOW;
}

/*
 * Write a byte to given range
 *
 * Parameters:
 *  value   : data to be writen
 *  pinStart: start of pin range
 *  pinEnd  : end of pin range
 *
 * Return:
 *  void
 **************************************************************
 */

void digitalWriteByte( const int value, const int pinStart, const int pinEnd ) //pin start must be less than pinEnd
{
  uint32_t pinSet = 0;
  uint32_t pinClr = 0;
  int mask = 1;
  
  if ( ( pinEnd - pinStart ) != 7 )
  {
    printf( "Must be 8 pins 0-7: %d\n", ( pinEnd - pinStart ) );
    return;
  }
  
  for ( int i = pinStart; i <= pinEnd; i++ ) //hardcode pins 3 - 10 to be the data bus
  {
    if ( ( value & mask ) == 0 )
    {
      pinClr |= ( 1 << i ); //set the pin to be cleared
    }

    else 
    {
      pinSet |= ( 1 << i ); //set the pin the be set
    }

    mask <<= 1;
  }

  GPIO_CLR = pinClr;
  GPIO_SET = pinSet;
}

static void *interrupt_handler(void *arg)
{
  int pin;
  
  (void)piHiPri(55);

  pin = pin_pass;
  pin_pass = -1;

  for (;;)
  {
    int ret = wait_for_interrupt( pin, -1 );
    if ( ret > 0 )
    {
      if ( isr_functions[pin] )
      {
        if ( isr_function_called[pin] )
        {
          isr_functions[pin]();
          isr_function_called[pin] = 0;
        }
      }
    }

    else if ( ret < 0 )
    {
      break;
    }
  }

  wait_for_interrupt_to_close( pin );

  return NULL;
}

int wait_for_interrupt( const int pin, const int ms )
{
  int fd, ret;
  struct pollfd polls;
  struct gpioevent_data event_data;

  if ( ( fd = pin_FDs[pin] ) == -1 )
  {
    return -2;
  }

  polls.fd = fd;
  polls.events = POLLIN | POLLERR;
  polls.revents = 0;

  ret = poll( &polls, 1, ms );
  if ( ret <= 0 )
  {
  //  printf( "Failed: poll returned %d\n", ret );
  }
  else
  {
    int readret = read( pin_FDs[pin], &event_data, sizeof(event_data) );
    
    if ( readret == sizeof(event_data) )
    {
      ret = event_data.id;
      if ( ret == 1 )
      {
        isr_function_called[pin] = 1;
      }
    }

    else
    {
      ret = 0;
    }
  }

  return ret;
}

int wait_for_interrupt_to_close( const int pin )
{
  if ( pin_FDs[pin] > 0 )
  {
    if ( pthread_cancel( isr_threads[pin] ) == 0 )
    {
      //printf("thread cnacnled successfully\n")
    }

    else
    {
      printf( "Failed: could not cancel thread\n" );
    }
    close( pin_FDs[pin] );
  }
  pin_FDs[pin] = -1;
  isr_functions[pin] = NULL;
  isr_function_called[pin] = 1;

  return 0;
}

int interrupt_init( const int pin, const int mode )
{
  const char* strmode = "";
  sleep(1);
  const char *gpio_chip = "/dev/gpiochip0";

  if ( chip_FD < 0 )
  {
    chip_FD = open( gpio_chip, O_RDWR );
    if ( chip_FD < 0 )
    {
      printf( "Error opening: %s open ret=%d\n", gpio_chip, chip_FD );
      return -1;
    }
  }

  struct gpioevent_request req;
  req.lineoffset = pin;
  req.handleflags = GPIOHANDLE_REQUEST_INPUT;
  switch( mode )
  {
    case FALLING_EDGE:
      req.eventflags = GPIOEVENT_REQUEST_FALLING_EDGE;
      strmode = "falling";
      break;
    case RISING_EDGE:
      req.eventflags = GPIOEVENT_REQUEST_RISING_EDGE;
      strmode = "rising";
      break;
    case BOTH_EDGE:
      req.eventflags = GPIOEVENT_REQUEST_BOTH_EDGES;
      strmode = "both";
      break;
  }
  strncpy( req.consumer_label, "jakestering_gpio_irq", sizeof( req.consumer_label ) - 1 );

  int ret = ioctl( chip_FD, GPIO_GET_LINEEVENT_IOCTL, &req );
  if ( ret )
  {
    printf("Failed: %s ioctl get line %d %s returned %d\n", gpio_chip, pin, strmode, ret );
    return -1;
  }

  int fd_line = req.fd;
  pin_FDs[pin] = fd_line;
  int flags = fcntl( fd_line, F_GETFL );
  flags |= O_NONBLOCK;
  ret = fcntl( fd_line, F_SETFL, flags );

  if (ret)
  {
    printf( "Failed: %s fcntl set nonblock read=%d\n", gpio_chip, chip_FD );
    return -1;
  }

  return 0;
}

int jakestering_ISR( const int pin, const int mode, void (*function)(void) )
{
  isr_functions[pin] = function;
  isr_modes[pin] = mode;
  isr_function_called[pin] = 1;

  if ( interrupt_init( pin, mode ) < 0 )
  {
    printf("Waiting for interrupt init failed\n");
  }
  
  pthread_mutex_lock( &pin_mutex );
    pin_pass = pin;

    if ( pthread_create( &isr_threads[pin], NULL, interrupt_handler, NULL ) == 0 )
    {
      while ( pin_pass != -1 )
      {
        delay(1);
      }
    }

    else
    {
      printf("Failed to create thread\n");
    }
  pthread_mutex_unlock( &pin_mutex );

  return 0;
}

int piHiPri (const int pri)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  if (pri > sched_get_priority_max (SCHED_RR))
    sched.sched_priority = sched_get_priority_max (SCHED_RR) ;
  else
    sched.sched_priority = pri ;

  return sched_setscheduler (0, SCHED_RR, &sched) ;
}


