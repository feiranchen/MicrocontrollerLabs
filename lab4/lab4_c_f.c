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
#include "lcd_lib.h"
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
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
const int8_t LCD_initialize[] PROGMEM = "LCD Initialized!\0";
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
volatile float rpm_isr;
volatile unsigned int fan_period;

// --- control LEDs from buttons and uart -------------------
// input arguments to each thread
// not actually used in this example
int args[3] ;

// shared S,P,I,D, RPM
uint8_t s_value; // sem 3
uint8_t p_value; // sem 4
uint8_t i_value; // sem 5
uint8_t d_value; // sem 6
volatile int RPM; // sem 7

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

	TCCR2B |= (1<<CS22);    // sets the prescaler to 64
end

void timer0_init(void)
begin
	TCCR0A = 0x00;
	TCCR0B = 0x00;
	TIMSK0 = 0x00;
	OCR0A = 150;    // sets up 0 duty cycle
	EICRA = 0x00;
	EIMSK = 0x00;

	EICRA |= (1<<ISC01);    // falling edge triggers INT0
	EIMSK |= (1<<INT0);    // enables INT0

	TCCR0A |= (1<<COM0A1) + (1<<WGM01) + (1<<WGM00);    // fast pwm
	TCCR0B |= (1<<CS01) + (1<<CS00);    // prescaler of 64 -> 976 cycles/sec
end

// write to LCD
void write_3digit_LCD(int num, int x, int y)
begin
	sprintf(lcd_buffer,"%4d", num);
	LCDGotoXY(x, y);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end

void write_string_LCD(char* str, int x, int y)
begin
	sprintf(lcd_buffer,"%12s\n", str);
	LCDGotoXY(x, y);
	LCDstring(lcd_buffer, strlen(lcd_buffer));
end


// --- define task 1  ----------------------------------------
void get_User_Input(void* args) 
  begin
  	uint32_t rel, dead ;
	uint16_t inputValue;
	char cmd[4] ;

	while(1)
	begin
		// commands:
		// 's 3' turns on led 3
		// 'c 4' turns off led 4
		// 't 1' toggles led 1
		fprintf(stdout, ">") ;
		fscanf(stdin, "%s %d", cmd, &inputValue) ;
		//trtWait(SEM_STRING_DONE);

		// update shared leds
		
		if (cmd[0] == 's')
		begin
			trtWait(SEM_SHARED_S) ;
			s_value = inputValue;
			trtSignal(SEM_SHARED_S);
		end
		if (cmd[0] == 'p')
		begin
			trtWait(SEM_SHARED_P) ;
			p_value = inputValue;
			trtSignal(SEM_SHARED_P);
		end
		if (cmd[0] == 'i')
		begin
			trtWait(SEM_SHARED_I) ;
			i_value = inputValue;
			trtSignal(SEM_SHARED_I);
		end
		if (cmd[0] == 'd')
		begin
			trtWait(SEM_SHARED_D) ;
			d_value = inputValue;
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
	int error, prev_error, sum_error, CF;
	char p, i, d;
	s_value = 1000;
	p = 70;
	i = 0;
	d = 0;
	prev_error = 0;

	while(1)
	begin
		prev_error = error;
		sum_error += error;
		// lock and look at error
		trtWait(SEM_SHARED_RPM);
		trtWait(SEM_SHARED_S);
		error = RPM-s_value;
		trtSignal(SEM_SHARED_S);
		trtSignal(SEM_SHARED_RPM);

		// check if error had a zero crossing and reset the i term
				
		// calculate CF
		CF = p * error + d * (error-prev_error) + i * (sum_error);
		if (CF>255) OCR0A = 255;
		if (CF<0) OCR0A = 0;
		if (CF<=255 && CF>=0) OCR0A = CF; 

		// Sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.05);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.08);
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
		rpm_isr = (float)fan_period*7;
		rpm_isr = 1/rpm_isr;

		trtWait(SEM_SHARED_RPM);
		RPM = (int)rpm_isr;
		write_3digit_LCD(RPM, 1, 1);
		trtSignal(SEM_SHARED_RPM);

		// Sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.2);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.3);
	    trtSleepUntil(rel, dead);
	end
  end

// pin change interrupt on D.2. Initialized in task 2
ISR(INT0_vect)
begin
	fan_period = TCNT2;
    TCNT2 = 0;
end


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
  trtCreateTask(get_User_Input, 100, SECONDS2TICKS(0.1), SECONDS2TICKS(0.1), &(args[0]));
  trtCreateTask(get_User_Input, 100, SECONDS2TICKS(0.1), SECONDS2TICKS(0.1), &(args[1]));
  trtCreateTask(get_Fan_Speed, 100, SECONDS2TICKS(0.05), SECONDS2TICKS(0.05), &(args[2]));
  
  
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
