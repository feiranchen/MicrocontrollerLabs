/* Lab 3 - Particle Beam Game
	Connor Archard - cwa37
	Feiran Chen - fc254

*/

#define F_CPU 16000000UL
#include "lcd_lib.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <util/delay.h> // needed for lcd_lib
#define begin {
#define end }
typedef enum { false, true } bool;

// LCD globals
const int8_t LCD_initialize[] PROGMEM = "LCD Initialized!\0";
const int8_t LCD_burst_freq[] PROGMEM = "Burst Frequency:\0";
const int8_t LCD_interval[] PROGMEM =  "Chirp Interval: \0";
const int8_t LCD_num_syllable[] PROGMEM = "Num Syllables:  \0";
const int8_t LCD_dur_syllable[] PROGMEM = "Dur Syllables:  \0";
const int8_t LCD_rpt_interval[] PROGMEM = "Rpt interval:   \0";
const int8_t LCD_playing[] PROGMEM = "Chirp, Chirp    \0";
const int8_t LCD_cap_clear[] PROGMEM = "            \0";

volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t keystr[17];
volatile char LCD_char_count;

// Fixed Point Maths
#define int2fix(a)   (((int)(a))<<8)            //Convert char to fix. a is a char
#define fix2int(a)   ((signed char)((a)>>8))    //Convert fix to char. a is an int
#define float2fix(a) ((int)((a)*256.0))         //Convert float to fix. a is a float
#define fix2float(a) ((float)(a)/256.0)         //Convert fix to float. a is an int 

void LCD_init(void)
begin
	// start the LCD 
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
	CopyStringtoLCD(LCD_initialize, 0, 0);
	LCD_char_count = 0;
end


void ADC_init(void)
begin
	ADMUX = 0;
	ADCSRA = 0;

	ADMUX = (1<<REFS0) + (1<<ADLAR);
	ADCSRA = (1<<ADEN) + 7 ; 
end


void port_init(void)
begin
	DDRA = 0x00;    // all of PORTA is an input to avoid coupling with ADC meas
	PORTA = 0x00;    // no pull-up resistors to avoid coupling
end


// performs an ADC on the selected channel.
void ADC_start_measure(char channel)
begin
	ADMUX = 0;
	ADMUX = (1<<REFS0) + (1<<ADLAR) + channel;
	ADCSRA |= (1<<ADSC);
end


// write to LCD
void write_LCD(char num)
begin
	sprintf(lcd_buffer,"%3d", num);
	LCDGotoXY(0, 1);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end


void initialize(void)
begin
	ADC_init();
	LCD_init();
	port_init();
end


int main(void)
begin
	char temp = 0;
	initialize();

	// ADC Test Code
	while(1)
	begin
	ADC_start_measure(0);
	write_LCD(temp);
	_delay_ms(50);
	temp = ADCH;
	end

	return 1;
end
