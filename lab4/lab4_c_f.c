// test TRT with two tasks:
// Task 1  reads the buttons (PORTB), 
//         latches on the corresponding LED (PORTC),
//         and send a message to the uart console
// Task 2  reads a command from the uart console
//         sets, clears, or toggles an LED,
//         command format is
//		   the char s, c or t followed by a number 
//		   between 0 and 7, e.g. 's 0'

#include "trtSettings.h"
#include "trtkernel_1284.c"
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
#include "trtUart.h"
#include "trtUart.c"

// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// semaphore to protect shared variables
#define SEM_SHARED_S 3
#define SEM_SHARED_P 4
#define SEM_SHARED_I 5
#define SEM_SHARED_D 6
#define SEM_SHARED_RPM 7

// the usual
#define begin {
#define end }

// LCD globals
const int8_t LCD_initialize[] PROGMEM = "LCD Initiali    \0";
const int8_t LCD_burst_freq[] PROGMEM = "Burst Frequency:\0";
const int8_t LCD_interval[] PROGMEM = "Chirp Interval: \0";
const int8_t LCD_num_syllable[] PROGMEM = "Num Syllables: \0";
const int8_t LCD_dur_syllable[] PROGMEM = "Dur Syllables: \0";
const int8_t LCD_rpt_interval[] PROGMEM = "Rpt interval: \0";
const int8_t LCD_playing[] PROGMEM = "Chirp, Chirp \0";
const int8_t LCD_cap_clear[] PROGMEM = " \0";
volatile int8_t lcd_buffer[17];	// LCD display buffer
volatile int8_t keystr[17];
volatile char LCD_char_count;

// rpm variables
volatile unsigned int fan_period;

int args[3] ;

// shared S,P,I,D, RPM
volatile float s_value; // sem 3
volatile float p_value; // sem 4
volatile float i_value; // sem 5
volatile float d_value; // sem 6
uint16_t RPM; // sem 7
volatile uint16_t motor_period_ovlf;

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

// sets timer2 to be a counter
void timer2_init(void)
begin
	TCCR2A = 0x00;
	TCCR2B = 0x00;
	TIMSK2 = 0x00;

	TIMSK2 |= (1<<TOIE2);    // enables the overflow ISR
	TCCR2B |= (1<<CS22) + (1<<CS21);// + (1<<CS20);    // sets the prescaler to 256
end

void timer0_init(void)
begin
	TCCR0A = 0x00;
	TCCR0B = 0x00;
	TIMSK0 = 0x00;
	OCR0A = 0;    // sets up 0 duty cycle
	EICRA = 0x00;
	EIMSK = 0x00;

	EICRA |= (1<<ISC01);    // falling edge triggers INT0
	EIMSK |= (1<<INT0);    // enables INT0

	TCCR0A |= (1<<COM0A1) + (1<<COM0B1) + (1<<WGM01) + (1<<WGM00);    // fast pwm
	TCCR0B |= (1<<CS01) + (1<<CS00);    // prescaler of 64 -> 976 cycles/sec
end

/*
// write to LCD
void write_3digit_LCD(int num, int x, int y)
begin
	sprintf(lcd_buffer,"%4.4d", num);
	LCDGotoXY(x, y);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end

// write to LCD
void write_LCD(int num)
begin
	sprintf(lcd_buffer,"%-i", num);
	LCDGotoXY(0, 1);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end

void write_string_LCD(char* str, int x, int y)
begin
	sprintf(lcd_buffer,"%12s\n", str);
	LCDGotoXY(x, y);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end
*/

// --- define task 1  ----------------------------------------
void get_User_Input(void* args) 
  begin
  	uint32_t rel, dead ;
	int inputValue;
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

		// update shared leds
		
		if (cmd[0] == 's')
		begin
			trtWait(SEM_SHARED_S) ;
			s_value = finputValue;
			fprintf(stdout,"value of s changed to %.1f\n\n",finputValue);
			trtSignal(SEM_SHARED_S);
		end
		if (cmd[0] == 'p')
		begin
			trtWait(SEM_SHARED_P) ;
			p_value = finputValue;
			fprintf(stdout,"value of p changed to %.1f\n\n",finputValue);
			trtSignal(SEM_SHARED_P);
		end
		if (cmd[0] == 'i')
		begin
			trtWait(SEM_SHARED_I) ;
			i_value = finputValue;
			fprintf(stdout,"value of i changed to %.1f\n\n",finputValue);
			trtSignal(SEM_SHARED_I);
		end
		if (cmd[0] == 'd')
		begin
			trtWait(SEM_SHARED_D) ;
			d_value = finputValue;
			fprintf(stdout,"value of d changed to %.1f\n\n",finputValue);
			trtSignal(SEM_SHARED_D);
		end
		
		// Sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.1);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.3);
	    trtSleepUntil(rel, dead);

	end
  end

// --- define task 2  ----------------------------------------
void calc_PWM_Const(void* args) 
  begin	
  	uint32_t rel, dead ;
	signed int error, prev_error, sum_error, temp; 
	float CF,i_calc;
	signed int p, i, d;
	float rpm_isr;

	s_value = 1000; // <------------------------------------- This is a test statement only
	p = 0;
	i = 0;
	d = 0;
	p_value = 4;
	i_value = .25;
	d_value = .3;
	error = 0;
	OCR0A = 150;
	prev_error = 0;

	while(1)
	begin
		temp = fan_period*7;    // ticks for one rotation
		rpm_isr = 62500 * 60 /temp;    // divide 60 seconsd by rotations/sec for rpm
		
		prev_error = error;
		
		// lock and look at error
		trtWait(SEM_SHARED_RPM);
		RPM = (int)rpm_isr;    // saves the calculated value into a global that LCD func can use
		trtWait(SEM_SHARED_S);
		error = (int)s_value - RPM;
		trtSignal(SEM_SHARED_S);
		trtSignal(SEM_SHARED_RPM);

		// check if error had a zero crossing and reset the i term
		if((error>0 && prev_error>0) || (error<0 && prev_error<0))
		begin
			sum_error += error;
		end
		else sum_error = 0;

		// calculate CF
		/*
		trtWait(SEM_SHARED_P);
		p = p_value;
		trtSignal(SEM_SHARED_P);

		trtWait(SEM_SHARED_I);
		i = i_value;
		trtSignal(SEM_SHARED_I);

		trtWait(SEM_SHARED_D);
		d = d_value;
		trtSignal(SEM_SHARED_D);*/
		
		
		trtWait(SEM_SHARED_P);
		trtWait(SEM_SHARED_I);
		trtWait(SEM_SHARED_D);
		//CF = p * error + d * (error-prev_error) + i * (sum_error);
		i_calc = i_value * (sum_error) > 50 ? 50 : i_value * (sum_error);
		i_calc = i_value * (sum_error) < -50? -50 : i_value * (sum_error);
		CF = p_value * error + d_value * (error-prev_error) + i_calc;
		trtSignal(SEM_SHARED_P);
		trtSignal(SEM_SHARED_I);
		trtSignal(SEM_SHARED_D);

		if (CF>255) OCR0A = 255;
		if (CF<0) OCR0A = 0;
		if (CF<=255 && CF>=0) OCR0A = (char)CF; 
		
		OCR0B = (char)(rpm_isr*.0853); // set for the Oscope measurement

		// Sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.01);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.04);
	    trtSleepUntil(rel, dead);
	end
  end

// --- define task 3  ----------------------------------------
// writes the desired fan speed and the current fan speed to the LCD
// approx five times a second
void get_Fan_Speed(void* args) 
  begin	
  	uint32_t rel, dead ;
	timer2_init();
	timer0_init();    // sets up the fast pwm
	LCD_init();    // init LCD for our use
	port_init();    // init port c

	while(1)
	begin
		trtWait(SEM_SHARED_S) ;
		sprintf(lcd_buffer,"input RPM: %-i ", (int)s_value);
		//sprintf(lcd_buffer,"%d, %d", (int)s_value, RPM);
		trtSignal(SEM_SHARED_S) ;
		LCDGotoXY(0, 0);
		LCDstring(lcd_buffer, strlen(lcd_buffer));

		trtWait(SEM_SHARED_RPM);
		sprintf(lcd_buffer,"fan RPM: %-i   ", RPM);
		//sprintf(lcd_buffer,"err %d %d", error, (int)d_value * (error-prev_error));
		//sprintf(lcd_buffer," %-i %-i %-i",(int)p_value, (int)i_value,(int)d_value);
		trtSignal(SEM_SHARED_RPM);
		LCDGotoXY(0, 1);
		LCDstring(lcd_buffer, strlen(lcd_buffer));
		

		// Sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.1);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.3);
	    trtSleepUntil(rel, dead);
	end
  end

// pin change interrupt on D.2. Initialized in task 2
ISR(INT0_vect)
begin
	fan_period = TCNT2 + motor_period_ovlf;
    TCNT2 = 0;
	motor_period_ovlf = 0 ;
end

// --- set up extra 8 bits on timer 2 ----------------
ISR (TIMER2_OVF_vect) {
        motor_period_ovlf = motor_period_ovlf + 256 ;
}

// --- Main Program ----------------------------------
int main(void) {
  //init the UART -- trt_uart_init() is in trtUart.c
  trt_uart_init();
  stdout = stdin = stderr = &uart_str;
  fprintf(stdout,"\n\r Hi\n\r\n\r");

  // start TRT
  trtInitKernel(140); // 80 bytes for the idle task stack

  // --- create semaphores ----------
  // You must creat the first two semaphores if you use the uart
  trtCreateSemaphore(SEM_RX_ISR_SIGNAL, 0) ; // uart receive ISR semaphore
  trtCreateSemaphore(SEM_STRING_DONE,0) ;  // user typed <enter>
  
  // variable protection
  trtCreateSemaphore(SEM_SHARED_S, 1) ; // protect shared variables
  trtCreateSemaphore(SEM_SHARED_P, 1) ; // protect shared variables
  trtCreateSemaphore(SEM_SHARED_I, 1) ; // protect shared variables
  trtCreateSemaphore(SEM_SHARED_D, 1) ; // protect shared variables
  trtCreateSemaphore(SEM_SHARED_RPM, 1) ; // protect shared variables


 // --- creat tasks  ----------------
  trtCreateTask(get_User_Input, 1000, SECONDS2TICKS(0.01), SECONDS2TICKS(0.1), &(args[0]));
  trtCreateTask(calc_PWM_Const, 1000, SECONDS2TICKS(0.01), SECONDS2TICKS(0.05), &(args[1]));
  trtCreateTask(get_Fan_Speed, 1000, SECONDS2TICKS(0.05), SECONDS2TICKS(0.2), &(args[2]));
  
  
  // --- Idle task --------------------------------------
  // just sleeps the cpu to save power 
  // every time it executes
  set_sleep_mode(SLEEP_MODE_IDLE);
  sleep_enable();
  while (1) 
  begin
  	sleep_cpu();
  end

} // main
