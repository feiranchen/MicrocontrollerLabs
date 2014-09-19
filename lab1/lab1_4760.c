/* ECE 4760 Lab 1
	Connor Archard	cwa37
	Feiran Chen		fc254
	
	Rev 3
	9/16/14
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
typedef enum { false, true } bool;

const int8_t LCD_initialize[] PROGMEM = " LCD Initialized!\0";
const int8_t LCD_no_capacitor[] PROGMEM =  "No Cap Detected!\0";
const int8_t LCD_yes_capacitor[] PROGMEM = "Detected!       \0";
const int8_t LCD_cap_equals[] PROGMEM = "C =\0";
const int8_t LCD_cap_clear[] PROGMEM = "            \0";
int8_t lcd_buffer[13];	// LCD display buffer

// Global variable declarations
volatile char t0_count;    // counts the mS since last task1 call as updated by timer0
volatile char ready;	// set to 1 every ~200mS
volatile bool LCD_has_cap= false; //helper flag for LCD display
volatile char LED_count;    // counts when to toggle the LED
volatile uint16_t cap_time_count;


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

ACSR = (1<<ACIC);     // sets the AOC to be on, using pin B.2, and rising on B.2 greater tan B.3

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
LED_count = 0;

TCCR0A = 0x00;
TCCR0B = 0x00;
TIMSK0 = 0x00;

TCCR0A |= (1<<WGM01);    // sets the timer mode to compare and capture on OCR0A
TCCR0B |= (1<<CS01)+(1<<CS00);    // Sets the prescaler for the timer to 64
TIMSK0 |= (1<<OCIE0A);    // enables CTC interrupt

OCR0A = 249;	// sets the compare value to the timer counter

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

	TCCR1B |= (1<<ICES1)+(1<<CS10);    // sets the timer mode to normal on ICR0 rising edge
	TIMSK1 |= (1<<ICIE1);
	TCNT1 = 0;

end


void discharge(void)
begin
	PORTD ^= 0x08;    // Turns on red LED to indicate discharging
	ready = false;
	DDRB |= 0x04;    // B.2 is set to an output
	PORTB &= 0xFB;    // B.2 is set low to begin cap discharge
end

// zeros timer1 for charge count
// sets B.2 to input to allow for charging
void start_charge(void)
begin
	DDRB ^= 0x04;    // sets B.2 to an input (capacitor starts charging)
	TCNT1 = 0;    // Initializes timer1 to 0x0000
end

void write_LCD_no_capacitor()
begin
	CopyStringtoLCD(LCD_no_capacitor, 0, 0);
	CopyStringtoLCD(LCD_cap_clear, 0, 1);
	LCD_has_cap= false;
end

void write_LCD_yes_capacitor()
begin
	CopyStringtoLCD(LCD_yes_capacitor, 0, 0);
	CopyStringtoLCD(LCD_cap_equals, 0, 1);
	LCD_has_cap= true;
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
	//LCDclr();
	LCD_has_cap= false;
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
		// populate format for capacitance
		sprintf(lcd_buffer,"%-i",capacitance/10);
		sprintf(lcd_buffer + strlen(lcd_buffer), "%c", '.');
		sprintf(lcd_buffer + strlen(lcd_buffer), "%-i nf  ", capacitance % 10);
		// display capacitance
		LCDGotoXY(4, 1);
		LCDstring(lcd_buffer, strlen(lcd_buffer));	
	end
end

// initializes timers, pins, and compare registers
void initialize(void)
begin
	portInit();
	aocInit();
	timer0Init();
	timer1Init();    
	init_LCD();
	cap_time_count = 0;
end


// used for tracking ~200mS to govern a new capacitance measurement
ISR(TIMER0_COMPA_vect)
begin

	t0_count++;
	if (t0_count >= 200)
	begin
		t0_count = 0;
		ready = true;	
		LED_count++;

		if (LED_count == 3)
		begin
			LED_count = 0;
			PORTD ^= 0x01;
		end

	end

	TCNT0 = 0;

end

// used for measuring capacitance through charge and comparison
ISR(TIMER1_CAPT_vect)
begin
	cap_time_count = ICR1;
	PORTD ^= 0x02;    // Toggle yellow LED to indicate discharging
end

int main(void) 
begin
	ready = false;	// set to 1 every ~200mS

	initialize();    // set all ports and timers up
	sei();    // enable interrupts
	discharge();    // set B.2 to low output to start discharge
	//write_LCD(cap_time_count);
	//_delay_ms(1000);
	//write_LCD(0);
	//start_charge();

	while(1)
	begin
		if (ready) 
		begin

			discharge();    // begins discharge of the capacitor

			// performs calculations based on the value read in the last cycle's ISR
			// taking the board's cap offset into consideration
			if (cap_time_count < 46)
			begin
				write_LCD(0);
			end
			else
			begin
				write_LCD((cap_time_count - 42) / 29.4);
			end

			start_charge();    // sets B.2 to an input, leading to capacitor charging and timer1 ISR triggering
			
		end

	end

end
