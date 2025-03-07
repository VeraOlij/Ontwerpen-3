/*!
 *  \file      sdmm.c
 *  \brief     This is customized version of sdmm.c 
 *             The original version is from sample \e generic release R0.11a
 *  \date      11 march 2020 (previous released 2016)
 *          
 *  \details  It is modified by Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>) 
 *             for the HvA-Xmegaboard version 2
 *             The most important global changes are:
 *             - It is suitable for the ATxmega256a3u.
 *             - It uses the Xmega style (DIRSET, OUTSET, ...).
 *             - It uses SPIC with a remap of (SCK and MOSI) in stead of bit banging.
 *             .
 *  
 *             The most important detailed changes are:
 *             1.  The static function dly_us() is changed. It uses the normal delay functions.
 *                 The delays are a little longer (about 1.5% at 32Hz) due to the 
 *                 fact that the for loop introduces extra clock cycles.
 *             2.  A static function sdspi_init() is added that initialize the SPI
 *             3.  A static function sdspi_write_byte() is added to write a byte to the SPI
 *             4.  A static function sdspi_read_byte() is added to read a byte from the SPI
 *             5.  The static function xmit_mmc() is changed. 
 *                 It now uses sdspi_write_byte() to write bytes
 *             6.  The static function rcvr_mmc() is changed. 
 *                 It now uses sdspi_read_byte() to read bytes
 *             7.  The function disk_initialize() is changed.
 *                 It now uses the function sdspi_init() to initialize the SPI.
 *
 *  \note      It is beter to use a 32 MHz clock.
 *             The error in the delays will be relatively large at a low frequency
 */
//! \cond CHANGED1
/*------------------------------------------------------------------------/
/  Foolproof MMCv3/SDv1/SDv2 (in SPI mode) control module
/-------------------------------------------------------------------------/
/
/  Copyright (C) 2013, ChaN, all right reserved.
/
/ * This software is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-------------------------------------------------------------------------/
  Features and Limitations:
  * It is modified by Wim Dolman (w.e.dolman@hva.nl) for the
     HvA-Xmegaboard version 2

  * Platform Dependent
    It is now special for the HvA-Xmegaboard version 2.

  * No Media Change Detection
    Application program needs to perform a f_mount() after media change.

  * No timeout
    It uses delays

   Changed 30 march 2016
   Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
   Made it suitable for the HvA-Xmegaboard. The most important chenges are:
     -  Xmega256a3u
     -  Xmega-style (DIRSET, OUTSET, ...)
     -  Using SPIC with remap of (SCK and MOSI) in stead of bit banging
     -  It excepts a 32 MHz clock for the delays

/-------------------------------------------------------------------------*/

#include "diskio.h"    /* Common include file for FatFs and disk I/O layer */

/*-------------------------------------------------------------------------*/
/* Platform dependent macros and functions needed to be modified           */
/*-------------------------------------------------------------------------*/
#ifndef F_CPU
#define F_CPU 32000000UL                                                       //!< added:  This implementation expects a 32 MHz clock
#endif
#include <avr/io.h>      /* Include device specific declaration file here */
#include <util/delay.h>                                                        //!< added:  This implementation uses the delay functions

static void dly_us (uint16_t n) {
  for (uint16_t i=0; i<n/10; i++) {
    _delay_us(10);
  }
}

#define CS_H sdspi_CShigh                                                      //!< added:  alias for sdspi_CShigh
#define CS_L sdspi_CSlow                                                       //!< added:  alias for sdspi_CSlow

//! \endcond 

//! \cond NOTCHANGED1
/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

// MMC/SD command (SPI mode)
#define CMD0   (0)        /* GO_IDLE_STATE */
#define CMD1   (1)        /* SEND_OP_COND */
#define ACMD41 (0x80+41)  /* SEND_OP_COND (SDC) */
#define CMD8   (8)        /* SEND_IF_COND */
#define CMD9   (9)        /* SEND_CSD */
#define CMD10  (10)       /* SEND_CID */
#define CMD12  (12)       /* STOP_TRANSMISSION */
#define CMD13  (13)       /* SEND_STATUS */
#define ACMD13 (0x80+13)  /* SD_STATUS (SDC) */
#define CMD16  (16)       /* SET_BLOCKLEN */
#define CMD17  (17)       /* READ_SINGLE_BLOCK */
#define CMD18  (18)       /* READ_MULTIPLE_BLOCK */
#define CMD23  (23)       /* SET_BLOCK_COUNT */
#define ACMD23 (0x80+23)  /* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24  (24)       /* WRITE_BLOCK */
#define CMD25  (25)       /* WRITE_MULTIPLE_BLOCK */
#define CMD32  (32)       /* ERASE_ER_BLK_START */
#define CMD33  (33)       /* ERASE_ER_BLK_END */
#define CMD38  (38)       /* ERASE */
#define CMD55  (55)       /* APP_CMD */
#define CMD58  (58)       /* READ_OCR */

static DSTATUS Stat = STA_NOINIT;  // Disk status
static BYTE CardType;              // b0:MMC, b1:SDv1, b2:SDv2,
                                   //       b3:Block addressing

//! \endcond 

//! \cond CHANGED2
/*-----------------------------------------------------------------------*/
/* SPI Chip Select for SD-card HvA-Xmegaboard version 2                  */
/*-----------------------------------------------------------------------*/
#define sdspi_CSlow()   PORTC.OUTCLR = PIN4_bm           //!< added:  define for Chip Select low  
#define sdspi_CShigh()  PORTC.OUTSET = PIN4_bm           //!< added:  define for Chip Select high

/*-----------------------------------------------------------------------*/
/* SPI Initialization for SD-card HvA-Xmegaboard version 2               */
/*-----------------------------------------------------------------------*/
static void sdspi_init(void)
{
  // port configuratie mmc/sd/sdhc card
  PORTC.REMAP    = PORT_SPI_bm;         // swap SCK and MOSI
  PORTC.DIRCLR   = PIN6_bm;             // MISO        input
  PORTC.DIRSET   = PIN5_bm;             // SCK         output
  PORTC.DIRSET   = PIN7_bm;             // MOSI        output
  PORTC.DIRSET   = PIN4_bm;             // chip select output
  PORTC.PIN6CTRL = PORT_OPC_PULLUP_gc;  // MISO        pullup

  // fspi = fcpu/4 = 8 MHz
  SPIC.CTRL      = SPI_ENABLE_bm |
                   SPI_MASTER_bm |
                   SPI_PRESCALER_DIV4_gc |
                   SPI_MODE_0_gc;
  SPIC.INTCTRL   = SPI_INTLVL_OFF_gc;

  // disable card
  sdspi_CShigh();
}

/*-----------------------------------------------------------------------*/
/* SPI write byte for SD-card HvA-Xmegaboard version 2                   */
/*-----------------------------------------------------------------------*/
static void sdspi_write_byte(unsigned char databyte)
{
  SPIC.DATA = databyte;
  while ( !(SPIC.STATUS & (SPI_IF_bm))) ;
}

/*-----------------------------------------------------------------------*/
/* SPI read byte for SD-card HvA-Xmegaboard version 2                    */
/*-----------------------------------------------------------------------*/
static unsigned char sdspi_read_byte(void)
{
  SPIC.DATA = 0xff;
  while ( !(SPIC.STATUS & (SPI_IF_bm))) ;

  return (SPIC.DATA);
}

/*-----------------------------------------------------------------------*/
/* Transmit bytes to the card (with SPI)                                 */
/*-----------------------------------------------------------------------*/
//  const BYTE* buff,  /* Data to be sent */
//  UINT bc        /* Number of bytes to send */
static void xmit_mmc (const BYTE* buff, UINT bc)
{
  BYTE d;

  do {
    d = *buff++;  /* Get a byte to be sent */
    sdspi_write_byte(d);
  } while (--bc);
}

/*-----------------------------------------------------------------------*/
/* Receive bytes from the card (with SPI)                                */
/*-----------------------------------------------------------------------*/
//  BYTE *buff,  /* Pointer to read buffer */
//  UINT bc    /* Number of bytes to receive */
static void rcvr_mmc(BYTE *buff, UINT bc)
{
  BYTE r;

  do {
    r = sdspi_read_byte();
    *buff++ = r;      /* Store a received byte */
  } while (--bc);
}
//! \endcond

//! \cond NOTCHANGED2
/*
 * FROM HERE NOTHING CHANGED!
 * From it is completely equal to sdmm.c of example generic/sdmm.c
 * Except that in function disk_initialize() spi_init() is used to
 * initialize the SPI
 */
/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/
static int wait_ready (void)  /* 1:OK, 0:Timeout */
{
  BYTE d;
  UINT tmr;


  for (tmr = 5000; tmr; tmr--) {  /* Wait for ready in timeout of 500ms */
    rcvr_mmc(&d, 1);
    if (d == 0xFF) break;
    dly_us(100);
  }

  return tmr ? 1 : 0;
}

/*-----------------------------------------------------------------------*/
/* Deselect the card and release SPI bus                                 */
/*-----------------------------------------------------------------------*/

static void deselect (void)
{
  BYTE d;

  CS_H();
  rcvr_mmc(&d, 1);  /* Dummy clock (force DO hi-z for multiple slave SPI) */
}

/*-----------------------------------------------------------------------*/
/* Select the card and wait for ready                                    */
/*-----------------------------------------------------------------------*/
static int select (void)  /* 1:OK, 0:Timeout */
{
  BYTE d;

  CS_L();
  rcvr_mmc(&d, 1);  /* Dummy clock (force DO enabled) */
  if (wait_ready()) return 1;  /* Wait for card ready */

  deselect();
  return 0;      /* Failed */
}

/*-----------------------------------------------------------------------*/
/* Receive a data packet from the card                                   */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (  /* 1:OK, 0:Failed */
  BYTE *buff,      /* Data buffer to store received data */
  UINT btr      /* Byte count */
)
{
  BYTE d[2];
  UINT tmr;


  for (tmr = 1000; tmr; tmr--) {  /* Wait for data packet in timeout of 100ms */
    rcvr_mmc(d, 1);
    if (d[0] != 0xFF) break;
    dly_us(100);
  }
  if (d[0] != 0xFE) return 0;    /* If not valid data token, return with error */

  rcvr_mmc(buff, btr);      /* Receive the data block into buffer */
  rcvr_mmc(d, 2);          /* Discard CRC */

  return 1;            /* Return with success */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the card                                        */
/*-----------------------------------------------------------------------*/

static
int xmit_datablock (  /* 1:OK, 0:Failed */
  const BYTE *buff,  /* 512 byte data block to be transmitted */
  BYTE token      /* Data/Stop token */
)
{
  BYTE d[2];


  if (!wait_ready()) return 0;

  d[0] = token;
  xmit_mmc(d, 1);        /* Xmit a token */
  if (token != 0xFD) {    /* Is it data token? */
    xmit_mmc(buff, 512);  /* Xmit the 512 byte data block to MMC */
    rcvr_mmc(d, 2);      /* Xmit dummy CRC (0xFF,0xFF) */
    rcvr_mmc(d, 1);      /* Receive data response */
    if ((d[0] & 0x1F) != 0x05)  /* If not accepted, return with error */
      return 0;
  }

  return 1;
}



/*-----------------------------------------------------------------------*/
/* Send a command packet to the card                                     */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (    /* Returns command response (bit7==1:Send failed)*/
  BYTE cmd,    /* Command byte */
  DWORD arg    /* Argument */
)
{
  BYTE n, d, buf[6];


  if (cmd & 0x80) {  /* ACMD<n> is the command sequense of CMD55-CMD<n> */
    cmd &= 0x7F;
    n = send_cmd(CMD55, 0);
    if (n > 1) return n;
  }

  /* Select the card and wait for ready except to stop multiple block read */
  if (cmd != CMD12) {
    deselect();
    if (!select()) return 0xFF;
  }

  /* Send a command packet */
  buf[0] = 0x40 | cmd;      /* Start + Command index */
  buf[1] = (BYTE)(arg >> 24);    /* Argument[31..24] */
  buf[2] = (BYTE)(arg >> 16);    /* Argument[23..16] */
  buf[3] = (BYTE)(arg >> 8);    /* Argument[15..8] */
  buf[4] = (BYTE)arg;        /* Argument[7..0] */
  n = 0x01;            /* Dummy CRC + Stop */
  if (cmd == CMD0) n = 0x95;    /* (valid CRC for CMD0(0)) */
  if (cmd == CMD8) n = 0x87;    /* (valid CRC for CMD8(0x1AA)) */
  buf[5] = n;
  xmit_mmc(buf, 6);

  /* Receive command response */
  if (cmd == CMD12) rcvr_mmc(&d, 1);  /* Skip a stuff byte when stop reading */
  n = 10;                /* Wait for a valid response in timeout of 10 attempts */
  do
    rcvr_mmc(&d, 1);
  while ((d & 0x80) && --n);

  return d;      /* Return with the response value */
}



/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
  BYTE drv      /* Drive number (always 0) */
)
{
  if (drv) return STA_NOINIT;

  return Stat;
}


//! \endcond 

//! \cond CHANGED3

/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
  BYTE drv    /* Physical drive nmuber (0) */
)
{
  BYTE n, ty, cmd, buf[4];
  UINT tmr;
  DSTATUS s;


  if (drv) return RES_NOTRDY;

  dly_us(10000);      /* 10ms */
  sdspi_init();       //! Changed: this function uses now sdspi_init() for initialization  

  for (n = 10; n; n--) rcvr_mmc(buf, 1);  /* Apply 80 dummy clocks and the card gets ready to receive command */

  ty = 0;
  if (send_cmd(CMD0, 0) == 1) {      /* Enter Idle state */
    if (send_cmd(CMD8, 0x1AA) == 1) {  /* SDv2? */
      rcvr_mmc(buf, 4);              /* Get trailing return value of R7 resp */
      if (buf[2] == 0x01 && buf[3] == 0xAA) {    /* The card can work at vdd range of 2.7-3.6V */
        for (tmr = 1000; tmr; tmr--) {      /* Wait for leaving idle state (ACMD41 with HCS bit) */
          if (send_cmd(ACMD41, 1UL << 30) == 0) break;
          dly_us(1000);
        }
        if (tmr && send_cmd(CMD58, 0) == 0) {  /* Check CCS bit in the OCR */
          rcvr_mmc(buf, 4);
          ty = (buf[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;  /* SDv2 */
        }
      }
    } else {              /* SDv1 or MMCv3 */
      if (send_cmd(ACMD41, 0) <= 1)   {
        ty = CT_SD1; cmd = ACMD41;  /* SDv1 */
      } else {
        ty = CT_MMC; cmd = CMD1;  /* MMCv3 */
      }
      for (tmr = 1000; tmr; tmr--) {      /* Wait for leaving idle state */
        if (send_cmd(cmd, 0) == 0) break;
        dly_us(1000);
      }
      if (!tmr || send_cmd(CMD16, 512) != 0)  /* Set R/W block length to 512 */
        ty = 0;
    }
  }
  CardType = ty;
  s = ty ? 0 : STA_NOINIT;
  Stat = s;

  deselect();

  return s;
}

//! \endcond 

//! \cond NOTCHANGED3

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
  BYTE drv,      /* Physical drive nmuber (0) */
  BYTE *buff,      /* Pointer to the data buffer to store read data */
  DWORD sector,    /* Start sector number (LBA) */
  UINT count      /* Sector count (1..128) */
)
{
  BYTE cmd;


  if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
  if (!(CardType & CT_BLOCK)) sector *= 512;  /* Convert LBA to byte address if needed */

  cmd = count > 1 ? CMD18 : CMD17;      /*  READ_MULTIPLE_BLOCK : READ_SINGLE_BLOCK */
  if (send_cmd(cmd, sector) == 0) {
    do {
      if (!rcvr_datablock(buff, 512)) break;
      buff += 512;
    } while (--count);
    if (cmd == CMD18) send_cmd(CMD12, 0);  /* STOP_TRANSMISSION */
  }
  deselect();

  return count ? RES_ERROR : RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
  BYTE drv,      /* Physical drive nmuber (0) */
  const BYTE *buff,  /* Pointer to the data to be written */
  DWORD sector,    /* Start sector number (LBA) */
  UINT count      /* Sector count (1..128) */
)
{
  if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;
  if (!(CardType & CT_BLOCK)) sector *= 512;  /* Convert LBA to byte address if needed */

  if (count == 1) {  /* Single block write */
    if ((send_cmd(CMD24, sector) == 0)  /* WRITE_BLOCK */
      && xmit_datablock(buff, 0xFE))
      count = 0;
  }
  else {        /* Multiple block write */
    if (CardType & CT_SDC) send_cmd(ACMD23, count);
    if (send_cmd(CMD25, sector) == 0) {  /* WRITE_MULTIPLE_BLOCK */
      do {
        if (!xmit_datablock(buff, 0xFC)) break;
        buff += 512;
      } while (--count);
      if (!xmit_datablock(0, 0xFD))  /* STOP_TRAN token */
        count = 1;
    }
  }
  deselect();

  return count ? RES_ERROR : RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
  BYTE drv,    /* Physical drive nmuber (0) */
  BYTE ctrl,    /* Control code */
  void *buff    /* Buffer to send/receive control data */
)
{
  DRESULT res;
  BYTE n, csd[16];
  DWORD cs;


  if (disk_status(drv) & STA_NOINIT) return RES_NOTRDY;  /* Check if card is in the socket */

  res = RES_ERROR;
  switch (ctrl) {
    case CTRL_SYNC :    /* Make sure that no pending write process */
      if (select()) res = RES_OK;
      break;

    case GET_SECTOR_COUNT :  /* Get number of sectors on the disk (DWORD) */
      if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
        if ((csd[0] >> 6) == 1) {  /* SDC ver 2.00 */
          cs = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
          *(DWORD*)buff = cs << 10;
        } else {          /* SDC ver 1.XX or MMC */
          n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
          cs = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
          *(DWORD*)buff = cs << (n - 9);
        }
        res = RES_OK;
      }
      break;

    case GET_BLOCK_SIZE :  /* Get erase block size in unit of sector (DWORD) */
      *(DWORD*)buff = 128;
      res = RES_OK;
      break;

    default:
      res = RES_PARERR;
  }

  deselect();

  return res;
}
//! \endcond



