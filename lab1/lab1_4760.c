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
volatile LCD_has_cap= false;
volatile unsigned int time1, time2;

int main(void) 
begin
	volatile char ready = true;	// set to 1 every ~200mS
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



// initializes timers, pins, and compare registers
void initialize(void)
begin


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

// used for measuring capacitance through discharge and comparison
ISR(// insert timer0 compare vector here
begin


end


// used for tracking ~200mS to govern a new capacitance measurement
ISR(// insert timer 1 compare vector here
begin


end
