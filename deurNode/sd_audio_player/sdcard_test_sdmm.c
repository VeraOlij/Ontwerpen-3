/*!
 *  \file    sdcard_test_sdmm.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    11-03-2020
 *  \version 1.5
 *
 *  \brief   Test program for sd-card with Chan FatFS and customized sdmm.c
 *
 *  \details <b>Usage with Atmel Studio.</b><br>
 *           This example uses the serialF0, see https://dolman-wim.nl/xmega/libraries/online/serialF0/html/index.html
 *           and the clock functions, see https://dolman-wim.nl/xmega/libraries/online/clock/html/index.html
 *           In the project directory uses three subfolders: clock, ff, and serialF0.
 *           <ol><li>Make an Atmel Studio project
 *           </li><li>Add three subfolders (use New Folder...) to the project  : clock, ff, and serialF0
 *           </li><li>Place the necessary files in the Atmel Studio project folder/subfolders
 *               <table>
 *               <tr><td>clock/clock.c          </td><td>Clock functions</td></tr>
 *               <tr><td>clock/clock.h          </td><td>Clock functions</td></tr>
 *               <tr><td>serialF0/serialF0.c    </td><td>Serial interface</td></tr>
 *               <tr><td>serialF0/serialF0.h    </td><td>Serial interface</td></tr>
 *               <tr><td>ff/ff.c                </td><td>Latest version from Chan's Library</td></tr>
 *               <tr><td>ff/ff.h                </td><td>Latest version from Chan's Library</td></tr>
 *               <tr><td>ff/ffconf.h            </td><td>Latest version from Chan's Library</td></tr>
 *               <tr><td>ff/ sdmm.c             </td><td>Customized version Chan's Sample \e generic release 11a</td></tr>
 *               <tr><td>sdcard_test_sdmm.c     </td><td>This example</td></tr>
 *               <tr><td>demo_ff.c              </td><td>function with demo SD-card</td></tr>
 *               </table>
 *           </li><li> Add all c-files to the project
 *           </li><li> Change the four defines in ffconf.h
 *           \verbatim
               #define FF_USE_STRFUNC  2
               #define FF_USE_CHMO     1
               #define FF_FS_NORTC     1
               #define FF_USE_LFN      0 \endverbatim
 *           </li></ol>
 *
 *           You can place the library files also directly in the project folder or in an other
 *           folder. In that case you need to change the path in the <code>\#include</code> lines.
 *
 *           This test program is based on the example D.1 and D.2 from
 *           <a href="http://www.dolman-wim.nl/xmega/index.php">'De taal C en de Xmega'</a> 2nd edition.
 *
 */
#ifndef F_CPU
#define F_CPU 32000000UL
#endif

#include <avr/interrupt.h>
#include "clock/clock.h"
#include "serialF0/serialF0.h"
#include <stdio.h>
#include "ff/ff.h"
#include <avr/io.h>
#include <math.h>
#include <avr/interrupt.h>
#include "clock/clock.h"
#include "serialF0/serialF0.h"
#include <util/delay.h>

FATFS FatFs;                        // Bestandssysteem object
FIL file;                  // Bestand object
UINT br;                   // Bytes gelezen
volatile uint8_t sampleBuffer[256];  // 16-bit buffer
volatile uint16_t bufferIndex = 0;
volatile uint16_t bufferSize = 0;




void init_dac(void)
{
  DACB.CTRLC = DAC_REFSEL_AVCC_gc;
  DACB.CTRLB = DAC_CHSEL_SINGLE_gc;
  DACB.CTRLA = DAC_CH0EN_bm | DAC_ENABLE_bm;
}

int demo_ff(void);
void play_wav();
int read_wav_header(const char *filename);

void sd_init()
{
  if (f_mount(&FatFs, "", 1) != FR_OK)
  {
    printf("SD-kaart mounten mislukt!\n");
    while (1)
      ;
  }
  printf("SD-kaart gemount!\n");
}

void timer_init()
{
  TCC0.CTRLA = TC_CLKSEL_DIV1_gc;     // Start timer met
  TCC0.PER = 1999;                    // Stel periode in voor 16 kHz
  TCC0.CTRLB = TC_WGMODE_NORMAL_gc;   // Normale mode
  TCC0.INTCTRLA = TC_OVFINTLVL_LO_gc; // Interrupt bij overflow
  PMIC.CTRL |= PMIC_LOLVLEN_bm;       // Zet Low Level Interrupts aan
}

int main(void)
{

  init_clock();
  timer_init();
  init_dac();
  init_stream(F_CPU);
  _delay_ms(10);
  sd_init();

  if (f_open(&file, "audio.wav", FA_READ) != FR_OK)
  {
    printf("Kan bestand niet openen!\n");
    while (1)
      ;
  }
_delay_ms(10);
  f_lseek(&file, 44); // Header overslaan
  sei();

  while (1)
  {

  } // do nothing
}

ISR(TCC0_OVF_vect) {
    if (bufferIndex < bufferSize) {
        // Zet sample om naar DAC (8-bit unsigned naar 12-bit DAC)
        DACB.CH0DATA = sampleBuffer[bufferIndex] << 4;
        bufferIndex++;
    } else {
        // Lees nieuw audioblok in
        f_read(&file, (void *)sampleBuffer, sizeof(sampleBuffer), &br);
        bufferSize = br;
        bufferIndex = 0;

        if (br == 0) { 
            TCC0.CTRLA = 0;  // Stop als we klaar zijn
            f_close(&file);
        }
    }
}