
#include <stdio.h>
#include <avr/sleep.h>

// serial communication library
// Don't mess with the semaphores
#define SEM_RX_ISR_SIGNAL 1
#define SEM_STRING_DONE 2 // user hit <enter>
#define F_CPU 16000000UL

#define x_axis 0
#define y_axis 1

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
const int8_t LCD_hello[] PROGMEM = "hello world     \0";
const int8_t LCD_wait1[] PROGMEM = "waiting for user\0";
const int8_t LCD_wait2[] PROGMEM = "to press button \0";
const int8_t LCD_printing[] PROGMEM = "printing        \0";
const int8_t LCD_df1[] PROGMEM = "downloading     \0";
const int8_t LCD_df2[] PROGMEM = "frame           \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t lcd_buffer2[17];	// LCD display buffer
volatile char LCD_char_count;
volatile int x_vect[100];
volatile int y_vect[100];
volatile int d_vect[100];
volatile unsigned int x_pos;
volatile unsigned int y_pos;
enum {IDLE =1, PULLING_FRAME, DRAWING};

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
	DDRD = 0xef;    // all outputs - bottom 2 are USART top 6 are motor control
	PORTA = 0x00;    // no pull up resistors
	PORTD = 0x10;    // start with no power and a pullup on D.4
	
end

void initialize(void)
begin
	port_init();
	LCD_init();
	ADC_init();
end


// Helper Functions -----------------------------------------------------------
// performs an ADC on the selected channel.
void ADC_start_measure(char channel)
begin
	ADMUX = 0;
	ADMUX = (1<<REFS1) + (1<<REFS0) + channel;
	ADCSRA |= (1<<ADSC);
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

void move_positive_y(void)
begin
	PORTD &= 0xbf;
	_delay_us(5);
	PORTD |= 0x80;
end

void move_negative_y(void)
begin
	PORTD &= 0x7f;
	_delay_us(5);
	PORTD |= 0x40;
end

// all motors coast to a stop
void stop_all(void)
begin
	PORTD &= 0x33;
	_delay_ms(100);
end

void stop_x(void)
begin
	PORTD &= 0xf3;
end

void stop_y(void)
begin
	PORTD &= 0x3f;
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
// 0= both, 1= x only, 2= y only
move_to_XY(int x_in, int y_in, int d, char motion)
begin
	if (d==2) raise_pen();
	if (d==1) lower_pen();
	if(x_in>0 && y_in>0)
	begin
		if(x_in>700) x_in = 700;
		if(y_in>700) y_in = 700;
		if(x_in<15) x_in = 15;
		if(y_in<15) y_in = 15;

		if(motion == 0)
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
					while(ADCSRA & (1<<ADSC)) stop_x();
					move_negative_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(x_pos < x_in)
				begin
					ADC_start_measure(x_axis);
					while(ADCSRA & (1<<ADSC))stop_x();
					move_positive_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
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
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_negative_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(y_pos < y_in)
				begin
					ADC_start_measure(y_axis);
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_positive_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end
	
		if (motion == 1)
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
					while(ADCSRA & (1<<ADSC)) stop_x();
					move_negative_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(x_pos < x_in)
				begin
					ADC_start_measure(x_axis);
					while(ADCSRA & (1<<ADSC))stop_x();
					move_positive_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end

		if (motion == 2)
		begin
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
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_negative_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(y_pos < y_in)
				begin
					ADC_start_measure(y_axis);
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_positive_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end
	end	
	// print where you end up
	print_position();			
end

// 1= pen down, 2= pen up
// 0= both, 1= x only, 2= y only
move_back_XY(int x_in, int y_in, int d, char motion)
begin
	if (d==2) raise_pen();
	if (d==1) lower_pen();
	if(x_in>0 && y_in>0)
	begin
		if(x_in>700) x_in = 700;
		if(y_in>700) y_in = 700;
		if(x_in<15) x_in = 15;
		if(y_in<15) y_in = 15;
		if(motion == 0)
		begin
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
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_negative_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(y_pos < y_in)
				begin
					ADC_start_measure(y_axis);
					while(ADCSRA & (1<<ADSC)); stop_y();
					move_positive_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

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
					while(ADCSRA & (1<<ADSC)) stop_x();
					move_negative_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(x_pos < x_in)
				begin
					ADC_start_measure(x_axis);
					while(ADCSRA & (1<<ADSC))stop_x();
					move_positive_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end

		if (motion == 1)
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
					while(ADCSRA & (1<<ADSC)) stop_x();
					move_negative_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(x_pos < x_in)
				begin
					ADC_start_measure(x_axis);
					while(ADCSRA & (1<<ADSC))stop_x();
					move_positive_x();
					x_pos = (int)ADCL;
					x_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end
		
		if (motion == 2)
		begin
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
					while(ADCSRA & (1<<ADSC)) stop_y();
					move_negative_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end

			else
			begin
				while(y_pos < y_in)
				begin
					ADC_start_measure(y_axis);
					while(ADCSRA & (1<<ADSC)); stop_y();
					move_positive_y();
					y_pos = (int)ADCL;
					y_pos += (int)(ADCH*256);
					_delay_us(500);
				end
				stop_all();
			end
		end
	end
	// print where you end up
	print_position();			
end

void get_frame()
begin
  int i=0, x=-2 ,y=-2,d=-2;// container for parsed ints
  char buffer[17];
  uint16_t file_size = 0;
 sprintf(lcd_buffer2,"File Length\n\r");
  fprintf(stdout,"%s\0", lcd_buffer2);
  fscanf(stdin, "%d*", &file_size) ;
  sprintf(lcd_buffer2,"             %-i.", file_size);

	LCDGotoXY(0, 0);
	LCDstring(lcd_buffer2, strlen(lcd_buffer2));

  for (i=0; i<file_size; i++)
  begin

  	fprintf(stdout,"Hi\n\r");
	fscanf(stdin, "%s", buffer) ;
	sscanf(buffer, "X%dY%dD%d", &x,&y,&d);

    sprintf(lcd_buffer2,"%-i ", i);
	LCDGotoXY(11, 0);
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
		sprintf(lcd_buffer,"Invalid@%-i", i);
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, 10);
	}
	//_delay_ms(1000);
  end
	//	_delay_ms(2000);
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

end

void move_motor()
begin
	int i =0;
	//_delay_ms(1000);
	//CopyStringtoLCD(LCD_hello, 0, 0);
	//_delay_ms(1000);
	move_to_XY(x_vect[0],y_vect[0],2,0);
	for(i=1;i<100;i++)
	begin
		if(x_vect[i]>=0 && y_vect[i] >= 0)
		begin
			if(x_vect[i] == x_vect[i-1])
			begin
				move_to_XY(x_vect[i],y_vect[i],d_vect[i],2);
				if (d_vect[i] == 1)
				begin
					move_back_XY(x_vect[i-1],y_vect[i-1],1,2);
					move_to_XY(x_vect[i],y_vect[i],1,2);
				end
			end
			else 
			begin
				if(y_vect[i] == y_vect[i-1])
				begin
					move_to_XY(x_vect[i],y_vect[i],d_vect[i],1);
					if (d_vect[i] == 1)
					begin
						move_back_XY(x_vect[i-1],y_vect[i-1],1,1);
						move_to_XY(x_vect[i],y_vect[i],1,1);
					end
				end
				else
				begin
					move_to_XY(x_vect[i],y_vect[i],d_vect[i],0);
					if (d_vect[i] == 1)
					begin
						move_back_XY(x_vect[i-1],y_vect[i-1],1,0);
						move_to_XY(x_vect[i],y_vect[i],1,0);
					end
				end
			end
		end
		else
		begin
			break;
		end
	end
	move_to_XY(700,700,2,0);
end

// --- Main Program ----------------------------------
int main(void) {
  
  initialize();
  stop_all();
	LCD_init();
  //init the UART -- uart_init() is in uart.c
  uart_init();
  stdout = stdin = stderr = &uart_str;
//while(1) move_positive_y();
  while(1)
  begin
  	move_to_XY(700,700,2,0);
	CopyStringtoLCD(LCD_wait1, 0, 0);
	CopyStringtoLCD(LCD_wait2, 0, 1);
  	while(PIND & 0x10);
	_delay_ms(30);
	if(!(PIND&0x10))
	begin
		CopyStringtoLCD(LCD_df1, 0, 0);
		CopyStringtoLCD(LCD_df2, 0, 1);
	  	get_frame();
		CopyStringtoLCD(LCD_printing, 0, 0);
		move_motor();
	end
  end

// TODO:
// button trigger pulling
// Gerber file trasmittion
// Testing on multiple frames.
//
}
