/*
 * clock.c
 *
 * Created: 4-9-2016 10:04:12
 *  Author: dolwe
 */ 
#include <avr/io.h>

void Config32MHzClock(void)
{
  OSC.CTRL |= OSC_RC32MEN_bm;                  // Enable internal 32 MHz oscillator
  while(!(OSC.STATUS & OSC_RC32MRDY_bm));      // Wait for oscillator is ready
  CCP = CCP_IOREG_gc;                          // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc;             // Select sysclock 32 MHz oscillator
}

void Config32MHzClock_Ext16M(void)
{
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc |                   // Select frequency range
                 OSC_XOSCSEL_XTAL_16KCLK_gc;                // Select start-up time
  OSC.CTRL |= OSC_XOSCEN_bm;                                // Enable oscillator
  while ( ! (OSC.STATUS & OSC_XOSCRDY_bm) );                // Wait for oscillator is ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (OSC_PLLFAC_gm & 2);   // Select PLL source and multipl. factor
  OSC.CTRL |= OSC_PLLEN_bm;                                 // Enable PLL
  while ( ! (OSC.STATUS & OSC_PLLRDY_bm) );                 // Wait for PLL is ready

  CCP = CCP_IOREG_gc;                                       // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;                            // Select system clock source
  OSC.CTRL &= ~OSC_RC2MEN_bm;                               // Turn off 2MHz internal oscillator
  OSC.CTRL &= ~OSC_RC32MEN_bm;                              // Turn off 32MHz internal oscillator
}

void Config16MHzClock_Ext16M(void)
{
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc |                   // Select frequency range
                 OSC_XOSCSEL_XTAL_16KCLK_gc;                // Select start-up time
  OSC.CTRL |= OSC_XOSCEN_bm;                                // Enable oscillator
  while ( ! (OSC.STATUS & OSC_XOSCRDY_bm) );                // Wait for oscillator is ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (OSC_PLLFAC_gm & 1);   // Select PLL source and multipl. factor
  OSC.CTRL |= OSC_PLLEN_bm;                                 // Enable PLL
  while ( ! (OSC.STATUS & OSC_PLLRDY_bm) );                 // Wait for PLL is ready

  CCP = CCP_IOREG_gc;                                       // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;                            // Select system clock source
  OSC.CTRL &= ~OSC_RC2MEN_bm;                               // Turn off 2MHz internal oscillator
  OSC.CTRL &= ~OSC_RC32MEN_bm;                              // Turn off 32MHz internal oscillator
}
