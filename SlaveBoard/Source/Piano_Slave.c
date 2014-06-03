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

struct SongRead
{
	uint16_t usTimeLength;
	uint16_t usNoteLength;
};
typedef struct SongRead SongRead_t;

SongRead_t EEMEM SongOneInfo = {7, 7};
uint16_t EEMEM SongOneNotes[7] = {392,349,392,349,330,294,330};
uint16_t EEMEM SongOneTime[7] = {1000,1000,1000,1000,1000,1000,2000};

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

enum Msg_States{Msg_Init, Msg_Get, Msg_Form, Msg_Send} Msg_State;

void TckFct_Messenger()
{
	static unsigned char ucSongNum = 0;
	static unsigned char ucPrd = 2; // period for checking for messages
	static unsigned char ucPrdSnd = 16; // period for sending messages
	static unsigned char ucCnt = 0;
	static unsigned char ucMsgFlag = 0; // flag for when sending or receiving
	static unsigned char ucSendMsg = 0;
	static uint8_t		 *pucBuffer = NULL;
	static unsigned short usIdx = 0;
	static unsigned short usMsgLength = 0;
	static unsigned char ucErrorCnt = 0; // checking for errors, if a certain number then reset messenger
	static unsigned char ucLight = 0x00;
	
	switch(Msg_State)
	{
		case -1:
			Msg_State = Msg_Init;
			break;
		case Msg_Init:
			USART_Flush(0);
			ucLight = 0x01;
			PORTA = ucLight;
			Msg_State = Msg_Get;
			break;
		case Msg_Get:
			if(ucCnt < ucPrd)
			{
				ucCnt++;
			}
			else
			{
				
				ucCnt = 0;
				if(USART_HasReceived(0))
				{
					ucLight = 0x02;
					ucSongNum = USART_Receive(0);
					USART_Flush(0);
					PORTA = ucSongNum;
					ucMsgFlag = 1;
				}
			}
			if(ucMsgFlag == 1)
			{
				Msg_State = Msg_Form;
			}
			else
			{
				Msg_State = Msg_Get;
			}
			PORTA = ucLight;
			break;
		case Msg_Form:
			switch(ucSongNum)
			{
				case 1:
					if((pucBuffer = (uint8_t*)malloc(sizeof(SongOneInfo)+sizeof(SongOneNotes)+sizeof(SongOneTime))) == NULL)
					{
						ucErrorCnt++;
						Msg_State = Msg_Form;
						break;
					}
					memset(&pucBuffer[0], 0, sizeof(SongOneInfo)+sizeof(SongOneNotes)+sizeof(SongOneTime));
					eeprom_read_block(pucBuffer + usIdx, &SongOneInfo, sizeof(SongOneInfo));
					usIdx += sizeof(SongOneInfo);
					eeprom_read_block((void*)pucBuffer + usIdx, (const void*)SongOneNotes, sizeof(SongOneNotes));
					usIdx += sizeof(SongOneNotes);
					eeprom_read_block((void*)pucBuffer + usIdx, (const void*)SongOneTime, sizeof(SongOneTime));
					usIdx += sizeof(SongOneTime);
					usMsgLength = usIdx;
					usIdx = 0;
					ucLight = 0x03;
					Msg_State = Msg_Send;
					break;
				case 2:
					break;
				case 3:
					break;
				case 4:
					break;
				default:
					//this is error check, if go here reset whole messenger
					break;
			}
			PORTA = ucLight;
			break;
		case Msg_Send:
			if(ucCnt < ucPrdSnd)
			{
				ucCnt++;
			}
			else
			{
				ucCnt = 0;
				if(USART_IsSendReady(1) && ucMsgFlag == 1)
				{
					ucLight = 0x04;
					ucSendMsg = pucBuffer[usIdx];
					USART_Send(ucSendMsg, 1);
					ucMsgFlag = 2;
				}
				if(USART_HasTransmitted(1) && ucMsgFlag == 2)
				{
					usIdx++;
					ucMsgFlag = 1;
					ucLight = 0x10;
					if(usIdx >= usMsgLength)
					{
						ucLight = 0x0F;
						ucMsgFlag = 0;	
					}			
				}
			}
			if(ucMsgFlag == 0)
			{
				Msg_State = Msg_Init;
			}
			else
			{
				Msg_State = Msg_Send;
			}
			PORTA = ucLight;
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
		case Msg_Form:
			break;
		case Msg_Send:
			break;
		default:
			break;
	}
}

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	TimerSet(1);
	TimerOn();
	initUSART(0);
	initUSART(1);
	USART_Flush(0);
	USART_Flush(1);
	Msg_State = -1;
	
    while(1)
    {
        TckFct_Messenger();
		while(!TimerFlag);
		TimerFlag = 0;
    }
}