/*
 * lcd128x64.c:
 *  Interface for ST7920 lcd driver
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

#include "jakestering.h"
#include "lcd128x64.h"

static const int rowsOffset[4] = { 0b00000000, 0b01000000, 0b00010100, 0b01010100 }; //0x00, 0x40, 0x14, 0x54

/*
 * Pulse the enable line 
 *
 * Parameters:
 *  lcd: which lcd to enable
 *
 * Return:
 *  void
 **************************************************************
 */

void pulseEnable( LCD128 *lcd )
{
  digitalWrite( lcd->E, HIGH );
  delayMicro( 1 );
  digitalWrite( lcd->E, LOW );
  delayMicro( 5 );
}

/*
 * Send data to the lcd
 *
 * Parameters:
 *  lcd : which lcd to receive the data
 *  data: data to be sent
 *
 * Return:
 *  void
 **************************************************************
 */

void sendData( LCD128 *lcd, const int data )
{
  digitalWriteByte( data, lcd->DB0, lcd->DB7 );
  pulseEnable( lcd );
  delay( 2 );
}

/*
 * Send instruction to the lcd
 *
 * Parameters:
 *  lcd        : which lcd to receive the instruction
 *  instruction: instruction to be sent 
 *
 * Return:
 *  void
 **************************************************************
 */

void sendInstruction( LCD128 *lcd, const int instruction )
{
  digitalWrite( lcd->RS, LOW  );
  sendData( lcd, instruction );
  digitalWrite( lcd->RS, HIGH );
}

/*
 * Move the cursor
 *
 * Parameters:
 *  lcd: holds cursor info
 *  x  : move horizontally this amount
 *  y  : move vertically this amount
 *
 * Return:
 *  void
 **************************************************************
 */

void lcdTextPosition( LCD128 *lcd, int x, int y )
{
  if ( ( x > lcd->cols ) || ( x < 0 ) )
  {
    return;
  }

  if ( ( y > lcd->rows ) || ( y < 0 ) )
  {
    return;
  }

  sendInstruction( lcd, x + ( LCD128_DDRAM_SET | rowsOffset[y] ) );

  lcd->cx = x;
  lcd->cy = y;
}

/* 
 * Print a char to the lcd
 *
 * Parameters:
 *  lcd      : lcd to print to  
 *  character: char
 *  
 * Return:
 *  void
 **************************************************************
 */

void lcdPutChar( LCD *lcd, unsigned char character )
{
  sendData( lcd, character );
  
  if ( ++lcd->cx == lcd->cols )
  {
    lcd->cx = 0;
    
    if ( ++lcd->cy == lcd->rows )
    {
      lcd->cy = 0;
    }

    lcdTextPosition( lcd, lcd->cx, lcd->cy );
  }
}

/* 
 * Print a string to the lcd
 *
 * Parameters:
 *  lcd   : lcd to print to  
 *  string: string
 *  
 * Return:
 *  void
 **************************************************************
 */

void lcdPuts( LCD *lcd, const char* string )
{
  while ( *string )
  {
    lcdPutChar( lcd, *string++ );
  }
}

/* 
 * Print a formated string to the lcd
 *
 * Parameters:
 *  lcd   : lcd to print to  
 *  string: formated string
 *  
 * Return:
 *  void
 **************************************************************
 */

void lcdPrintf( LCD *lcd, const char *string, ... )
{
  char buffer[ 1024 ];
  va_list args;

  va_start( args, string );
  vsnprintf( buffer, sizeof( buffer ), string, args );
  va_end( args );

  lcdPuts( lcd, buffer );
}

/*
 * Set lcd to Text mode
 *
 * Parameters:
 *  lcd: to set text mode
 *
 * Return:
 *  void
 **************************************************************
 */

void setTextMode( LCD128 *lcd )
{
  sendInstruction( lcd, LCD128_FUNCTION_SET | LCD128_DL_FUNCTION );
  lcdClear( lcd );
}

/*
 * Set lcd to graphics mode
 *
 * Parameters:
 *  lcd: to set graphics mode
 *
 * Return:
 *  void
 **************************************************************
 */

void setGraphicsMode( LCD128 *lcd )
{
  sendInstruction( lcd, LCD128_FUNCTION_SET | LCD128_DL_FUNCTION | LCD128_RE_FUNCTION );
  sendInstruction( lcd, LCD128_FUNCTION_SET | LCD128_DL_FUNCTION | LCD128_RE_FUNCTION | LCD128_G_FUNCTION );
  delay( 5 );
}

/*
 * Clear lcd
 *
 * Parameters:
 *  lcd: to clear
 *
 * Return:
 *  void
 **************************************************************
 */

void lcdClear(LCD128 *lcd )
{
  sendInstruction( lcd, LCD128_DISPLAY_CLEAR );
  sendInstruction( lcd, LCD128_RETURN_HOME );
  lcd->cx = 0;
  lcd->cy = 0;
  delay( 5 );
}

/*
 * Ruturn cursor to the home position ( 0,0 )
 *
 * Parameters:
 *  lcd: to update
 *
 * Return:
 *  void
 **************************************************************
 */

void lcdReturnHome( LCD128 *lcd )
{
  sendInstruction( lcd, LCD128_RETURN_HOME );
  lcd->cx = 0;
  lcd->cy = 0;
  delay( 5 );
}


/*
 * Set the pixel at x, y
 *
 * Parameters:
 *  lcd: holds the pixel info
 *  x  : horizontal position
 *  y  : vertical position
 *
 * Return:
 *  void
 **************************************************************
 */

void lcdGraphicsPosition( LCD128 *lcd, int x, int y )
{

}

/*
 * Update the lcd with contents of buffer
 *
 * Parameters:
 *  lcd   : holds the cursor info
 *  buffer: 128 x 64 array with the new contents of the screen
 *
 * Return:
 *  void
 **************************************************************
 */

void lcdUpdateScreen( LCD128 *lcd, unsigned int buffer[128][64] )
{

}

/*
 * Initialize the lcd
 *
 * Parameters:
 *  RS   : register select
 *  RW   : read/write
 *  E    : enable
 *  DB0-7: data lines
 *  PSB  : interface selection 1 for 8-bit parallel
 *  RST  : reset
 *
 * Return:
 *  LCD128 that has been initialized
 **************************************************************
 */

LCD128 *initLcd( int RS, int RW, int E, int DB0, int DB1, int DB2, int DB3, int DB4, int DB5, int DB6, int DB7, int PSB, int RST )
{
  LCD128 *lcd = ( LCD128x64* )malloc( sizeof( LCD128 ) );

  lcd->RS  =  RS;
  lcd->RW  =  RW;
  lcd->E   =   E;
  lcd->DB0 = DB0;
  lcd->DB1 = DB1;
  lcd->DB2 = DB2;
  lcd->DB3 = DB3;
  lcd->DB4 = DB4;
  lcd->DB5 = DB5;
  lcd->DB6 = DB6;
  lcd->DB7 = DB7;
  lcd->PSB = PSB;
  lcd->RST = RST;

  lcd->cx = 0;
  lcd->cy = 0;

  lcd->screen[128][64] = { 0 };

  pinMode( lcd->RS , OUTPUT );
  pinMode( lcd->RW , OUTPUT );
  pinMode( lcd->E  , OUTPUT );
  pinMode( lcd->DB0, OUTPUT );
  pinMode( lcd->DB1, OUTPUT );
  pinMode( lcd->DB2, OUTPUT );
  pinMode( lcd->DB3, OUTPUT );
  pinMode( lcd->DB4, OUTPUT );
  pinMode( lcd->DB5, OUTPUT );
  pinMode( lcd->DB6, OUTPUT );
  pinMode( lcd->DB7, OUTPUT );
  pinMode( lcd->PSB, OUTPUT );
  pinMode( lcd->RST, OUTPUT );

  digitalWrite( lcd->RS , HIGH );
  digitalWrite( lcd->RW , LOW  );
  digitalWrite( lcd->E  , LOW  );
  digitalWrite( lcd->PSB, HIGH );
  digitalWrite( lcd->RST, LOW  );

  

  return lcd;
}
