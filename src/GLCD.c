/*
  ks0108.cpp - Arduino library support for ks0108 and compatable graphic LCDs
  Copyright (c)2008 Michael Margolis All right reserved
  mailto:memargolis@hotmail.com?subject=KS0108_Library 

  The high level functions of this library are based on version 1.1 of ks0108 graphics routines
  written and copyright by Fabian Maximilian Thiele. His sitelink is dead but
  you can obtain a copy of his original work here:
  http://www.scienceprog.com/wp-content/uploads/2007/07/glcd_ks0108.zip

  Code changes include conversion to an Arduino C++ library, rewriting the low level routines 
  to read busy status flag and support a wider range of displays, adding more flexibility
  in port addressing and improvements in I/O speed. The interface has been made more Arduino friendly
  and some convenience functions added. 

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

  Version:   1.0 - May 8  2008  - first release
  Version:   1.1 - Nov 7  2008  - restructured low level code to adapt to panel speed
                                 - moved chip and panel configuration into seperate header files    
                                                                 - added fixed width system font
  Version:   2   - May 26 2009   - second release
                                 - added support for Mega and Sanguino, improved panel speed tolerance, added bitmap support
     
 */

#include <stdint.h>
#include <xc.h>

#define ksSOURCE
#include "GLCD.h"

#define delay(ms) __delay_ms(ms)

// the (EN_DELAY_VALUE) argument for the above delay is in GLCD_panel.h
#define EN_DELAY() __delay_us(EN_DELAY_VALUE)

/* GLCD Macros */
#define INPUT_MODE  0xFF
#define OUTPUT_MODE 0x00

#define _BV(i) (1 << (i))
#define isFixedWidthFont(font)  (GLCD_FontRead(font+FONT_LENGTH) == 0 && GLCD_FontRead(font+FONT_LENGTH+1) == 0)

#define GLCD_Enable()		\
	do {			\
		EN_DELAY();	\
		fastWriteHigh(EN); /* EN high level width min 450 ns */ \
		EN_DELAY();		\
		fastWriteLow(EN);	\
		/*EN_DELAY(); // some displays may need this delay at the end of the enable pulse */ \
	} while (0)

#define GLCD_SelectChip(chip)			\
	do {					\
		if(chipSelect[chip] & 1) 	\
			fastWriteHigh(CSEL1);	\
		else				\
			fastWriteLow(CSEL1);	\
						\
		if(chipSelect[chip] & 2)	\
			fastWriteHigh(CSEL2);	\
		else				\
			fastWriteLow(CSEL2);	\
	} while (0)

#define GLCD_WaitReady(chip)                        \
    do {                                            \
        /* wait until LCD busy bit goes to zero */  \
                                                    \
        GLCD_SelectChip(chip);                      \
        lcdDataDir(INPUT_MODE);                     \
        fastWriteLow(D_I);                          \
        fastWriteHigh(R_W);                         \
        fastWriteHigh(EN);                          \
        EN_DELAY();                                 \
        while (LCD_DATA_IN_HIGH & LCD_BUSY_FLAG);   \
        fastWriteLow(EN);                           \
    } while (0)

#define GLCD_ReadData(data)                             \
    do {                                                \
        GLCD_DoReadData(1); /* dummy read */            \
        data = GLCD_DoReadData(0); /* "real" read */    \
    } while (0)

#define GLCD_WriteCommand(cmd, chip)                    \
    do {                                                \
        if (GLCD_Coord.x % CHIP_WIDTH == 0 && chip > 0) { /*todo , ignore address 0??? */   \
            EN_DELAY();                                                                     \
        }                                                                                   \
        GLCD_WaitReady(chip);                                                               \
        fastWriteLow(D_I);                                                                  \
        fastWriteLow(R_W);                                                                  \
        lcdDataDir(OUTPUT_MODE);                                                            \
                                                                                            \
        EN_DELAY();                                                                         \
        lcdDataOut(cmd);                                                                    \
        GLCD_Enable();                                                                      \
        EN_DELAY();                                                                         \
        EN_DELAY();                                                                         \
        lcdDataOut(0x00);                                                                   \
    }  while (0)

/* GLCD control variables */
static lcdCoord GLCD_Coord;
static boolean GLCD_Inverted;
static FontCallback GLCD_FontRead;
static uint8_t GLCD_FontColor;
static const uint8_t* GLCD_Font;

/* GLCD private functions */
uint8_t GLCD_DoReadData(uint8_t first);
void GLCD_WriteData(uint8_t data); // experts can make this public but the functionality is not documented

//#define GLCD_DEBUG  // uncomment this if you want to slow down drawing to see how pixels are set

void GLCD_ClearPage(uint8_t page, uint8_t color)
{
    for (uint8_t x = 0; x < DISPLAY_WIDTH; x++) {
        GLCD_GotoXY(x, page * 8);
        GLCD_WriteData(color);
    }
}

void GLCD_ClearScreen(uint8_t color)
{
    uint8_t page;
    for (page = 0; page < 8; page++) {
        GLCD_GotoXY(0, page * 8);
        GLCD_ClearPage(page, color);
    }
}

/*
 * First, define a few macros to make the DrawLine code below read more like
 * the wikipedia example code.
 */

#define _GLCD_absDiff(x,y) ((x>y) ?  (x-y) : (y-x))
#define _GLCD_swap(a,b) \
do\
{\
uint8_t t;\
	t=a;\
	a=b;\
	b=t;\
} while(0)

void GLCD_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t color)
{
    uint8_t deltax, deltay, x, y, steep;
    int8_t error, ystep;

    steep = _GLCD_absDiff(y1, y2) > _GLCD_absDiff(x1, x2);

    if (steep) {
        _GLCD_swap(x1, y1);
        _GLCD_swap(x2, y2);
    }

    if (x1 > x2) {
        _GLCD_swap(x1, x2);
        _GLCD_swap(y1, y2);
    }

    deltax = x2 - x1;
    deltay = _GLCD_absDiff(y2, y1);
    error = deltax / 2;
    y = y1;
    if (y1 < y2) ystep = 1;
    else ystep = -1;

    for (x = x1; x <= x2; x++) {
        if (steep) GLCD_SetDot(y, x, color);
        else GLCD_SetDot(x, y, color);
        error = error - deltay;
        if (error < 0) {
            y = y + ystep;
            error = error + deltax;
        }
    }
}

void GLCD_DrawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    GLCD_DrawHoriLine(x, y, width, color); // top
    GLCD_DrawHoriLine(x, y + height, width, color); // bottom
    GLCD_DrawVertLine(x, y, height, color); // left
    GLCD_DrawVertLine(x + width, y, height, color); // right
}

void GLCD_DrawRoundRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t radius, uint8_t color)
{
    int16_t tSwitch, x1 = 0, y1 = radius;
    tSwitch = 3 - 2 * radius;

    while (x1 <= y1) {
        GLCD_SetDot(x + radius - x1, y + radius - y1, color);
        GLCD_SetDot(x + radius - y1, y + radius - x1, color);

        GLCD_SetDot(x + width - radius + x1, y + radius - y1, color);
        GLCD_SetDot(x + width - radius + y1, y + radius - x1, color);

        GLCD_SetDot(x + width - radius + x1, y + height - radius + y1, color);
        GLCD_SetDot(x + width - radius + y1, y + height - radius + x1, color);

        GLCD_SetDot(x + radius - x1, y + height - radius + y1, color);
        GLCD_SetDot(x + radius - y1, y + height - radius + x1, color);

        if (tSwitch < 0) {
            tSwitch += (4 * x1 + 6);
        } else {
            tSwitch += (4 * (x1 - y1) + 10);
            y1--;
        }
        x1++;
    }

    GLCD_DrawHoriLine(x + radius, y, width - (2 * radius), color); // top
    GLCD_DrawHoriLine(x + radius, y + height, width - (2 * radius), color); // bottom
    GLCD_DrawVertLine(x, y + radius, height - (2 * radius), color); // left
    GLCD_DrawVertLine(x + width, y + radius, height - (2 * radius), color); // right
}

/*
 * Hardware-Functions 
 */
void GLCD_FillRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint8_t color)
{
    uint8_t mask, pageOffset, h, i, data;
    height++;

    pageOffset = y % 8;
    y -= pageOffset;
    mask = 0xFF;
    if (height < 8 - pageOffset) {
        mask >>= (8 - height);
        h = height;
    } else {
        h = 8 - pageOffset;
    }
    mask <<= pageOffset;

    GLCD_GotoXY(x, y);
    for (i = 0; i <= width; i++) {
        GLCD_ReadData(data);

        if (color == BLACK) {
            data |= mask;
        } else {
            data &= ~mask;
        }

        GLCD_WriteData(data);
    }

    while (h + 8 <= height) {
        h += 8;
        y += 8;
        GLCD_GotoXY(x, y);

        for (i = 0; i <= width; i++) {
            GLCD_WriteData(color);
        }
    }

    if (h < height) {
        mask = ~(0xFF << (height - h));
        GLCD_GotoXY(x, y + 8);

        for (i = 0; i <= width; i++) {
            GLCD_ReadData(data);

            if (color == BLACK) {
                data |= mask;
            } else {
                data &= ~mask;
            }

            GLCD_WriteData(data);
        }
    }
}

void GLCD_InvertRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height)
{
    uint8_t mask, pageOffset, h, i, data, tmpData;
    height++;

    pageOffset = y % 8;
    y -= pageOffset;
    mask = 0xFF;
    if (height < 8 - pageOffset) {
        mask >>= (8 - height);
        h = height;
    } else {
        h = 8 - pageOffset;
    }
    mask <<= pageOffset;

    GLCD_GotoXY(x, y);
    for (i = 0; i <= width; i++) {
        GLCD_ReadData(data);
        tmpData = ~data;
        data = (tmpData & mask) | (data & ~mask);
        GLCD_WriteData(data);
    }

    while (h + 8 <= height) {
        h += 8;
        y += 8;
        GLCD_GotoXY(x, y);

        for (i = 0; i <= width; i++) {
            GLCD_ReadData(data);
            GLCD_WriteData(~data);
        }
    }

    if (h < height) {
        mask = ~(0xFF << (height - h));
        GLCD_GotoXY(x, y + 8);

        for (i = 0; i <= width; i++) {
            GLCD_ReadData(data);
            tmpData = ~data;
            data = (tmpData & mask) | (data & ~mask);
            GLCD_WriteData(data);
        }
    }
}

void GLCD_SetInverted(boolean invert)
{ // changed type to boolean
    if (GLCD_Inverted != invert) {
        GLCD_InvertRect(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);
        GLCD_Inverted = invert;
    }
}

void GLCD_SetDot(uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t data;

    GLCD_GotoXY(x, y - y % 8); // read data from display memory

    GLCD_ReadData(data);
    if (color == BLACK) {
        data |= 0x01 << (y % 8); // set dot
    } else {
        data &= ~(0x01 << (y % 8)); // clear dot
    }
    GLCD_WriteData(data); // write data back to display
}

//
// Font Functions
//

uint8_t ReadPgmData(const uint8_t* ptr)
{
    return *ptr;
}

void GLCD_SelectFontEx(const uint8_t* font, uint8_t color, FontCallback callback)
{
    GLCD_Font = font;
    GLCD_FontRead = callback;
    GLCD_FontColor = color;
}

void GLCD_PrintNumber(long n)
{
    byte buf[10]; // prints up to 10 digits
    byte i = 0;
    if (n == 0)
        GLCD_PutChar('0');
    else {
        if (n < 0) {
            GLCD_PutChar('-');
            n = -n;
        }
        while (n > 0 && i <= 10) {
            buf[i++] = n % 10; // n % base
            n /= 10; // n/= base
        }
        for (; i > 0; i--)
            GLCD_PutChar((char) (buf[i - 1] < 10 ? '0' + buf[i - 1] : 'A' + buf[i - 1] - 10));
    }
}

void GLCD_PrintHexNumber(uint16_t n)
{
    int8_t i;
    uint8_t d, previous_write = 0;

    if (n == 0) {
        GLCD_PutChar('0');
        return;
    }
    
    for (i=3; i>=0; i--) {
        d = ((0x0F << i*4) & n) >> i*4;

        if (d < 0xA)
            d += '0';
        else
            d = (d - 0xA) + 'A';
        
        if (d != '0' || previous_write) {
            GLCD_PutChar((char) d);
            previous_write = 1;
        }
    }
}

void GLCD_PrintRealNumber(double n)
{
    long int_part, frac_part;
    uint8_t sign = 0;

    if (n < 0) {
        sign = 1;
        n *= -1;
    }
    
    int_part = (long)n;
    frac_part = (long)((n - (double)int_part)*1000000);
    if (sign) GLCD_PutChar('-');
    GLCD_PrintNumber(int_part);
    GLCD_PutChar('.');

    while ((frac_part != 0) && (frac_part % 10 == 0)) {
        frac_part /= 10;
    }
    
    GLCD_PrintNumber(frac_part);
}

#undef GLCD_OLD_FONTDRAW

int GLCD_PutChar(char c)
{
    if (GLCD_Font == 0)
        return 0; // no font selected

    /*
     * check for special character processing
     */

    if (c < 0x20) {
        //SpecialChar(c);
        return 1;
    }

    uint8_t width = 0;
    uint8_t height = GLCD_FontRead(GLCD_Font + FONT_HEIGHT);
    uint8_t bytes = (height + 7) / 8; /* calculates height in rounded up bytes */

    uint8_t firstChar = GLCD_FontRead(GLCD_Font + FONT_FIRST_CHAR);
    uint8_t charCount = GLCD_FontRead(GLCD_Font + FONT_CHAR_COUNT);

    uint16_t index = 0;
    uint8_t x = GLCD_Coord.x, y = GLCD_Coord.y;
    uint8_t thielefont;

    if (c < firstChar || c >= (firstChar + charCount)) {
        return 0; // invalid char
    }
    c -= firstChar;

    if (isFixedWidthFont(GLCD_Font)) {
        thielefont = 0;
        width = GLCD_FontRead(GLCD_Font + FONT_FIXED_WIDTH);
        index = c * bytes * width + FONT_WIDTH_TABLE;
    } else {
        // variable width font, read width data, to get the index
        thielefont = 1;
        /*
         * Because there is no table for the offset of where the data
         * for each character glyph starts, run the table and add up all the
         * widths of all the characters prior to the character we
         * need to locate.
         */
        for (uint8_t i = 0; i < c; i++) {
            index += GLCD_FontRead(GLCD_Font + FONT_WIDTH_TABLE + i);
        }
        /*
         * Calculate the offset of where the font data
         * for our character starts.
         * The index value from above has to be adjusted because
         * there is potentialy more than 1 byte per column in the glyph,
         * when the characgter is taller than 8 bits.
         * To account for this, index has to be multiplied
         * by the height in bytes because there is one byte of font
         * data for each vertical 8 pixels.
         * The index is then adjusted to skip over the font width data
         * and the font header information.
         */

        index = index * bytes + charCount + FONT_WIDTH_TABLE;

        /*
         * Finally, fetch the width of our character
         */
        width = GLCD_FontRead(GLCD_Font + FONT_WIDTH_TABLE + c);
    }

    // last but not least, draw the character
#ifdef GLCD_OLD_FONTDRAW
    /*================== OLD FONT DRAWING ============================*/

    GLCD_GotoXY(x, y);

    /*
     * Draw each column of the glyph (character) horizontally
     * 8 bits (1 page) at a time.
     * i.e. if a font is taller than 8 bits, draw upper 8 bits first,
     * Then drop down and draw next 8 bits and so on, until done.
     * This code depends on WriteData() doing writes that span LCD
     * memory pages, which has issues because the font data isn't
     * always a multiple of 8 bits.
     */

    for (uint8_t i = 0; i < bytes; i++) /* each vertical byte */ {
        uint16_t page = i*width; // page must be 16 bit to prevent overflow
        for (uint8_t j = 0; j < width; j++) /* each column */ {
            uint8_t data = GLCD_FontRead(GLCD_Font + index + page + j);

            /*
             * This funkyness is because when the character glyph is not a
             * multiple of 8 in height, the residual bits in the font data
             * were aligned to the incorrect end of the byte with respect
             * to the GLCD. I believe that this was an initial oversight (bug)
             * in Thieles font creator program. It is easily fixed
             * in the font program but then creates a potential backward
             * compatiblity problem.
             *	--- bperrybap
             */

            if (height > 8 && height < (i + 1)*8) /* is it last byte of multibyte tall font? */ {
                data >>= (i + 1)*8 - height;
            }

            if (GLCD_FontColor == BLACK) {
                GLCD_WriteData(data);
            } else {
                GLCD_WriteData(~data);
            }
        }
        // 1px gap between chars
        if (GLCD_FontColor == BLACK) {
            GLCD_WriteData(0x00);
        } else {
            GLCD_WriteData(0xFF);
        }
        GLCD_GotoXY(x, GLCD_Coord.y + 8);
    }
    GLCD_GotoXY(x + width + 1, y);

    /*================== END of OLD FONT DRAWING ============================*/
#else

    /*================== NEW FONT DRAWING ===================================*/

    /*
     * Paint font data bits and write them to LCD memory 1 LCD page at a time.
     * This is very different from simply reading 1 byte of font data
     * and writing all 8 bits to LCD memory and expecting the write data routine
     * to fragement the 8 bits across LCD 2 memory pages when necessary.
     * That method (really doesn't work) and reads and writes the same LCD page
     * more than once as well as not do sequential writes to memory.
     *
     * This method of rendering while much more complicated, somewhat scrambles the font
     * data reads to ensure that all writes to LCD pages are always sequential and a given LCD
     * memory page is never read or written more than once.
     * And reads of LCD pages are only done at the top or bottom of the font data rendering
     * when necessary.
     * i.e it ensures the absolute minimum number of LCD page accesses
     * as well as does the sequential writes as much as possible.
     *
     */

    uint8_t pixels = height + 1; /* 1 for gap below character*/
    uint8_t p;
    uint8_t dy;
    uint8_t tfp;
    uint8_t dp;
    uint8_t dbyte;
    uint8_t fdata;

    for (p = 0; p < pixels;) {
        dy = y + p;

        /*
         * Align to proper Column and page in LCD memory
         */

        GLCD_GotoXY(x, (dy & ~7));

        uint16_t page = p / 8 * width; // page must be 16 bit to prevent overflow

        for (uint8_t j = 0; j < width; j++) /* each column of font data */ {

            /*
             * Fetch proper byte of font data.
             * Note:
             * This code "cheats" to add the horizontal space/pixel row
             * below the font.
             * It essentially creates a font pixel of 0 when the pixels are
             * out of the defined pixel map.
             *
             * fake a fondata read read when we are on the very last
             * bottom "pixel". This lets the loop logic continue to run
             * with the extra fake pixel. If the loop is not the
             * the last pixel the pixel will come from the actual
             * font data, but that is ok as it is 0 padded.
             *
             */

            if (p >= height) {
                /*
                 * fake a font data read for padding below character.
                 */
                fdata = 0;
            } else {
                fdata = GLCD_FontRead(GLCD_Font + index + page + j);

                /*
                 * Have to shift font data because Thiele shifted residual
                 * font bits the wrong direction for LCD memory.
                 *
                 * The real solution to this is to fix the variable width font format to
                 * not shift the residual bits the wrong direction!!!!
                 */
                if (thielefont && (height - (p&~7)) < 8) {
                    fdata >>= 8 - (height & 7);
                }
            }

            if (GLCD_FontColor == WHITE)
                fdata ^= 0xff; /* inverted data for "white" font color	*/


            /*
             * Check to see if a quick full byte write of font
             * data can be done.
             */

            if (!(dy & 7) && !(p & 7) && ((pixels - p) >= 8)) {
                /*
                 * destination pixel is on a page boundary
                 * Font data is on byte boundary
                 * And there are 8 or more pixels left
                 * to paint so a full byte write can be done.
                 */

                GLCD_WriteData(fdata);
                continue;
            } else {
                /*
                 * No, so must fetch byte from LCD memory.
                 */
                GLCD_ReadData(dbyte);
            }

            /*
             * At this point there is either not a full page of data
             * left to be painted  or the font data spans multiple font
             * data bytes. (or both) So, the font data bits will be painted
             * into a byte and then written to the LCD memory.page.
             */


            tfp = p; /* font pixel bit position 		*/
            dp = dy & 7; /* data byte pixel bit position */

            /*
             * paint bits until we hit bottom of page/byte
             * or run out of pixels to paint.
             */
            while ((dp <= 7) && (tfp) < pixels) {
                if (fdata & _BV(tfp & 7)) {
                    dbyte |= _BV(dp);
                } else {
                    dbyte &= ~_BV(dp);
                }

                /*
                 * Check for crossing font data bytes
                 */
                if ((tfp & 7) == 7) {
                    fdata = GLCD_FontRead(GLCD_Font + index + page + j + width);

                    /*
                     * Have to shift font data because Thiele shifted residual
                     * font bits the wrong direction for LCD memory.
                     *
                     */

                    if ((thielefont) && ((height - tfp) < 8)) {
                        fdata >>= (8 - (height & 7));
                    }

                    if (GLCD_FontColor == WHITE)
                        fdata ^= 0xff; /* inverted data for "white" color	*/
                }
                tfp++;
                dp++;
            }

            /*
             * Now flush out the painted byte.
             */
            GLCD_WriteData(dbyte);
        }

        /*
         * now create a horizontal gap (vertical line of pixels) between characters.
         * Since this gap is "white space", the pixels painted are oposite of the
         * font color.
         *
         * Since full LCD pages are being written, there are 4 combinations of filling
         * in the this gap page.
         *	- pixels start at bit 0 and go down less than 8 bits
         *	- pixels don't start at 0 but go down through bit 7
         *	- pixels don't start at 0 and don't go down through bit 7 (fonts shorter than 6 hi)
         *	- pixels start at bit 0 and go down through bit 7 (full byte)
         *
         * The code below creates a mask of the bits that should not be painted.
         *
         * Then it is easy to paint the desired bits since if the color is WHITE,
         * the paint bits are set, and if the coloer is not WHITE the paint bits are stripped.
         * and the paint bits are the inverse of the desired bits mask.
         */



        if ((dy & 7) || (pixels - p < 8)) {
            uint8_t mask = 0;

            GLCD_ReadData(dbyte);

            if (dy & 7)
                mask |= _BV(dy & 7) - 1;

            if ((pixels - p) < 8)
                mask |= ~(_BV(pixels - p) - 1);


            if (GLCD_FontColor == WHITE)
                dbyte |= ~mask;
            else
                dbyte &= mask;

        } else {
            if (GLCD_FontColor == WHITE)
                dbyte = 0xff;
            else
                dbyte = 0;
        }

        GLCD_WriteData(dbyte);

        /*
         * advance the font pixel for the pixels
         * just painted.
         */

        p += 8 - (dy & 7);
    }


    /*
     * Since this rendering code always starts off with a GotoXY() it really isn't necessary
     * to do a real GotoXY() to set the h/w location after rendering a character.
     * We can get away with only setting the s/w version of X & Y.
     *
     * Since y didn't change while rendering, it is still correct.
     * But update x for the pixels rendered.
     *
     */

    GLCD_GotoXY(x + width + 1, y);

    /*================== END of NEW FONT DRAWING ============================*/

#endif // NEW_FONTDRAW

    return 1; // valid char
}

void GLCD_Puts(const char *str)
{
    int x = GLCD_Coord.x;
    while (*str != 0) {
        if (*str == '\n') {
            GLCD_GotoXY(x, GLCD_Coord.y + GLCD_FontRead(GLCD_Font + FONT_HEIGHT));
        } else {
            GLCD_PutChar(*str);
        }
        str++;
    }
}

uint8_t GLCD_CharWidth(char c)
{
    uint8_t width = 0;
    uint8_t firstChar = GLCD_FontRead(GLCD_Font + FONT_FIRST_CHAR);
    uint8_t charCount = GLCD_FontRead(GLCD_Font + FONT_CHAR_COUNT);

    // read width data
    if (c >= firstChar && c < (firstChar + charCount)) {
        c -= firstChar;
        width = GLCD_FontRead(GLCD_Font + FONT_WIDTH_TABLE + c) + 1;
    }

    return width;
}

uint16_t GLCD_StringWidth(const char *str)
{
    uint16_t width = 0;

    while (*str != 0) {
        width += GLCD_CharWidth(*str++);
    }

    return width;
}

void GLCD_CursorTo(uint8_t x, uint8_t y)
{ // 0 based coordinates for fixed width fonts (i.e. systemFont5x7)
    GLCD_GotoXY(x * (GLCD_FontRead(GLCD_Font + FONT_FIXED_WIDTH) + 1),
            y * (GLCD_FontRead(GLCD_Font + FONT_HEIGHT) + 1));
}

void GLCD_GotoXY(uint8_t x, uint8_t y)
{
    uint8_t chip, cmd;

    if ((x > DISPLAY_WIDTH - 1) || (y > DISPLAY_HEIGHT - 1)) // exit if coordinates are not legal
        return;
    GLCD_Coord.x = x; // save new coordinates
    GLCD_Coord.y = y;

    if (y / 8 != GLCD_Coord.page) {
        GLCD_Coord.page = y / 8;
        cmd = LCD_SET_PAGE | GLCD_Coord.page; // set y address on all chips
        for (chip = 0; chip < DISPLAY_WIDTH / CHIP_WIDTH; chip++) {
            GLCD_WriteCommand(cmd, chip);
        }
    }
    chip = GLCD_Coord.x / CHIP_WIDTH;
    x = x % CHIP_WIDTH;
    cmd = LCD_SET_ADD | x;
    GLCD_WriteCommand(cmd, chip); // set x address on active chip
}

void GLCD_Init(boolean invert)
{
    /* User must Declare PINs as OUTPUT */

    delay(10);

    fastWriteLow(D_I);
    fastWriteLow(R_W);
    fastWriteLow(EN);

#ifdef RST
    /*
     * Reset the glcd module if there is a reset pin defined
     */
    fastWriteLow(RST);
    delay(2);
    fastWriteHigh(RST);
#endif

    /*
     *  extra blind delay for slow rising external reset signals
     *  and to give time for glcd to get up and running
     */
    delay(50);

    GLCD_Coord.x = 0;
    GLCD_Coord.y = 0;
    GLCD_Coord.page = 0;

    GLCD_Inverted = invert;

    for (uint8_t chip = 0; chip < DISPLAY_WIDTH / CHIP_WIDTH; chip++) {
        delay(10);
        GLCD_WriteCommand(LCD_ON, chip); // power on
        GLCD_WriteCommand(LCD_DISP_START, chip); // display start line = 0
    }
    delay(50);
    GLCD_ClearScreen(invert ? BLACK : WHITE); // display clear
    GLCD_GotoXY(0, 0);
}

uint8_t GLCD_DoReadData(uint8_t first)
{
    uint8_t data, chip;

    chip = GLCD_Coord.x / CHIP_WIDTH;
    GLCD_WaitReady(chip);
    if (first) {
        if (GLCD_Coord.x % CHIP_WIDTH == 0 && chip > 0) { // todo , remove this test and call GotoXY always?
            GLCD_GotoXY(GLCD_Coord.x, GLCD_Coord.y);
            GLCD_WaitReady(chip);
        }
    }
    fastWriteHigh(D_I); // D/I = 1
    fastWriteHigh(R_W); // R/W = 1

    fastWriteHigh(EN); // EN high level width: min. 450ns
    EN_DELAY();

#ifdef LCD_DATA_NIBBLES
    data = (LCD_DATA_IN_LOW & 0x0F) | (LCD_DATA_IN_HIGH & 0xF0);
#else
    data = LCD_DATA_IN_HIGH; // low and high nibbles on same port so read all 8 bits at once
#endif 
    fastWriteLow(EN);
    if (first == 0)
        GLCD_GotoXY(GLCD_Coord.x, GLCD_Coord.y);
    if (GLCD_Inverted)
        data = ~data;
    return data;
}

void GLCD_WriteData(uint8_t data)
{
    uint8_t displayData, yOffset, chip;
    //showHex("wrData",data);
    //showXY("wr", GLCD_Coord.x,GLCD_Coord.y);

#ifdef LCD_CMD_PORT	
    uint8_t cmdPort;
#endif

#ifdef GLCD_DEBUG
    volatile uint16_t i;
    for (i = 0; i < 5000; i++);
#endif

    if (GLCD_Coord.x >= DISPLAY_WIDTH)
        return;
    chip = GLCD_Coord.x / CHIP_WIDTH;
    GLCD_WaitReady(chip);


    if (GLCD_Coord.x % CHIP_WIDTH == 0 && chip > 0) { // todo , ignore address 0???
        GLCD_GotoXY(GLCD_Coord.x, GLCD_Coord.y);
    }

    fastWriteHigh(D_I); // D/I = 1
    fastWriteLow(R_W); // R/W = 0
    lcdDataDir(OUTPUT_MODE); // data port is output

    yOffset = GLCD_Coord.y % 8;

    if (yOffset != 0) {
        // first page
#ifdef LCD_CMD_PORT 
        cmdPort = LCD_CMD_PORT; // save command port
#endif
        GLCD_ReadData(data);
#ifdef LCD_CMD_PORT 		
        LCD_CMD_PORT = cmdPort; // restore command port
#else
        fastWriteHigh(D_I); // D/I = 1
        fastWriteLow(R_W); // R/W = 0
        GLCD_SelectChip(chip);
#endif
        lcdDataDir(OUTPUT_MODE); // data port is output

        displayData |= data << yOffset;
        if (GLCD_Inverted)
            displayData = ~displayData;
        lcdDataOut(displayData); // write data
        GLCD_Enable(); // enable

        // second page
        GLCD_GotoXY(GLCD_Coord.x, GLCD_Coord.y + 8);

        GLCD_ReadData(displayData);

#ifdef LCD_CMD_PORT 		
        LCD_CMD_PORT = cmdPort; // restore command port
#else		
        fastWriteHigh(D_I); // D/I = 1
        fastWriteLow(R_W); // R/W = 0
        GLCD_SelectChip(chip);
#endif
        lcdDataDir(OUTPUT_MODE); // data port is output

        displayData |= data >> (8 - yOffset);
        if (GLCD_Inverted)
            displayData = ~displayData;
        lcdDataOut(displayData); // write data
        GLCD_Enable(); // enable

        GLCD_GotoXY(GLCD_Coord.x + 1, GLCD_Coord.y - 8);
    } else {
        // just this code gets executed if the write is on a single page
        if (GLCD_Inverted)
            data = ~data;
        EN_DELAY();
        lcdDataOut(data); // write data
        GLCD_Enable(); // enable
        GLCD_Coord.x++;
        //showXY("WrData",GLCD_Coord.x, GLCD_Coord.y);
    }
}

void GLCD_DrawBitmap(const uint8_t * bitmap, uint8_t x, uint8_t y, uint8_t color)
{
    uint8_t width, height;
    uint8_t i, j;

    width = ReadPgmData(bitmap++);
    height = ReadPgmData(bitmap++);
    for (j = 0; j < height / 8; j++) {
        GLCD_GotoXY(x, y + (j * 8));
        for (i = 0; i < width; i++) {
            uint8_t displayData = ReadPgmData(bitmap++);
            if (color == BLACK)
                GLCD_WriteData(displayData);
            else
                GLCD_WriteData(~displayData);
        }
    }
}

