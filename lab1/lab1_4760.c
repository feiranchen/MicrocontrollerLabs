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

const int8_t LCD_initialize[] PROGMEM = "LCD Initialized!\0";
const int8_t LCD_line[] PROGMEM = "line 1\0";
const int8_t LCD_cap_equals[] PROGMEM = "C =\0";
const int8_t LCD_cap_clear[] PROGMEM = "           \0";
const int8_t LCD_no_capacitor[] PROGMEM = "No Capacitor Detected!\0";
const int8_t LCD_yes_capacitor[] PROGMEM = "Capacitor Detected!   \0";
typedef enum { false, true } bool;

// Global variable declarations
volatile char t0_count;    // counts the mS since last task1 call as updated by timer0
volatile char ready;	// set to 1 every ~200mS
volatile LCD_has_cap= false;

int main(void) 
begin
	volatile char ready = true;	// set to 1 every ~200mS
	initialize();
	sei();
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
void init_LCD(void)
begin
	// start the LCD 
	LCDinit();	//initialize the display
	LCDcursorOFF();
	LCDclr();				//clear the display
	LCDGotoXY(0,0);
	CopyStringtoLCD(LCD_initialize, 0, 0);
	LCDclr();
	LCD_has_cap= false;
	write_LCD_no_capacitor();
end

// write to LCD
// C = xx.x nf
// param: capacitance in .1nf
void write_LCD(int capacitance)
begin
	if (!capacitance && LCD_has_cap)
	begin
		write_LCD_no_capacitor();
	end
	else if (capacitance && !LCD_has_cap)
	begin
		write_LCD_yes_capacitor();
	end

	if (capacitance)
	begin
		// print value of capacitance
		LCDGotoXY(4, 0);
		sprintf(lcd_buffer,"%-i",capacitance/10);
			// display the count 
		LCDstring(lcd_buffer, strlen(lcd_buffer));	
	end
end

void write_LCD_no_capacitor()
begin
	CopyStringtoLCD(LCD_no_capacitor, 0, 0);
	CopyStringtoLCD(LCD_cap_clear, 0, 1);
end

void write_LCD_yes_capacitor()
begin
	CopyStringtoLCD(LCD_yes_capacitor, 0, 0);
	CopyStringtoLCD(LCD_cap_equals, 0, 1);
end


// used for tracking ~200mS to govern a new capacitance measurement
ISR(TIMER0_COMPA_vect)
begin
t0_count++;
if t0_count == 200
	begin
	t0_count = 0;
	ready = 1;
	end
end


// used for measuring capacitance through discharge and comparison
ISR(TIMER1_CAPT_vect)
begin
// note that the value of the timer is saved into: 
// ICR1H
// ICR1L

end
