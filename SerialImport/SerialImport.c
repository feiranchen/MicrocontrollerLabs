
#include <stdio.h>
#include <avr/sleep.h>

// serial communication library
// Don't mess with the semaphores
#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>
#define F_CPU 16000000UL

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <float.h>
#include <math.h>
#include "lcd_lib.h"
#include <util/delay.h> // needed for lcd_lib
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include "uart.h"


// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// the usual
#define begin {
#define end }

// LCD globals
const int8_t LCD_initialize[] PROGMEM = "LCD Initialize  \0";
const int8_t LCD_line_clear[] PROGMEM = "                \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t lcd_buffer2[17];	// LCD display buffer
volatile char LCD_char_count;
volatile int x_vect[100];
volatile int y_vect[100];
volatile int d_vect[100];

//Helper functions
void LCD_init(void)
begin
	// start the LCD
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();	//clear the display
	LCDGotoXY(0,0);
	CopyStringtoLCD(LCD_initialize, 0, 0);
	LCD_char_count = 0;
end

// --- Main Program ----------------------------------
int main(void) {
  int i =0;
  int x=-2 ,y=-2,d=-2;// container for parsed ints
  char buffer[17];
  uint16_t file_size = 0;
  LCD_init();
  //init the UART -- uart_init() is in uart.c
  uart_init();
  stdout = stdin = stderr = &uart_str;

  // Allocate memory for the buffer	
  fprintf(stdout,"File Length\n\r");
  fscanf(stdin, "%d*", &file_size) ;
  sprintf(lcd_buffer2,"             %-i.", file_size);

	LCDGotoXY(0, 0);
	LCDstring(lcd_buffer2, strlen(lcd_buffer2));

  for (i=0; i<file_size; i++)
  begin

  	fprintf(stdout,"Hi\n\r");
	fscanf(stdin, "%s", buffer) ;
	sscanf(buffer, "X%dY%dD%d", &x,&y,&d);

    sprintf(lcd_buffer2,"%-i  ", i);
	LCDGotoXY(10, 0);
	LCDstring(lcd_buffer2, 2);

	//print org
	LCDGotoXY(0, 1);
	LCDstring(buffer,15);

	//print parsed
	if (x>=-1 && y>=-1 && d>=-1){
		sprintf(lcd_buffer,"x%dy%dd%d", x,y,d);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
		x_vect[i] = x;
		y_vect[i] = y;
		d_vect[i] = d;
		x=-2;
		y=-2;
		d=-2;
	} else {
		sprintf(lcd_buffer,"Invalid Input@%-i", i);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
	}
	_delay_ms(1000);
  end
		_delay_ms(2000);
		sprintf(lcd_buffer,"finished%-i", i);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
		sprintf(lcd_buffer,"x%d%d%d%d", x_vect[0],  x_vect[1],  x_vect[2],  x_vect[3]);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
		sprintf(lcd_buffer,"y%d%d%d%d", y_vect[0],  y_vect[1],  y_vect[2],  y_vect[3]);
		LCDGotoXY(0, 1);
		LCDstring(lcd_buffer, 10);
		sprintf(lcd_buffer,"d%d%d%d%d", d_vect[0],  d_vect[1],  d_vect[2],  d_vect[3]);
		LCDGotoXY(10, 0);
		LCDstring(lcd_buffer, 10);
} // main
