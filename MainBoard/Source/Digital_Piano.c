/*
 * Digital_Piano.c
 *
 * Created: 5/19/2014 4:24:25 PM
 *  Author: Brandon
 */ 

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include "usart_ATmega1284.h"

volatile unsigned char		TimerFlag			= 0;
unsigned long				_avr_timer_M		= 1;
unsigned long				_avr_time_cntcurr	= 0;
unsigned char				tmpA				= 0x00;
unsigned char				gucSongChoice		= 0; // global for song choice, might not need later on

// struct for holding recorded song info
struct SongInfo
{
	char szSongName[32];
	unsigned short usSongLength;
	unsigned short usNoteLength; 
};
typedef struct SongInfo SongInfo_t;

// struct for holding stored songs
struct SongRead
{
	uint16_t usTimeLength;
	uint16_t usNoteLength;
};
typedef struct SongRead SongRead_t;

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

// 0.954 hz is lowest frequency possible with this function,
// based on settings in PWM_on()
// Passing in 0 as the frequency will stop the speaker from generating sound
void set_PWM(unsigned short frequency) {
	
	if (!frequency) TCCR3B &= 0x08; //stops timer/counter
	else TCCR3B |= 0x03; // resumes/continues timer/counter
	
	// prevents OCR3A from overflowing, using prescaler 64
	// 0.954 is smallest frequency that will not result in overflow
	if (frequency < 1) OCR3A = 0xFFFF;
	
	// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
	else if (frequency > 31250) OCR3A = 0x0000;
	
	// set OCR3A based on desired frequency
	else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

	TCNT3 = 0; // resets counter
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

/* function for getting recorded song from eeprom */
void GetData()
{
}

/* function for setting recorded song into eeprom */
void SetData(unsigned char* ucTemp, unsigned char ucSongNum)
{
	
}

//basic prototype for talking to other micro controller
void UsartSend(unsigned char ucSong, unsigned char *ucMsgFlag)
{	
	if(USART_IsSendReady(1))
	{
		USART_Send(ucSong, 1);
		*ucMsgFlag = 1;
	}		
}

void UsartRecv(unsigned char *ucTemp, unsigned char *ucDataLeft)
{
	if(USART_HasReceived(0))
	{
		*ucTemp = USART_Receive(0);
		*ucDataLeft -= sizeof(unsigned char);
	}
}


enum Dsp_States{Dsp_Init, Dsp_Intro, Dsp_Intro_Rel, Dsp_Choice, Dsp_Choice_Rel, Dsp_Play, Dsp_Rec} Dsp_State;
	
void TckFct_Display()
{
	static unsigned short	usCnt			= 0;
	static unsigned char	ucIdx			= 0;
	static unsigned char	ucWelcIdx		= 3;
	static unsigned char	ucMainIdx		= 2;
	static unsigned char	ucSongIdx		= 3; // variable for size of songs stored on chip
	static unsigned char	ucRecIdx		= 2;
	static unsigned char	ucChoice		= 0;  /* might need for storing user choice */
	static unsigned short	usDspPrd		= 2000;
	const char				*Welcome[3]		= {"Welcome to the  Digital Piano.", "Can play, record Press 1 to", "continue."};
	const char				*MainMenu[2]	= {"1: Play stored songs.", "2: Record Song"};
	const char				*SongMenu[8]	= {"1.Test Song #1", "2.Test Song #2", "3. Previous Menu"}; /* will add predetermined song names here */ 
	const char				*RecordMenu[3]	= {"Push and release 1 to start", "recording the song played"};
	
	tmpA = ~PINA;
	switch(Dsp_State)
	{
		case -1:
			Dsp_State = Dsp_Init;
			break;
		case Dsp_Init:
			LCD_DisplayString(1, (const unsigned char*)Welcome[ucIdx]);
			ucIdx++;
			Dsp_State = Dsp_Intro;
			break;
		case Dsp_Intro:	 // displaying menu and waiting for user input
			if(tmpA == 0x01) // checking user pressing button1 
			{
				usCnt = 0;
				ucIdx = 0;
				Dsp_State = Dsp_Intro_Rel;
			}
			else
			{
				if(ucIdx < ucWelcIdx)
				{
					if(usCnt < usDspPrd)
					{
						usCnt++;
					}
					else
					{
						usCnt = 0;
						LCD_DisplayString(1, (const unsigned char*)Welcome[ucIdx]);
						ucIdx++;
					}
				}
				else
				{
					ucIdx = 0;
					usCnt = 0;
				}
				Dsp_State = Dsp_Intro;
			}
			break;
		case Dsp_Intro_Rel:  // waiting for user to release button
			if(gucSongChoice == 0)
			{
				if(tmpA == 0x00)
				{
					LCD_DisplayString(1, (const unsigned char*)MainMenu[ucIdx]);
					ucIdx++;
					Dsp_State = Dsp_Choice;
				}
				else
				{
					Dsp_State = Dsp_Intro_Rel;
				}
			}
			break;
		case Dsp_Choice: // getting user choice to record or play
			if(tmpA == 0x01 || tmpA == 0x02)
			{
				usCnt = 0;
				ucIdx = 0;
				if(tmpA == 0x01)
				{
					ucChoice = 0;
				}
				else
				{
					ucChoice = 1;
				}
				Dsp_State = Dsp_Choice_Rel;
			}
			else
			{
				if(ucIdx < ucMainIdx)
				{
					if(usCnt < usDspPrd)
					{
						usCnt++;
					}
					else
					{
						usCnt = 0;
						LCD_DisplayString(1, (const unsigned char*)MainMenu[ucIdx]);
						ucIdx++;
					}
				}
				else
				{
					ucIdx = 0;
					usCnt = 0;
				}
				Dsp_State = Dsp_Choice;
			}
			break;
		case Dsp_Choice_Rel: // waiting for user to release button
			if(tmpA == 0x00)
			{
				if(ucChoice == 0)
				{
					LCD_DisplayString(1, (unsigned char*)SongMenu[ucIdx]);
					ucIdx++;
					Dsp_State = Dsp_Play;
				}
				else
				{
					LCD_DisplayString(1, (unsigned char*)RecordMenu[ucIdx]);
					ucIdx++;
					Dsp_State = Dsp_Rec;
				}
			}
			else
			{
				Dsp_State = Dsp_Choice_Rel;
			}
			break;
		case Dsp_Play:
			//need to create switch statement for choice of song or previous menu
			if(tmpA != 0x00)
			{
				// testing with two songs only at first, will add more later
				switch(tmpA)
				{
					case 1:
						gucSongChoice = 1;
						break;
					case 2:
						gucSongChoice = 2;
						break;
					case 4:
						//gucSongChoice = 3;
						break;
					case 8:
						break;
					case 16:
						break;
					case 32:
						break;
					case 64:
						break;
					case 128:
						break;
					default: //need to add error check
						break;
				}
				ucIdx = 0;
				usCnt = 0;
				Dsp_State = Dsp_Intro_Rel;
			}
			else
			{
				if(ucIdx < ucSongIdx)
				{
					if(usCnt < usDspPrd)
					{
						usCnt++;
					}
					else
					{
						usCnt = 0;
						LCD_DisplayString(1, (unsigned char*)SongMenu[ucIdx]);
						ucIdx++;
					}
				}
				else
				{
					ucIdx = 0;
					usCnt = 0;
				}
				Dsp_State = Dsp_Play;
			}
			break;
		case Dsp_Rec:
			if(tmpA == 0x01)
			{
				// need to decide on how to store record state for next state machine
			}
			else
			{
				if(ucIdx < ucRecIdx)
				{
					if(usCnt < usDspPrd)
					{
						usCnt++;
					}
					else
					{
						usCnt++;
						LCD_DisplayString(1, (unsigned char*)RecordMenu[ucIdx]);
						ucIdx++;
					}
				}
				else
				{
					ucRecIdx = 0;
					usCnt = 0;
				}
				Dsp_State = Dsp_Rec;
			}
			break;
		default:
			break;
	}
	
	switch(Dsp_State)
	{
		case Dsp_Init:
			break;
		case Dsp_Intro:
			break;
		case Dsp_Intro_Rel:
			break;
		case Dsp_Choice:
			break;
		case Dsp_Choice_Rel:
			break;
		case Dsp_Play:
			break;
		case Dsp_Rec:
			break;
		default:
			break;
	}	
}

enum Ply_States{Ply_Init,Ply_Send, Ply_Get, Ply_Play, Ply_Off} Ply_State;

void TckFct_PlaySong()
{
	SongRead_t SongHold;
	static unsigned char	*pucBuffer		= NULL;
	static unsigned short	*pusNoteBuff	= NULL;
	static unsigned short	*pusTimeBuff	= NULL;
	static unsigned short	ucMsgPrd		= 2; // period for messaging of usart
	//static unsigned char	ucReadPrd		= 2; // period for checking for read
	static unsigned short	ucCnt			= 0; // counter variable for period
	static unsigned char	ucTemp			= 0;
	static unsigned char	ucReadAmt		= 0; // this will change after testing
	static unsigned char	ucMsgFlag		= 0; // 0 = send, 1 = sent, 2 = sent complete, 3 = readheader, 4 = readarray1, 5= readarray2, 6=read done;
	static unsigned short   usTimeCnt		= 0;
	static unsigned short   usIdx			= 0;
	static unsigned short	usFrequency		= 0;
	static unsigned char	ucSizeSong		= 0;
	
	
	switch(Ply_State)
	{
		case -1:
			Ply_State = Ply_Init;
			break;
		case Ply_Init:
			if(gucSongChoice != 0)
			{
				LCD_DisplayString(1, (const unsigned char*)"In play song");
				memset(&SongHold, 0, sizeof(SongRead_t));
				Ply_State = Ply_Send;	
			}
			else
			{
				Ply_State = Ply_Init;
			}
			break;
		case Ply_Send:
			if(ucCnt < ucMsgPrd)
			{
				ucCnt++;
			}
			else
			{
				ucCnt = 0;
				if(ucMsgFlag == 0)
				{
					ucTemp = gucSongChoice;
					UsartSend(ucTemp, &ucMsgFlag);
				}
				if(ucMsgFlag == 1)
				{
					if(USART_HasTransmitted(1))
					{
						ucMsgFlag = 2;
					}
				}
			}
			if(ucMsgFlag == 2)
			{
				LCD_DisplayString(1, (const unsigned char*)"message sent");
				Ply_State = Ply_Get;
			}
			else
			{
				Ply_State = Ply_Send;
			}
			break;
		case Ply_Get:
			LCD_DisplayString(1, (const unsigned char*)"in get");
			if(ucCnt < ucMsgPrd)
			{
				ucCnt++;
			}
			else
			{
				// will need to read in size of song header, then add on the data size of the 
				// two arrays that are stored to the total amount to read.
				switch(ucMsgFlag)
				{
					case 2:
						ucReadAmt = sizeof(SongRead_t);
						if((pucBuffer = (unsigned char*)malloc(sizeof(SongRead_t))) == NULL)
						{
							break;
						}
						memset(&pucBuffer[0], 0, sizeof(SongRead_t));
						ucMsgFlag = 3;
						break;
					case 3:
						UsartRecv(&ucTemp, &ucReadAmt);
						pucBuffer[usIdx] = ucTemp;
						usIdx += sizeof(unsigned char);
						if(ucReadAmt == 0)
						{
							memcpy(&SongHold, &pucBuffer, sizeof(SongRead_t));
							free(pucBuffer);
							ucReadAmt = SongHold.usNoteLength;
							pucBuffer = (unsigned char*)malloc(SongHold.usNoteLength);
							usIdx = 0;
							ucMsgFlag = 4;
						}
						break;
					case 4:
						UsartRecv(&ucTemp, &ucReadAmt);
						pucBuffer[usIdx] = ucTemp;
						usIdx++;
						if(ucReadAmt == 0)
						{
							pusNoteBuff = (unsigned short*)malloc(SongHold.usNoteLength);
							memcpy(&pusNoteBuff[0], &pucBuffer[0], SongHold.usNoteLength);
							free(pucBuffer);
							ucReadAmt = SongHold.usTimeLength;
							pucBuffer = (unsigned char*)malloc(SongHold.usTimeLength);
							usIdx = 0;
							ucMsgFlag = 5;
						}
						break;
					case 5:
						UsartRecv(&ucTemp, &ucReadAmt);
						pucBuffer[usIdx] = ucTemp;
						usIdx++;
						if(ucReadAmt == 0)
						{
							pusTimeBuff = (unsigned short*)malloc(SongHold.usTimeLength);
							memcpy(&pusTimeBuff[0], (unsigned short*)&pucBuffer[0], SongHold.usTimeLength);
							free(pucBuffer);
							usIdx = 0;
							ucMsgFlag = 6;
						} 
						break;
					default:
						// add error check here
						break;
				}
				ucCnt = 0;
			}
			if(ucMsgFlag == 6)
			{
				ucCnt = 0;
				ucMsgFlag = 0;
				while(pusNoteBuff[ucSizeSong] != '\0')
				{
					ucSizeSong++;
				}
				Ply_State = Ply_Off;
			}
			else
			{
				Ply_State = Ply_Get;
			}
			break;
		case Ply_Play:
			if(usTimeCnt < pusTimeBuff[ucCnt] && ucCnt < ucSizeSong)
			{
				if(usTimeCnt == 0)
				{
					PWM_on();
					usFrequency = pusNoteBuff[ucCnt];
					set_PWM(usFrequency);
				}
				usTimeCnt++;
				Ply_State = Ply_Play;
			}
			else if(ucCnt < ucSizeSong)
			{
				usTimeCnt = 0;
				ucCnt++;
				Ply_State = Ply_Play;
			}
			else
			{
				usFrequency = 0;
				ucCnt = 0;
				set_PWM(usFrequency);
				gucSongChoice = 0;
				Ply_State = Ply_Init;	
			}
			break;
		case Ply_Off:
			PWM_on();
			usFrequency = pusNoteBuff[ucCnt];
			set_PWM(usFrequency);
			usTimeCnt++;
			Ply_State = Ply_Play;
			break;
		default:
			break;
	}
	switch(Ply_State)
	{
		case Ply_Init:
			break;
		case Ply_Send:
			break;
		case Ply_Get:
			break;
		case Ply_Play:
			break;
		case Ply_Off:
			break;
		default:
			break;		
	}	
}

/* function for recording input from user */
void TckFct_RecordSound()
{
}

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	LCD_init();
	TimerSet(1);
	TimerOn();
	initUSART(0);
	initUSART(1);
	USART_Flush(0);
	USART_Flush(1);
	Dsp_State = -1;
	Ply_State = -1;
	
	//SetData();
	//GetData();
	
    while(1)
    {
		TckFct_Display();
		TckFct_PlaySong();
        while(!TimerFlag);
		TimerFlag = 0;
    }
}