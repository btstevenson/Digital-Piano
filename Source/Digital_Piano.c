/*
 * Digital_Piano.c
 *
 * Created: 5/19/2014 4:24:25 PM
 *  Author: Brandon
 */ 

#include <avr/io.h>
#include "io.c"
#include <avr/interrupt.h>
#include <avr/eeprom.h>

const char *Welcome[3] = {"Welcome to the  Digital Piano.", "Can play, record Press 1 to", "continue."};
const char *MainMenu[2] = {"1: Play stored songs.", "2: Record Song"};
const char *SongMenu[8] = {"1.Test Song #1\n", "2. Test Song #2\n", "3. Previous Menu"};
const char *RecordMenu[3] = {0};
 
volatile unsigned char TimerFlag = 0;
unsigned long _avr_timer_M = 1;
unsigned long _avr_time_cntcurr=0;
unsigned char tmpA = 0x00;
unsigned char ucWelcIdx = 3;
unsigned char ucMainIdx = 2;
unsigned short usDspPrd = 2000;

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

enum Dsp_States{Dsp_Init, Dsp_Intro, Dsp_Intro_Rel, Dsp_Choice, Dsp_Play, Dsp_Rec} Dsp_State;
	
void TckFct_Display()
{
	static unsigned short usCnt = 0;
	static unsigned char ucIdx = 0;
	//static unsigned char ucChoice = 0;  /* might need for storing user choice */
	
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
		case Dsp_Choice:
			if(tmpA == 0x01)
			{
				Dsp_State = Dsp_Play;
			}
			else if(tmpA == 0x02)
			{
				Dsp_State = Dsp_Rec;
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
		case Dsp_Play:
			break;
		case Dsp_Rec:
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

/* function for recording input from user */
void TckFct_RecordSound()
{
	
}

/* function for getting data from eeprom */
void GetData()
{
}

/* function for setting data from eeprom */
void SetData()
{
	
}

int main(void)
{
	DDRA = 0x00; PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	DDRD = 0xFF; PORTD = 0x00;
	
	LCD_init();
	TimerSet(1);
	TimerOn();
	Dsp_State = -1;
	
	SetData();
	GetData();
	
    while(1)
    {
		TckFct_Display();
        while(!TimerFlag);
		TimerFlag = 0;
    }
}