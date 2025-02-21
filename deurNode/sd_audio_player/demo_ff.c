/*
 *  \file      demo_ff.h
 *  \brief     Demo routine to test write to and read from the SD-card
 *  \date      11-03-2020
 *  \version   1.5
 */
#include <stdio.h>
#include "ff/ff.h"
#include <avr/io.h>
#include <math.h>
#include <avr/interrupt.h>
#include "clock/clock.h"
#include "serialF0/serialF0.h"
#include <util/delay.h>

#define F_CPU 32000000UL

char buffer[256];

FATFS FatFs; // FatFs work area needed for each volume
FIL Fil;     // File object needed for each open file
UINT br, bw;  // Bytes gelezen en geschreven


/*! \brief  Demonstration. It writes text to a file on the SD-card. After that it reads the
 *          file from the SD-card and print the text.
 *          This example is code D.2 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 *
 *  \return void
 */

int demo_ff(void)
{
  UINT bw;
  uint8_t ret;

  printf("Demo FatFs\n");
  if ((ret = f_mount(&FatFs, "", 1)) != FR_OK)
  {
    printf("Demo aborted: device not found\n");
    return ret;
  }

  // create and write file demo.txt
  if ((ret = f_open(&Fil, "demo.txt", FA_WRITE | FA_CREATE_ALWAYS)) != FR_OK)
  {
    printf("Demo aborted: can't create file\n");
    return ret;
  }

  printf("DEMO : write file\n");
  f_write(&Fil, "Some text written!\r\n", 20, &bw);
  f_printf(&Fil, "Printf %s\n", "works as well");
  f_printf(&Fil, "Variabele bw is %d", bw);
  f_sync(&Fil);
  f_close(&Fil);

  // read file demo.txt
  if ((ret = f_open(&Fil, "demo.txt", FA_READ)) != FR_OK)
  {
    printf("Demo aborted: can't open file\n");
    return ret;
  }

  while (f_gets(buffer, 255, &Fil) != NULL)
  {
    printf("\t%s", buffer);
  }

  f_close(&Fil);

  return FR_OK;
}

typedef struct {
    char riff[4];       // "RIFF"
    uint32_t size;      // Bestandsgrootte - 8
    char wave[4];       // "WAVE"
    char fmt[4];        // "fmt "
    uint32_t fmt_size;  // Formaatgegevens grootte
    uint16_t audio_fmt; // Audio type (1 = PCM)
    uint16_t channels;  // Aantal kanalen
    uint32_t sample_rate; // Sample rate (bv. 44100 Hz)
    uint32_t byte_rate;   // Bytes per seconde
    uint16_t block_align; // Bytes per sample
    uint16_t bits_per_sample; // 8 of 16 bits
    uint16_t sub_chunk2id;
    uint16_t subchunk2_size;
} WAVHeader;

WAVHeader header;

int read_wav_header(const char *filename) {
    if (f_open(&Fil, filename, FA_READ) != FR_OK) {
        printf("Kan bestand niet openen!\n");
        return 0;
    }

    f_read(&Fil, &header, sizeof(WAVHeader), &br);
    if (br != sizeof(WAVHeader)) {
        printf("Fout bij lezen header!\n");
        f_close(&Fil);
        return 0;
    }

    // Controleer of het een geldig WAV-bestand is
    if (strncmp(header.riff, "RIFF", 4) || strncmp(header.wave, "WAVE", 4)) {
        printf("Geen geldig WAV-bestand!\n");
        f_close(&Fil);
        return 0;
    }

    printf("Sample Rate: %d Hz\n", header.sample_rate);
    printf("Bits per sample: %d\n", header.bits_per_sample);
    printf("Kanalen: %d\n", header.channels);
    printf("Data size: %d\n", header.subchunk2_size);

    return 1;
}



void play_wav()
{
  FIL file;
  WAVHeader header;
  UINT br;
  uint8_t buffer[512];

  if (f_open(&file, "audio.wav", FA_READ) == FR_OK)
  {
    f_read(&file, &header, sizeof(WAVHeader), &br); // Lees header
    while (f_read(&file, buffer, sizeof(buffer), &br) == FR_OK && br > 0)
    {
      for (uint16_t i = 0; i < br; i++)
      {
        DACB.CH0DATA = buffer[i];                // Stuur naar DAC
        _delay_ms(1000000 / header.sample_rate); // Sample rate timing
      }
    }
    f_close(&file);
  }
  else
  {
    printf("Kan WAV-bestand niet openen!\n");
  }
}
// void play_sound()
// {
//   uint16_t i;
//   while (1)
//   {
//     for (i = 0; i < 256; i++)
//     {
//       DACB.CH0DATA = 2048 + (1024 * sin(i * 2 * 3.14159 / 256)); // Sinusgolf
//       _delay_ms(4);
//     }
//   }
// }
