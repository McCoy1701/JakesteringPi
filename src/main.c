#include <stdio.h>

#include "jakestering.h"

void test_handler(void);

int main( int argc, char *argv[] )
{
  setupIO();

  pinMode( 25, INPUT );

  jakestering_ISR( 25, BOTH_EDGE, &test_handler );
  
  while (1)
  {
    delay( 1000 );
  }
  return 0;
}

void test_handler(void)
{
  printf("Had egde trigger\n");
}

