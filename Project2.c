/*
 * Project2.c
 *
 *  Created on: Sep 15, 2023
 *      Author: Aya Ashraf Rashad
 */

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>

#define Top 976      //defining the compare value

//initializing the clock digits
unsigned char sec1=0;
unsigned char sec2=0;
unsigned char min1=0;
unsigned char min2=0;
unsigned char hour1=0;
unsigned char hour2=0;

//initializing each interrupt flag that indicates its occurrence
unsigned char INT0_flag=0;
unsigned char INT1_flag=0;
unsigned char INT2_flag=0;

//initializing timer1
void Timer1_init(void)
{
	TCNT1=0;      //Set timer1 initial count to zero
	OCR1A=Top;    //Set the Compare value to 976 as the prescalar is 1024
	TIMSK = (1<<OCIE1A);    //Enable Timer1 Compare A Interrupt

	/*Configure timer control register TCCR1A
	  1. Disconnect OC1A and OC1B  COM1A1=0 COM1A0=0 COM1B0=0 COM1B1=0
	  2. FOC1A=1 FOC1B=0
	  3. CTC Mode WGM10=0 WGM11=0 (Mode Number 4)*/

	TCCR1A = (1<<FOC1A);

	/*Configure timer control register TCCR1B
	  1. CTC Mode WGM12=1 WGM13=0 (Mode Number 4)
	  2. Prescaler = F_CPU/1024 CS10=1 CS11=0 CS12=1
	  */
	TCCR1B = (1<<WGM12) | (1<<CS10) | (1<<CS12);


}

//this function updates the clock digits variables every compare match in timer1
void updateTime(void)
{
    sec1++;
    if (sec1 == 10) {
        sec1 = 0;
        sec2++;     //increment the second digit of the seconds when the first digit turns 9

        if (sec2 == 6) {
            sec2 = 0;
            min1++;      //increment the minutes when the seconds turn 60sec

            if (min1 == 10) {
                min1 = 0;
                min2++;     //increment the second digit of the minutes when the first digit turns 9

                if (min2 == 6) {
                    min2 = 0;
                    hour1++;       //increment the hours when the seconds turn 60min

                    if (hour1 == 10) {
                        hour1 = 0;
                        hour2++;     //increment the second digit of the hours when the first digit turns 9

                        if (hour2 == 2 && hour1 == 4) {
                            hour2 = 0;     // Reset hours to 00 when it reaches 24
                            hour1 = 0;
                        }
                    }
                }
            }
        }
    }
}


ISR(TIMER1_COMPA_vect)
{
	updateTime();   // call the updateTime function when the compare match occurs

}

void interrupt0_init(void)
{
	DDRD &= ~(1<<PD2);     // Configure INT0/PD2 as input pin
	MCUCR |= (1<<ISC01);   // Trigger INT0 with the falling edge
	MCUCR &= ~(1<<ISC00);
	GICR |= (1<<INT0);     // Enable external interrupt pin INT0

}

ISR(INT0_vect)
{
	INT0_flag=1;    // set the interrupt flag to indicate that INT0 occurs
}


void interrupt1_init(void)
{
	DDRD &= ~(1<<PD3);     // Configure INT1/PD3 as input pin
	MCUCR |= (1<<ISC10) |(1<<ISC11);    // Trigger INT1 with the rising edge
	GICR |= (1<<INT1);     // Enable external interrupt pin INT1

}

ISR(INT1_vect)
{
	INT1_flag=1;    // set the interrupt flag to indicate that INT1 occurs

}


void interrupt2_init(void)
{
	DDRB &= ~(1<<PB2);    // Configure INT2/PB2 as input pin
	MCUCSR &= ~(1<<ISC2);   // Trigger INT2 with the falling edge
	GICR |= (1<<INT2);     // Enable external interrupt pin INT2

}

ISR(INT2_vect)
{
	INT2_flag=1;  // set the interrupt flag to indicate that INT2 occurs
}


int main (void)
{
	DDRC |= 0x0F;       //Configure the portC 4 pins[PC0:PC3] as an outputs for the 7-segment decoder
	PORTC &= 0xF0;      //clear portC first 4 pins[PC0:PC3]
	DDRA |= 0x3F;       //Configure the portA 6 pins[PA0:PA5] as an enable/disable pins for the 7-segment
	PORTA &= 0xC0;      ////Clear the portA 6 pins[PA0:PA5] as they're disabled


	SREG |=(1<<7);      //Enable global interrupts in MC
	PORTD |= (1<<PD2);  //Enable the internal pull-up resistor for interrupt0
	PORTB |= (1<<PB2);  //Enable the internal pull-up resistor for interrupt2


	Timer1_init();
	interrupt0_init();
	interrupt1_init();
	interrupt2_init();


	while(1)
	{
		/*This case resets all the 7-segments and occurs when interrupt0
		  is enabled (push button connected to pin PD2 is pressed) */

		if(INT0_flag==1)
		{
			//reset all the clock digits
			 sec1=0;
			 sec2=0;
			 min1=0;
			 min2=0;
			 hour1=0;
			 hour2=0;

			/*reset the interrupt0 flag to do this case only once when
			 int0 is triggered*/
			 INT0_flag=0;
		}

		/*This case stop the clock from counting and occurs when interrupt1
		  is enabled (push button connected to pin PD3 is pressed) */

		else if(INT1_flag==1)
		{
			//No clock source (Timer/Counter stopped)

			TCCR1B &= ~(1 << CS12);  // Clear CS12 bit
			TCCR1B &= ~(1 << CS11);  // Clear CS11 bit
			TCCR1B &= ~(1 << CS10);  // Clear CS10 bit

			/*reset the interrupt1 flag to do this case only once when
			 int1 is triggered*/
			INT1_flag=0;

		}

		/*This case makes the clock resume counting and occurs when interrupt2
		  is enabled (push button connected to pin PB2 is pressed) */

		else if(INT2_flag==1)
		{
			//sets the clock to clk(I/O)/1024 (From prescaler)
			TCCR1B |=(1<<CS10) | (1<<CS12);

			/*reset the interrupt2 flag to do this case only once when
			 int2 is triggered*/
			INT2_flag=0;
		}

		_delay_ms(2);
		PORTA = 0x01;       //enables the first 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (sec1 & 0x0F);   //display the first digit (second 1)
		_delay_ms(2);

		PORTA = 0x02;       //enables the second 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (sec2 & 0x0F);   //display the second digit (second 2)
		_delay_ms(2);

		PORTA = 0x04;       //enables the third 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (min1 & 0x0F);  //display the third digit (minute 1)
		_delay_ms(2);

		PORTA = 0x08;      //enables the fourth 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (min2 & 0x0F);  //display the fourth digit (minute 2)
		_delay_ms(2);

		PORTA = 0x10;     //enables the fifth 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (hour1 & 0x0F);  //display the fifth digit (hour 1)
		_delay_ms(2);

		PORTA = 0x20;    //enables the sixth 7-segment and disables the others
		PORTC = (PORTC & 0xF0) | (hour2 & 0x0F);  //display the sixth digit (hour 2)
		_delay_ms(2);



	}


}





