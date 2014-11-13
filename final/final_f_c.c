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
#define x_axis 0
#define y_axis 1

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

	ADMUX = (1<<REFS0);
	ADCSRA = (1<<ADEN) + 7; 
end


void port_init(void)
begin
	DDRA = 0x00;    // all inputs to avoid ADC coupling, no pull ups
	DDRD = 0xff;    // all outputs - bottom 2 are USART top 6 are motor control
	PORTA = 0x00;    // no pull up resistors
	PORTD = 0x00;    // start with no power
end

void initialize(void)
begin
	port_init();
	LCD_init();
	ADC_init();

	sei();
end

// ISRS -----------------------------------------------------------------------


// Helper Functions -----------------------------------------------------------
// performs an ADC on the selected channel.
void ADC_start_measure(char channel)
begin
	ADMUX = 0;
	ADMUX = (1<<REFS1) + (1<<REFS0) + channel;
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
	sprintf(lcd_buffer,"X: %-i ",x_pos);  
	LCDGotoXY(0,1);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
	sprintf(lcd_buffer,"Y: %-i ",y_pos);
	LCDGotoXY(8,1);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end


void move_positive_x(void)
begin
	PORTD &= ~0x08;
	_delay_us(5);
	PORTD |= 0x10;
end

void move_negative_x(void)
begin
	PORTD &= ~0x10;
	_delay_us(5);
	PORTD |= 0x08;
end

void move_positive_y(void)
begin
	PORTD &= ~0x40;
	_delay_us(5);
	PORTD |= 0x80;
end

void move_negative_y(void)
begin
	PORTD &= ~0x80;
	_delay_us(5);
	PORTD |= 0x40;
end

void stop_x(void)
begin
	// checks previous direction and provides a short pulse backwards to slow motor
	if (PIND & 0x10)
	begin
		move_negative_x();
		_delay_us(5);
		PORTD &= ~0x08;
	end
	if (PIND & 0x08)
	begin
		move_positive_x();
		_delay_us(5);
		PORTD &= ~0x10;
	end

	PORTD &= ~0x18;
end

void stop_y(void)
begin
	if(PIND & 0x80)
	begin
		move_negative_y();
		_delay_us(5);
		PORTD &= ~0x40;
	end
	if(PIND & 0x40)
	begin
		move_positive_y();
		_delay_us(5);
		PORTD &= ~0x80;
	end

	PORTD &= ~0xc0;

end

// all motors coast to a stop
void stop_all(void)
begin
	PORTD &= 0x03;
end

// draw a circle
void circle(void)
begin
	move_positive_x();
	_delay_us(100);
	stop_all();
	move_positive_y();
	_delay_us(100);
	move_negative_x();
	_delay_us(100);
	stop_all();
	move_negative_y();
	_delay_us(75);
	stop_all();

	move_positive_x();
	_delay_us(60);
	stop_all();
	move_positive_y();
	_delay_us(60);
	move_negative_x();
	_delay_us(60);
	stop_all();
	move_negative_y();
	_delay_us(35);
	stop_all();

	move_positive_x();
	_delay_us(20);
	stop_all();
	move_positive_y();
	_delay_us(20);
	move_negative_x();
	_delay_us(20);
	stop_all();
	move_negative_y();
	_delay_us(10);
	stop_all();
end

int main(void)
begin
	initialize();
	while(1)
	begin
		// while loop until there is a file waiting to be sent over Putty
			// update LCD to say "waiting for file input"

		// exit while loop once there is a file
			// save the coordinates of the vectors into progmem
			// update LCD to say "printing"

		// for each vector in the file
				 // move motor x to start using while statement from the ADC
				ADC_start_measure(x_axis);
				while(ADCSRA & (1<<ADSC));
				x_pos = (int)ADCL;
				x_pos += (int)(ADCH*256);
				
				/*
				if (x_pos > x_start)
				begin
				

				end

				if (x_pos < x_start)
				begin
				

				end
				*/
		
				// move motor y to start using while statement from the ADC
				ADC_start_measure(y_axis);
				while(ADCSRA & (1<<ADSC));
				y_pos = (int)ADCL;
				y_pos += (int)(ADCH*256);
				
				/*
				if (y_pos > y_start)
				begin
				

				end

				if (y_pos < y_start)
				begin
				

				end
				*/

			print_position();

			// turn on the solenoid to drop the pen (set D.7 high)
			
			// draw a circle or a square at the start of the vector

			// for each point in the vector
				// move motor x using while statement from the ADC
				// move motor y using while statement from the ADC
				// update the LCD with the current positions
			// end for each point

			// draw a circle at the end
			
			// turn off the solenoid to raise the pen (set D.7 low)
		// end for each vector
	end // while(1)
	return 1;
end
