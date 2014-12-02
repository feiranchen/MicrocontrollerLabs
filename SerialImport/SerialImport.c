
#include <stdio.h>
#include <avr/sleep.h>

// serial communication library
// Don't mess with the semaphores
#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>
#define F_CPU 16000000UL
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include "lcd_lib.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h> // needed for lcd_lib
#include "uart.h"


// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// the usual
#define begin {
#define end }

// LCD globals
const int8_t LCD_initialize[] PROGMEM = "LCD Initialize  \0";
const int8_t LCD_burst_freq[] PROGMEM = "Burst Frequency:\0";
const int8_t LCD_interval[] PROGMEM = "Chirp Interval: \0";
const int8_t LCD_num_syllable[] PROGMEM = "Num Syllables: \0";
const int8_t LCD_dur_syllable[] PROGMEM = "Dur Syllables: \0";
const int8_t LCD_rpt_interval[] PROGMEM = "Rpt interval: \0";
const int8_t LCD_playing[] PROGMEM = "Chirp, Chirp \0";
const int8_t LCD_line_clear[] PROGMEM = "                \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t lcd_buffer2[17];	// LCD display buffer
volatile int8_t keystr[17];
volatile char LCD_char_count;

int args[3] ;

//Helper functions
void port_init(void)
begin
	DDRA = 0x00; // all of PORTA is an input to avoid coupling with ADC meas
	PORTA = 0x00; // no pull-up resistors to avoid coupling
	DDRC = 0xff; // all output
	PORTC = 0x00;
	DDRB = 0xff; // all output esp port B.3
	PORTB = 0x00;
	DDRD &= ~0x04; // d.2 is an input
	PORTD |= 0x04; // pull-up resistor on d.2 
end

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

void get_User_Input(void* args) 
begin
  	//uint32_t rel, dead ;
	//int inputValue;
	float finputValue;////////////////////update at one point
	char cmd[4] ;

	while(1)
	begin
		// commands:
		// 's 3' turns on led 3
		// 'c 4' turns off led 4
		// 't 1' toggles led 1
		fprintf(stdout, ">") ;
		fscanf(stdin, "%s %f", cmd, &finputValue) ;
		//trtWait(SEM_STRING_DONE);


	end
end




// --- Main Program ----------------------------------
int main(void) {
  int i =0;
  int x=-2 ,y=-2,d=-2;// container for parsed ints
  char buffer[17];
  uint16_t file_size = 0;
  char* file;
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
	sscanf(buffer, "X%dY%dD%d*", &x,&y,&d);

    sprintf(lcd_buffer2,"%-i  ", i++);
	LCDGotoXY(10, 0);
	LCDstring(lcd_buffer2, 2);

	//print org
	LCDGotoXY(0, 1);
	LCDstring(buffer,15);

	//print parsed
	if (x>0 && y>0 && d>0){
		sprintf(lcd_buffer,"x%dy%dd%d", x,y,d);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
	}
	//_delay_ms(1000);
  end


} // main
