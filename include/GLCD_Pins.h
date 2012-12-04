/*
  GLCD_Pins.h - PIC library support for ks0108 and compatible graphic LCDs
  Ported to PIC by Ivan de Jesus Deras ideras@gmail.com
  Original library by Michael Margolis All right reserved

  This is the configuration file for mapping PIC pins to the ks0108 Graphics LCD library
*/

#ifndef	GLCD_CONFIG_H
#define GLCD_CONFIG_H

/*********************************************************/
/*  Configuration for assigning LCD bits to PIC Pins */
/*********************************************************/
/* PIC pins used for Commands
 * This assignment is based on EasyPIC5
 */

#define CSEL1	PORTBbits.RB1		// CS1 Bit   // swap pin assignments with CSEL2 if left/right image is reversed
#define CSEL2	PORTBbits.RB0		// CS2 Bit

#define R_W	PORTBbits.RB3		// R/W Bit
#define D_I	PORTBbits.RB2		// D/I Bit 
#define EN	PORTBbits.RB4		// EN Bit
#define RST 	PORTBbits.RB5		// Reset Bit 

/*******************************************************/
/*     end of Pins configuration                       */
/*******************************************************/

#define LCD_DATA_LOW_NBL        D
#define LCD_DATA_HIGH_NBL       D

// macros to fast write data to pins known at compile time
#define fastWriteHigh(_pin_)	_pin_ = 1
#define fastWriteLow(_pin_) 	_pin_ = 0

#endif
