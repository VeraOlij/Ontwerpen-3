#ifndef SERIALF0_H_
#define SERIALF0_H_

#include <stdio.h>

#define TXBUF_DEPTH_F0    100      
#define RXBUF_DEPTH_F0    100      

#define UART_NO_DATA      0x0100                      
#define clear_screen()    printf("\e[H\e[2J\e[3J");   

char     *getline(char* buf,  uint16_t len);
void      init_stream(uint32_t f_cpu);
uint16_t  uartF0_getc(void);
void      uartF0_putc(uint8_t data);
void      uartF0_puts(char *s);

#endif // SERIALF0_H_ 