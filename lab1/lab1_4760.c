/* ECE 4760 Lab 1
	Connor Archard	cwa37
	Feiran Chen		fc254
	
	Rev 1
	9/6/14 
	20:00 
*/
/** 
 * Use an 2x16 alphanumeric LCD connected to PORTC as follows:
 *  
 *	[LCD] -	[Mega644 Pin]
 *	1 GND -	GND
 *	2 +5V -	VCC
 *	3 VLC 10k trimpot wiper (trimpot ends go to +5 and gnd) 
 *	4 RS -	PC0
 *	5 RD -	PC1
 *	6 EN -	PC2
 *	11 D4 -	PC4
 *	12 D5 -	PC5
 *	13 D6 -	PC6
 *	14 D7 -	PC7 
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
#include <util/delay.h> // needed for lcd_lib
#define begin {
#define end }

// Global variable declarations
volatile char t0_count;    // counts the mS since last task1 call as updated by timer0
volatile char ready;	// set to 1 every ~200mS

int main(void) 
begin
	initialize();

	while(1)
	begin
		if (ready) 
		begin
		capCount = task1();	// measures capacitance
		task2(capCount);	// displays capacitance on LCD
		ready = 0;
		end	
	end
end

/* Initialize all of the output ports
portA - unused
portB - start with B.3 as input and others as output
portC - all output for LCD
portD - unused
*/
void portInit(void)
begin
DDRA = 0xff;    // PORTA is unused and left to output low
DDRB = 0xf7;    // sets B.3 to an input
DDRC = 0xff;    // PORTC is used for the LCD and needs to output
DDRD = 0xff;    // PORTD is unused and left to output low

PORTA = 0x00;	// all of PORTA is output low
PORTB = 0x08;   // B.3 has pull-up on. The rest of PORTB is output low
PORTC = 0x00;   // PORTC is initialized to output low. LCD code will change
PORTD = 0x00;   // PORTD is output low to save power
end


/* Initializes the AOC to remove the 1.23V reference 
value, which will prepare for triggering timer1 appropriately
*/
void aocInit(void)
begin

ACSR = 0x07;     // sets the AOC to be on, using pin B.2, and rising on B.2 greater tan B.3

end


/* Initialize timer0 to be used for task1 timing. 
This timer functions as the scheduler to update the LCD
and to prompt the capacitance measurement. 

Generates and ISR once per mS.
*/
void timer0Init(void)
begin

// zero variables to remove previous settings
t0_count = 0;
ready = 0;
TCCR0A = 0x00;
TCCR0B = 0x00;

TCCR0A |= (1<<WGM1);    // sets the timer mode to compare and capture on OCR0A
TCCR0B |= (1<<CS01)|(1<<CS02);    // Sets the prescaler for the timer to 64
OCR0A = 250;	// sets the compare value to the timer counter
TIMSK0 = 0x01;    // sets the OCR0A compare interrupt enable
end


/* Initialize timer1 to be used for measuring the rise time
of the capacitor. This output compare will be captured on 
the rising edge of the AOC
*/
void timer1Init(void)
begin

// zero variables to remove previous settings
TCCR1A = 0x00;
TCCR1B = 0x00;
TIMSK1 = 0x00;

TCCR1B |= (1<<ICES1)|(1<<WGM13)|(1<<WGM12)|(1<<CS10);    // sets the timer mode to compare and capture on ICR0 rising edge
TIMSK1 |= (1<<ICIE1);
end

// initializes timers, pins, and compare registers
void initialize(void)
begin
portInit();
aocInit();
timer0Init();
timer1Init();
end


// measures capacitance
void measures_cap(void)
begin


end


// set up LCD
void initLCD(void)
begin
  // start the LCD 
  init_lcd();
  LCDclr();

  //init the task timer
  time1=t1;
  //set up timer 0 for 1 mSec ticks
  TIMSK0 = 2;		//turn on timer 0 cmp match ISR 
  OCR0A = 249;  	//set the compare reg to 250 time ticks
  TCCR0A = 0b00000010; // turn on clear-on-match
  TCCR0B = 0b00000011;	// clock prescalar to 64
  
  // put some stuff on LCD
  CopyStringtoLCD(LCD_line, 8, 1);//start at char=8 line=1	
  CopyStringtoLCD(LCD_number, 0, 0);//start at char=0 line=0
  // init animation state variables	
  count=0;
  anipos = 0;
  LCDGotoXY(anipos,1);	//second line
  LCDsendChar('o');
  
  sei();
  //main task scheduler loop 
  while(1)
  begin 
  	// reset time and call task    
    if (time1==0){ time1=t1; task1();}
  end
end

// write to LCD
void writeLCD()
begin

end


// used for measuring capacitance through discharge and comparison
ISR(// insert timer0 compare vector here
begin
t0_count++;
if t0_count == 200
	begin
	t0_count = 0;
	ready = 1;
	end
end


// used for tracking ~200mS to govern a new capacitance measurement
ISR(// insert timer 1 compare vector here
begin
// note that the value of the timer is saved into: 
// ICR1H
// ICR1L

end
