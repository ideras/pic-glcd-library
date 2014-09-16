/*
  GLCD.h - PIC library support for ks0108 and compatible graphic LCDs
  Ported to PIC by Ivan de Jesus Deras ideras@gmail.com

  Original Arduino library by Michael Margolis All right reserved

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdint.h>

#ifndef	GLCD_H
#define GLCD_H

typedef uint8_t boolean;
typedef uint8_t byte;

#define GLCD_VERSION 2 // software version of this library

// Chip specific includes
#include "GLCD_Pins.h"
#include "GLCD_Panel.h"      // this contains LCD panel specific configuration

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// panel controller chips
#define CHIP_WIDTH     64  // pixels per chip

#define lcdDataOut(d)   GLCD_DOUT_REG = d
#define lcdDataDir(d)   GLCD_DDIR_REG = d

// Commands
#ifdef HD44102 
#define LCD_ON			0x39
#define LCD_OFF			0x38
#define LCD_DISP_START		0x3E   // Display start page 0
#else
#define LCD_ON			0x3F
#define LCD_OFF			0x3E
#define LCD_DISP_START		0xC0
#endif

#define LCD_SET_ADD		0x40
#define LCD_SET_PAGE		0xB8

#define LCD_BUSY_FLAG		0x80 

// Colors
#define BLACK			0xFF
#define WHITE			0x00

// useful user contants
#define NON_INVERTED 0
#define INVERTED     1

// Font Indices
#define FONT_LENGTH		0
#define FONT_FIXED_WIDTH	2
#define FONT_HEIGHT		3
#define FONT_FIRST_CHAR		4
#define FONT_CHAR_COUNT		5
#define FONT_WIDTH_TABLE	6


// Uncomment for slow drawing
// #define DEBUG

typedef struct {
	uint8_t x;
	uint8_t y;
	uint8_t page;
} lcdCoord;

typedef uint8_t (*FontCallback)(const uint8_t*);

uint8_t ReadPgmData(const uint8_t* ptr);	//Standard Read Callback

#define GLCD_DrawVertLine(x, y, length, color) GLCD_FillRect(x, y, 0, length, color)
#define GLCD_DrawHoriLine(x, y, length, color) GLCD_FillRect(x, y, length, 0, color)
#define GLCD_DrawCircle(xCenter, yCenter, radius, color) GLCD_DrawRoundRect(xCenter-radius, yCenter-radius, 2*radius, 2*radius, radius, color)
#define GLCD_ClearScreenX() GLCD_FillRect(0, 0, (DISPLAY_WIDTH-1), (DISPLAY_HEIGHT-1), WHITE)
#define GLCD_ClearSysTextLine(_line) GLCD_FillRect(0, (line*8), (DISPLAY_WIDTH-1), ((line*8)+ 7), WHITE )
#define GLCD_SelectFont(font) GLCD_SelectFontEx(font, BLACK, ReadPgmData)

// Control functions
void GLCD_Init(boolean invert);
void GLCD_GotoXY(uint8_t x, uint8_t y);

// Graphic Functions
void GLCD_ClearPage(uint8_t page, uint8_t color);
void GLCD_ClearScreen(uint8_t color);
void GLCD_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color);
void GLCD_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void GLCD_DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color);
void GLCD_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color);
void GLCD_InvertRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height);
void GLCD_SetInverted(boolean invert);
void GLCD_SetDot(uint8_t x, uint8_t y, uint8_t color);
void GLCD_DrawBitmap(const uint8_t *bitmap, uint8_t x, uint8_t y, uint8_t color);

// Font Functions
void GLCD_SelectFontEx(const uint8_t* font, uint8_t color, FontCallback callback);
int  GLCD_PutChar(char c);
void GLCD_Puts(const char *str);
void GLCD_PrintNumber(long n);
void GLCD_PrintHexNumber(uint16_t n);
void GLCD_PrintRealNumber(double n);
void GLCD_CursorTo( uint8_t x, uint8_t y); // 0 based coordinates for fixed width fonts (i.e. systemFont5x7)

uint8_t  GLCD_CharWidth(char c);
uint16_t GLCD_StringWidth(const char *str);

#endif
