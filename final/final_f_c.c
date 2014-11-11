/* Lab 5 - Circuit Rapid Prototyping
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
#include <avr/sleep.h>

#define begin {
#define end }


// LCD Globals
const int8_t LCD_initialize[] PROGMEM = "LCD Initialized!\0";
const int8_t LCD_printing[] PROGMEM = "Printing        \0";
const int8_t LCD_waiting[] PROGMEM = "Waiting for file\0";
const int8_t LCD_null[] PROGMEM = "                \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t keystr[17];
volatile char LCD_char_count;

// Plotter Head Globals
volatile unsigned int x_pos;
volatile unsigned int y_pos;
#define x_axis 7
#define y_axis 6

// Inits ----------------------------------------------------------------------
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
	ADCSRA = (1<<ADEN) + 6 ; 
end


void port_init(void)
begin

end

// Helper Functions -----------------------------------------------------------
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

// writes the X and Y positions of the head to the second LCD line
void print_position(void)
begin
	sprintf(lcd_buffer,"X: %3d  Y: %3d ",x_pos,y_pos);
	LCDGotoXY(0,1)
	LCDstring(lcd_buffer, strlen(lcd_buffer);
end

// draw a circle
void circle(void)
begin
	// do this

end


int main(void)
begin

	while(1)
		// while loop until there is a file waiting to be sent over Putty
			// update LCD to say "waiting for file input"

		// exit while loop once there is a file
			// save the coordinates of the vectors into progmem
			// update LCD to say "printing"

		// for each vector in the file
			// if x_pos != x_start 
				// move motor x using while statement from the ADC
			// end if x_pos

			// if y_pos != y_start
				// move motor y using while statement from the ADC
			// end if y_pos

			// turn on the solenoid to drop the pen (set D.7 high)
			
			// draw a circle or a square at the start of the vector

			// for each point in the vector
				// move motor x using while statement from the ADC
				// move motor y using while statement from the ADC
				// update the LCD with the current positions
			// end for each point

			// move both motors slightly to make a circle or square at the end of each vector
			
			// turn off the solenoid to raise the pen (set D.7 low)
		// end for each vector
	end // while(1)
	return 1;
end
