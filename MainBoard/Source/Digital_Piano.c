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

volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_time_cntcurr=0;
unsigned char tmpA = 0x00;
unsigned char gucSongChoice = 0; // global for song choice, might not need later on

struct SongInfo
{
	char szSongName[32];
	unsigned short usSongLength; 
};
typedef struct SongInfo SongInfo_t;


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

enum Dsp_States{Dsp_Init, Dsp_Intro, Dsp_Intro_Rel, Dsp_Choice, Dsp_Choice_Rel, Dsp_Play, Dsp_Rec} Dsp_State;
	
void TckFct_Display()
{
	static unsigned short usCnt = 0;
	static unsigned char ucIdx = 0;
	static unsigned char ucWelcIdx = 3;
	static unsigned char ucMainIdx = 2;
	static unsigned char ucSongIdx = 3; // variable for size of songs stored on chip
	static unsigned char ucRecIdx = 2;
	static unsigned char ucChoice = 0;  /* might need for storing user choice */
	static unsigned short usDspPrd = 2000;
	const char *Welcome[3] = {"Welcome to the  Digital Piano.", "Can play, record Press 1 to", "continue."};
	const char *MainMenu[2] = {"1: Play stored songs.", "2: Record Song"};
	const char *SongMenu[8] = {"1.Test Song #1\n", "2. Test Song #2\n", "3. Previous Menu"}; /* will add predetermined song names here */ 
	const char *RecordMenu[3] = {"Push and release 1 to start", "recording the song played"};
	
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
					default:
						break;
				}
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
		case Dsp_Play:
			break;
		case Dsp_Rec:
			break;
		default:
			break;
	}	
}

enum Ply_States{Ply_Init, Ply_Get, Ply_Play} Ply_State;

void TckFct_PlaySong()
{
	SongInfo_t SongHold;
	static unsigned char ucMemFlag = 0; // flag for not clearing memory everytime
	if(ucMemFlag == 0)
	{
		memset(&SongHold, 0, sizeof(SongInfo_t));
	}
	
	switch(Ply_State)
	{
		case -1:
			break;
		case Ply_Init:
			break;
		case Ply_Get:
			break;
		case Ply_Play:
			break;
		default:
			break;
	}
	switch(Ply_State)
	{
		case Ply_Init:
			break;
		case Ply_Get:
			break;
		case Ply_Play:
			break;
		default:
			break;		
	}	
}

/* function for recording input from user */
void TckFct_RecordSound()
{
	
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
void UsartTalk()
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
	Dsp_State = -1;
	
	//SetData();
	//GetData();
	
    while(1)
    {
		TckFct_Display();
        while(!TimerFlag);
		TimerFlag = 0;
    }
}