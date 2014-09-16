/*
  GLCD_Pins.h - PIC library support for ks0108 and compatible graphic LCDs
  Ported to PIC by Ivan de Jesus Deras ideras@gmail.com
  Original library by Michael Margolis All right reserved

  This is the configuration file for mapping PIC pins to the ks0108 Graphics LCD library
*/

#ifndef	GLCD_PINS_H
#define GLCD_PINS_H

#ifdef _PIC18

#define CSEL1	LATBbits.LATB0		// CS1 Bit   // swap pin assignments with CSEL2 if left/right image is reversed
#define CSEL2	LATBbits.LATB1		// CS2 Bit

#define R_W	LATBbits.LATB3		// R/W Bit
#define D_I	LATBbits.LATB2		// D/I Bit
#define EN	LATBbits.LATB4		// EN Bit
#define RST 	LATBbits.LATB5		// Reset Bit

#define GLCD_DOUT_REG   LATD
#define GLCD_DIN_REG    PORTD
#define GLCD_DDIR_REG   TRISD

#elif defined (__PICC__)

#define CSEL1	PORTBbits.RB0		// CS1 Bit   // swap pin assignments with CSEL2 if left/right image is reversed
#define CSEL2	PORTBbits.RB1		// CS2 Bit

#define R_W	PORTBbits.RB3		// R/W Bit
#define D_I	PORTBbits.RB2		// D/I Bit
#define EN	PORTBbits.RB4		// EN Bit
#define RST 	PORTBbits.RB5		// Reset Bit

#define GLCD_DOUT_REG   PORTD
#define GLCD_DIN_REG    PORTD
#define GLCD_DDIR_REG   TRISD

#else
#error "Please define GLCD pin mapping for your platform."
#endif

// macros to fast write data to pins known at compile time
#define fastWriteHigh(_pin_)	_pin_ = 1
#define fastWriteLow(_pin_) 	_pin_ = 0

#endif
