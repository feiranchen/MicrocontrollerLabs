/* ECE 4760 Lab 1
	Connor Archard	cwa37
	Feiran Chen		fc254
	
	Rev 1
	9/6/14 
	20:00 
*/


#define F_CPU 16000000UL
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#define begin {
#define end }


int main(void) 
begin

	volatile char ready = 1;	// set to 1 every ~200mS
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
DDRB = ;	// set pin B.2 to be input and B.3 to be an output



end


// measures capacitance
void task1(void)
begin


end


// writes values to the LCD
void task2(void)
begin


end


// used for measuring capacitance through discharge and comparison
ISR(// insert timer0 compare vector here
begin


end


// used for tracking ~200mS to govern a new capacitance measurement
ISR(// insert timer 1 compare vector here
begin


end
