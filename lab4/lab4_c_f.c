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
#include "trtUart.h"
#include "trtUart.c"
// UART file descriptor
// putchar and getchar are in uart.c
FILE uart_str = FDEV_SETUP_STREAM(uart_putchar, uart_getchar, _FDEV_SETUP_RW);

// semaphore to protect shared variable
#define SEM_SHARED 3

// the usual
#define begin {
#define end }

// --- control LEDs from buttons and uart -------------------
// input arguments to each thread
// not actually used in this example
int args[2] ;

// shared led status
uint8_t led ;

// --- define task 1  ----------------------------------------
void buttonComm(void* args) 
  begin	
  	uint32_t rel, dead ;
	uint8_t sw, sw_num ;
	uint8_t sw_state ;

	sw_state = 0 ; // no buttons pushed

	while(1)
	begin
		// read the buttons
		// if a button is pushed,
		// latch on the corresponding LED
		sw = ~PINB ;
		// update shared leds
		trtWait(SEM_SHARED) ;
		led = led | sw ;
		PORTC = ~led ;
		trtSignal(SEM_SHARED);
		
		// chessy debouncer
		if (sw_state == 0 && sw!=0) // new button push?
		begin
			// convert from binary to switch number (0 to 7)
			sw_num = 0 ;
			while(sw>1)
			begin
				sw = sw>>1; 
				sw_num++ ;
			end
			fprintf(stdout,"Button pushed=%d\n\r>", sw_num) ;
			sw_state = 1 ;
		end
		if (sw_state == 1 && sw==0) // button release?
		 	sw_state = 0 ;
		// end debouncer

		// Sleep
		// debouncer works well with 50 mSec sleep
	    rel = trtCurrentTime() + SECONDS2TICKS(0.05);
	    dead = trtCurrentTime() + SECONDS2TICKS(0.05);
	    trtSleepUntil(rel, dead);
	end
  end

// --- define task 2  ----------------------------------------
void serialComm(void* args) 
  begin
	uint16_t ledNum ;
	char cmd[4] ;
	
	// turn off leds
	led = 0x00 ;

	while(1)
	begin
		// commands:
		// 's 3' turns on led 3
		// 'c 4' turns off led 4
		// 't 1' toggles led 1
		fprintf(stdout, ">") ;
		fscanf(stdin, "%s%d", cmd, &ledNum) ;
		//trtWait(SEM_STRING_DONE);

		// update shared leds
		trtWait(SEM_SHARED) ;	
		if (cmd[0] == 's')
			led  |= (1<<ledNum) ;
		if (cmd[0] == 'c')		
			led  &= ~(1<<ledNum) ;
		if (cmd[0] == 't')		
			led  ^= (1<<ledNum) ;
		PORTC = ~led ;
		trtSignal(SEM_SHARED);

	end
  end

// --- Main Program ----------------------------------
int main(void) {

  DDRC = 0xff;    // led connections
  PORTC = 0xff;
  DDRB = 0x00 ; 
  PORTB = 0xff ; // button pullups on

  //init the UART -- trt_uart_init() is in trtUart.c
  trt_uart_init();
  stdout = stdin = stderr = &uart_str;
  fprintf(stdout,"\n\r TRT 9feb2009\n\r\n\r");

  // start TRT
  trtInitKernel(80); // 80 bytes for the idle task stack

  // --- create semaphores ----------
  // You must creat the first two semaphores if you use the uart
  trtCreateSemaphore(SEM_RX_ISR_SIGNAL, 0) ; // uart receive ISR semaphore
  trtCreateSemaphore(SEM_STRING_DONE,0) ;  // user typed <enter>
  
  // variable protection
  trtCreateSemaphore(SEM_SHARED, 1) ; // protect shared variables

 // --- creat tasks  ----------------
  trtCreateTask(buttonComm, 100, SECONDS2TICKS(0.05), SECONDS2TICKS(0.05), &(args[0]));
  trtCreateTask(serialComm, 100, SECONDS2TICKS(0.1), SECONDS2TICKS(0.1), &(args[1]));
  
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
