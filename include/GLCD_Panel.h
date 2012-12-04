/*
  GLCD_Panel.h - PIC library support for ks0108 and compatible graphic LCDs
  Ported to PIC by Ivan de Jesus Deras ideras@gmail.com
  Original library by Michael Margolis All right reserved

  This is the configuration file for LCD panel specific configuration

*/

#ifndef	GLCD_PANEL_H
#define GLCD_PANEL_H

/*********************************************************/
/*  Configuration for LCD panel specific configuration   */
/*********************************************************/
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

// panel controller chips
#define CHIP_WIDTH     64  // pixels per chip 

// you can swap around the elements below if your display is reversed
#ifdef ksSOURCE

#if (DISPLAY_WIDTH / CHIP_WIDTH  == 2) 
   byte chipSelect[] = {1, 2};        // this is for 128 pixel displays
#elif (DISPLAY_WIDTH / CHIP_WIDTH  == 3)
   //byte chipSelect[] = {0, 1, 2};  // this is for 192 pixel displays
   byte chipSelect[] = {0, 2, 1};  // this is for 192 pixel displays on sanguino only
#endif

#if (DISPLAY_WIDTH / CHIP_WIDTH  == 2) 
#define DisableController(chip)    do { fastWriteLow(CSEL1); fastWriteLow(CSEL2); } while (0)   // disable for 128 pixel panels
#else
#define DisableController(chip)    do { fastWriteHigh(CSEL1); fastWriteHigh(CSEL2); } while (0) // disable for 192 pixel panels
#endif

#if _XTAL_FREQ == 8000000
#define EN_DELAY_VALUE 1 // this is the delay value that may need to be hand tuned for slow panels
#else
#define EN_DELAY_VALUE 2
#endif

#endif // ksSource defined to expose chipSelect only to c file

#endif
