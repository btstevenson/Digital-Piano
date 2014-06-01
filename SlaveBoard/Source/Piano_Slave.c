/*
 * Piano_Slave.c
 *
 * Created: 5/31/2014 6:27:34 PM
 *  Author: Brandon
 */ 


#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "usart_ATmega1284.h"

volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_time_cntcurr=0;

void TimerOn()
{
	TCCR1B = 0x0B;
	OCR1A = 125;
	TIMSK1 = 0x02;
	TCNT1 = 0;
	_avr_time_cntcurr = _avr_timer_M;
	SREG |= 0x80;
}

void TimerOff()
{
	TCCR1B = 0x00;
}

void TimerISR()
{
	TimerFlag = 1;
}

ISR(TIMER1_COMPA_vect)
{
	_avr_time_cntcurr--;
	if(_avr_time_cntcurr == 0)
	{
		TimerISR();
		_avr_time_cntcurr = _avr_timer_M;
	}
}

void TimerSet(unsigned long M)
{
	_avr_timer_M = M;
	_avr_time_cntcurr = _avr_timer_M;
}

enum Msg_States{Msg_Init, Msg_Get, Msg_Send} Msg_State;

void TckFct_Messenger()
{
	static unsigned char *pucBuffer = NULL;
	static unsigned short usPrd = 2;
	
	switch(Msg_State)
	{
		case -1:
			break;
		case Msg_Init:
			break;
		case Msg_Get:
			break;
		case Msg_Send:
			break;
		default:
			break;
	}
	switch(Msg_State)
	{
		case Msg_Init:
			break;
		case Msg_Get:
			break;
		case Msg_Send:
			break;
		default:
			break;
	}
}

int main(void)
{
	TimerSet(1);
	TimerOn();
	Msg_State = -1;
	
    while(1)
    {
        TckFct_Messenger();
		while(!TimerFlag);
		TimerFlag = 0;
    }
}