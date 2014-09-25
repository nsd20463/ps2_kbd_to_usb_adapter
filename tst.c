#define F_CPU 16000000
#include <avr/io.h>
#include <util/delay.h>

int main (void) {   
   DDRE = (1<<6); //set up pin 0 on port B

   while (1) {
       PORTE = (1<<6); //set pin 0 on port B high
       _delay_ms(25);
       PORTE = 0x00; //set pin 0 on port B low
       _delay_ms(125);
   }
   return 0;
}
