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
const int8_t LCD_hello[] PROGMEM = "hello world     \0";
const int8_t LCD_move[] PROGMEM = "about to move   \0";
const int8_t LCD_moving[] PROGMEM = "moving          \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t keystr[17];
volatile char LCD_char_count;

// Plotter Head Globals
volatile unsigned int x_pos;
volatile unsigned int y_pos;
#define x_axis 0
#define y_axis 1
volatile int x_vect[100] = {40,400,200,400,100,50,340,40,-1};
volatile int y_vect[100] = {40,400,420,100,100,400,250,300,-1};

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

void raise_pen(void)
begin
	PORTD &= ~0x20;
	_delay_ms(500);
end

void lower_pen(void)
begin
	PORTD |= 0x20;
	_delay_ms(400);
end

void move_negative_x(void)
begin
	PORTD &= 0xf7;
	_delay_us(5);
	PORTD |= 0x04;
end

void move_positive_x(void)
begin
	PORTD &= 0xfb;
	_delay_us(5);
	PORTD |= 0x08;
end

void move_negative_y(void)
begin
	PORTD &= 0xbf;
	_delay_us(5);
	PORTD |= 0x80;
end

void move_positive_y(void)
begin
	PORTD &= 0x7f;
	_delay_us(5);
	PORTD |= 0x40;
end

void stop_x(void)
begin
	PORTD &= ~0x18;
end

void stop_y(void)
begin
	PORTD &= ~0xc0; 

end

// all motors coast to a stop
void stop_all(void)
begin
	PORTD &= 0x23;
	_delay_ms(100);
end

// draw a circle
void circle(void)
begin
	move_positive_x();
	_delay_us(4000);
	stop_all();
	move_positive_y();
	_delay_us(4000);
	move_negative_x();
	_delay_us(4000);
	stop_all();
	move_negative_y();
	_delay_us(3000);
	stop_all();

	move_positive_x();
	_delay_us(2400);
	stop_all();
	move_positive_y();
	_delay_us(2400);
	move_negative_x();
	_delay_us(2400);
	stop_all();
	move_negative_y();
	_delay_us(1500);
	stop_all();

	move_positive_x();
	_delay_us(1000);
	stop_all();
	move_positive_y();
	_delay_us(1000);
	move_negative_x();
	_delay_us(1000);
	stop_all();
	move_negative_y();
	_delay_us(700);
	stop_all();
end

// 1= pen down, 2= pen up
move_to_XY(int x_in, int y_in, int d)
begin
	if (d==2) raise_pen();
	if (d==1) lower_pen();
	if(x_in>0 && y_in>0)
	begin
		// move to x position
		ADC_start_measure(x_axis);
		while(ADCSRA & (1<<ADSC));
		x_pos = (int)ADCL;
		x_pos += (int)(ADCH*256);

		if (x_pos > x_in)
		begin
			while(x_pos > x_in)
			begin
				ADC_start_measure(x_axis);
				while(ADCSRA & (1<<ADSC))move_negative_x();
				x_pos = (int)ADCL;
				x_pos += (int)(ADCH*256);
			end
			stop_all();
		end

		else
		begin
			while(x_pos < x_in)
			begin
				ADC_start_measure(x_axis);
				while(ADCSRA & (1<<ADSC))move_positive_x();
				x_pos = (int)ADCL;
				x_pos += (int)(ADCH*256);
			end
			stop_all();
		end
	
		// move to y position
		ADC_start_measure(y_axis);
		while(ADCSRA & (1<<ADSC));
		y_pos = (int)ADCL;
		y_pos += (int)(ADCH*256);

		if (y_pos > y_in)
		begin
			while(y_pos > y_in)
			begin
				ADC_start_measure(y_axis);
				while(ADCSRA & (1<<ADSC))move_negative_y();
				y_pos = (int)ADCL;
				y_pos += (int)(ADCH*256);
			end
			stop_all();
		end

		else
		begin
			while(y_pos < y_in)
			begin
				ADC_start_measure(y_axis);
				while(ADCSRA & (1<<ADSC))move_positive_y();
				y_pos = (int)ADCL;
				y_pos += (int)(ADCH*256);
			end
			stop_all();
		end
	end
	// print where you end up
	print_position();			
end

int main(void)
begin
	char i = 0;
	initialize();

	CopyStringtoLCD(LCD_hello, 0, 0);
	_delay_ms(1000);
	move_to_XY(x_vect[0],y_vect[0],2);
	for(i=1;i<100;i++)
	begin
		if(x_vect[i]>=0 && y_vect[i] >= 0)
		begin
			move_to_XY(x_vect[i],y_vect[i],1);
		end
		else
		begin
			break;
		end
	end
	move_to_XY(700,700,2);


	return 1;
end
