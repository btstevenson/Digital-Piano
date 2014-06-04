/*
 * TestPiano.c
 *
 * Created: 6/4/2014 1:26:00 AM
 *  Author: Brandon
 */ 


#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include "io.h"
#include <avr/interrupt.h>
#include <avr/eeprom.h>


volatile unsigned char		TimerFlag			= 0;
unsigned long				_avr_timer_M		= 1;
unsigned long				_avr_time_cntcurr	= 0;
unsigned char				tmpA				= 0x00;
unsigned char				gucSongChoice		= 0;
unsigned char				gucRecorded			= 0;

#define EEPROMSET

//#if define EEPROMSET
uint8_t  EEMEM SongSizeOne = 29;
uint16_t EEMEM SongOneNotes[29] = {262,294,330,349,330,294,330,0,262,294,330,349,392,392,0,392,349,392,349,330,294,330,0,262,294,330,294,262,262};
uint16_t EEMEM SongOneTime[29] = {1000,1000,1000,1000,1000,1000,2000,100,1000,1000,1000,1000,2000,2000,100,1000,1000,1000,1000,1000,1000,2000,100,1000,1000,1000,1000,2000,2000};
uint8_t  EEMEM SongSizeTwo = 27;
uint16_t EEMEM SongTwoNotes[27] = {262,392,262,392,262,294,330,349,392,262,392,262,392,0,392,262,392,262,392,349,330,294,262,392,262,392,262};
uint16_t EEMEM SongTwoTime[27] = {1000, 1000, 1000, 1000, 1000, 1000,1000,1000,1000,1000,1000,1000,4000,100,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,1000,4000};
uint8_t EEMEM RecordOneSize;
uint16_t EEMEM RecordOneNotes[80];
uint16_t EEMEM RecordOneTime[80];
uint8_t EEMEM RecordTwoSize;
uint16_t EEMEM RecordTwoNotes[80];
uint16_t EEMEM RecordTwoTime[80];
//#endif
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

unsigned char SongNote(unsigned short usNote)
{
	unsigned char ucValue = 0;
	
	switch(usNote)
	{
		case 262:
			ucValue = 0x43;
			break;
		case 294:
			ucValue = 0x44;
			break;
		case 330:
			ucValue = 0x45;
			break;
		case 349:
			ucValue = 0x46;
			break;
		case 392:
			ucValue = 0x47;
			break;
		case 440:
			ucValue = 0x41;
			break;
		case 494:
			ucValue = 0x42;
			break;
		case 523:
			ucValue = 0x43;
			break;
		default:
			break;
	}
	return ucValue;
}

unsigned short NoteFrequency(unsigned char ucNote)
{
	unsigned short usFrequency = 0;
	
	switch(ucNote)
	{
		case 0x01:
			usFrequency = 523;
			break;
		case 0x02:
			usFrequency = 494;
			break;
		case 0x04:
			usFrequency = 440;
			break;
		case 0x08:
			usFrequency = 392;
			break;
		case 0x10:
			usFrequency = 349;
			break;
		case 0x20:
			usFrequency = 330;
			break;
		case 0x40:
			usFrequency = 294;
			break;
		case 0x80:
			usFrequency = 262;
			break;
		default:
			usFrequency = 0;
			break;
	}
	
	return usFrequency;
}

void CustomChar(int place)
{
	unsigned char	ucLCDNotes[8]	= {0b00000, 0b00110, 0b00100, 0b00100,0b01100, 0b01100, 0b00000, 0b00000};
	unsigned char i = 0;
	LCD_WriteCommand(0x40 + (place *8));
	for(i = 0; i < 8; i++)
	{
		LCD_WriteData(ucLCDNotes[i]);
	}
}

enum Dsp_States{Dsp_Init, Dsp_Intro, Dsp_Intro_Rel, Dsp_Choice, Dsp_Choice_Rel, Dsp_Play, Dsp_Rec} Dsp_State;

void TckFct_Display()
{
	static unsigned short	usCnt			= 0;
	static unsigned char	ucIdx			= 0;
	static unsigned char	ucWelcIdx		= 3;
	static unsigned char	ucMainIdx		= 2;
	static unsigned char	ucSongIdx		= 5; // variable for size of songs stored on chip
	static unsigned char	ucRecIdx		= 2;
	static unsigned char	ucChoice		= 0;  /* might need for storing user choice */
	static unsigned short	usDspPrd		= 2000;
	const char				*Welcome[3]		= {"Welcome to the  Digital Piano.", "Can play, record Press 1 to", "continue."};
	const char				*MainMenu[2]	= {"1: Play stored songs.", "2: Record Song"};
	const char				*SongMenu[8]	= {"1.Hungarian Folk  Song", "2.Bike Ride", "3. Recorded #1", "4. Recorded #2", "5. Previous Menu"}; /* will add predetermined song names here */
	const char				*RecordMenu[3]	= {"Push and release 1 to start", "recording the song played"};
	
	tmpA = ~PINA;
	switch(Dsp_State)
	{
		case -1:
		Dsp_State = Dsp_Init;
		break;
		case Dsp_Init:
		if(gucSongChoice == 0)
		{
			LCD_DisplayString(1, (const unsigned char*)Welcome[ucIdx]);
			LCD_Cursor(0);
			ucIdx++;
			Dsp_State = Dsp_Intro;
		}
		else
		{
			Dsp_State = Dsp_Init;
		}
		
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
					LCD_Cursor(0);
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
			LCD_Cursor(0);
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
					LCD_Cursor(0);
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
				LCD_Cursor(0);
				ucIdx++;
				Dsp_State = Dsp_Play;
			}
			else
			{
				LCD_DisplayString(1, (unsigned char*)RecordMenu[ucIdx]);
				LCD_Cursor(0);
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
					gucSongChoice = 3;
					break;
				case 8:
					gucSongChoice = 4;
					break;
				case 16: // previous menu
					ucIdx = 0;
					usCnt = 0;
					Dsp_State = Dsp_Intro_Rel;
					break;
				case 32:
				//break;
				case 64:
				//break;
				case 128:
				//break;
				default: //need to add error check
					break;
			}
			if(gucSongChoice > 0 && gucSongChoice < 5)
			{
				ucIdx = 0;
				usCnt = 0;
				Dsp_State = Dsp_Init;	
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
					LCD_Cursor(0);
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
			gucSongChoice = 7;
			Dsp_State = Dsp_Init;
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
					LCD_Cursor(0);
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

enum Ply_States{Ply_Init, Ply_Load, Ply_Off, Ply_On, Ply_Cleanup} Ply_State;

void TckFct_Play()
{
	static unsigned short	*pusNoteHold	= NULL; // short array for note frequencies 
	static unsigned short	*pusTimeHold	= NULL; // short array for note periods
	static unsigned short	usTimeCnt		= 0;	//  short for period tracking
	static unsigned short	usFrequency		= 0;
	static unsigned char	ucCnt			= 0;
	static unsigned char	ucSize			= 0;	// size of arrays for songs
	
	switch(Ply_State)
	{
		case -1:
			Ply_State = Ply_Init;
			break;
		case Ply_Init:
			if(gucSongChoice > 0 && gucSongChoice < 5)
			{
				Ply_State = Ply_Load;	
			}
			else
			{
				Ply_State = Ply_Init;
			}		
			break;
		case Ply_Load:
			switch(gucSongChoice)
			{
				case 1:
					ucSize = eeprom_read_byte(&SongSizeOne);
					pusNoteHold = (unsigned short*)malloc(ucSize*sizeof(unsigned short));
					memset(pusNoteHold, 0, ucSize*sizeof(unsigned short));
					pusTimeHold = (unsigned short*)malloc(ucSize*sizeof(unsigned short));
					memset(pusTimeHold, 0, ucSize*sizeof(unsigned short));
					eeprom_read_block((void*)pusNoteHold, (const void*)&SongOneNotes[0], sizeof(SongOneNotes));
					eeprom_read_block((void*)pusTimeHold, (const void*)&SongOneTime[0], sizeof(SongOneTime));
					Ply_State = Ply_Off;
					break;
				case 2:
					ucSize = eeprom_read_byte(&SongSizeTwo);
					pusNoteHold = (unsigned short*)malloc(ucSize*sizeof(unsigned short));
					memset(pusNoteHold, 0, ucSize*sizeof(unsigned short));
					pusTimeHold = (unsigned short*)malloc(ucSize*sizeof(unsigned short));
					memset(pusTimeHold, 0, ucSize*sizeof(unsigned short));
					eeprom_read_block((void*)pusNoteHold, (const void*)&SongTwoNotes[0], sizeof(SongTwoNotes));
					eeprom_read_block((void*)pusTimeHold, (const void*)&SongTwoTime[0], sizeof(SongTwoTime));
					Ply_State = Ply_Off;
					break; //will add back in when needed
				case 3:
					ucSize = eeprom_read_byte(&RecordOneSize);
					if(ucSize == 0)
					{
						LCD_DisplayString(1, (const unsigned char*)"No song recorded");
						Ply_State = Ply_Cleanup;
					}
					else
					{
						pusNoteHold = (unsigned short*)malloc(sizeof(RecordOneNotes));
						memset(pusNoteHold, 0, sizeof(RecordOneNotes));
						pusTimeHold = (unsigned short*)malloc(sizeof(RecordOneTime));
						memset(pusTimeHold, 0, sizeof(RecordOneTime));
						eeprom_read_block((void*)pusNoteHold, (const void*)&RecordOneNotes[0], sizeof(RecordOneNotes));
						eeprom_read_block((void*)pusTimeHold, (const void*)&RecordOneTime[0], sizeof(RecordOneTime));
						Ply_State = Ply_Off;
					}
					break;
				case 4:
					ucSize = eeprom_read_byte(&RecordTwoSize);
					if(ucSize == 0)
					{
						LCD_DisplayString(1, (const unsigned char*)"No song recorded");
						Ply_State = Ply_Cleanup;
					}
					else
					{
						pusNoteHold = (unsigned short*)malloc(sizeof(RecordTwoNotes));
						memset(pusNoteHold, 0, sizeof(RecordTwoNotes));
						pusTimeHold = (unsigned short*)malloc(sizeof(RecordTwoTime));
						memset(pusTimeHold, 0, sizeof(RecordTwoTime));
						eeprom_read_block((void*)pusNoteHold, (const void*)&RecordTwoNotes[0], sizeof(RecordTwoNotes));
						eeprom_read_block((void*)pusTimeHold, (const void*)&RecordTwoTime[0], sizeof(RecordTwoTime));
						Ply_State = Ply_Off;
					}
					break;
				default:
					Ply_State = Ply_Init;
					break;			
			}
			break;
		case Ply_Off:
			PWM_on();
			usFrequency = pusNoteHold[ucCnt];
			LCD_ClearScreen();
			if(usFrequency != 0)
			{
				LCD_WriteData(SongNote(usFrequency));
				CustomChar(3);
				LCD_Cursor(3);
				LCD_WriteData(3);	
			}
			LCD_Cursor(0);
			set_PWM(usFrequency);
			usTimeCnt++;
			Ply_State = Ply_On;
			break;
		case Ply_On:
			if(usTimeCnt < pusTimeHold[ucCnt] && ucCnt < ucSize)
			{
				if(usTimeCnt == 0)
				{
					PWM_on();
					usFrequency = pusNoteHold[ucCnt];
					LCD_ClearScreen();
					if(usFrequency != 0)
					{
						LCD_WriteData(SongNote(usFrequency));
						CustomChar(3);
						LCD_Cursor(3);
						LCD_WriteData(3);
					}
					LCD_Cursor(0);
					set_PWM(usFrequency);
				}
				usTimeCnt++;
				Ply_State = Ply_On;
			}
			else if(ucCnt < ucSize)
			{
				usTimeCnt = 0;
				ucCnt++;
				Ply_State = Ply_On;
			}
			else
			{
				set_PWM(0);
				Ply_State = Ply_Cleanup;
			}
			break;
		case Ply_Cleanup:
			if(pusNoteHold != NULL)
			{
				free(pusNoteHold);
				pusNoteHold = NULL;
			}
			if(pusTimeHold != NULL)
			{
				free(pusTimeHold);
				pusTimeHold = NULL;
			}
			gucSongChoice = 0;
			usTimeCnt = 0;
			ucSize = 0;
			ucCnt = 0;
			PWM_off();
			Ply_State = Ply_Init;
			break;
		default:
			break;
	}
	switch(Ply_State)
	{
		case Ply_Init:
		break;
		case Ply_Load:
		break;
		case Ply_Off:
		break;
		case Ply_On:
		break;
		case Ply_Cleanup:
		break;
		default:
		break;
	}
}

enum Rec_States{Rec_Init, Rec_Listen, Rec_Store, Rec_Cleanup} Rec_State;

void TckFct_Rec()
{
	static unsigned short	pusRecNote[80];
	static unsigned short	pusRecTime[80];
	static unsigned short   usTime			= 0;
	static const unsigned short	usMaxPer		= 64000;
	static unsigned short	usNoResp		= 0;
	static unsigned char	ucCurNote		= 0x00;
	static unsigned char	ucPrevNote		= 0x00;
	static unsigned char	ucSize			= 0;	
	static unsigned char	ucRecSong		= 1;
	static const unsigned char		MAXSIZE			= 80;
	static unsigned char	ucTmpSize		= 0;
	
	
	tmpA = ~PINA;
	ucCurNote = tmpA;
	switch(Rec_State)
	{
		case -1:
			Rec_State = Rec_Init;
			break;
		case Rec_Init:
			if(gucSongChoice > 6 && tmpA == 0x00)
			{
				PWM_on();
				Rec_State = Rec_Listen;	
			}
			else
			{
				Rec_State = Rec_Init;
			}	
			break;
		case Rec_Listen:
			
			if(usNoResp < 3000 && ucSize < MAXSIZE)
			{
				if(ucSize == 0)
				{
					LCD_ClearScreen();
					ucPrevNote = ucCurNote;
					ucSize++;
					if(ucCurNote != 0x00)
					{
						LCD_WriteData(SongNote(NoteFrequency(ucCurNote)));
						LCD_Cursor(0);
						set_PWM(NoteFrequency(ucCurNote));	
					}
				}
				if(ucPrevNote != ucCurNote || usTime >= (usMaxPer-1))
				{
					if(ucCurNote == 0x00)
					{
						LCD_ClearScreen();
						LCD_Cursor(0);
					}
					else
					{
						LCD_ClearScreen();
						LCD_WriteData(SongNote(NoteFrequency(ucCurNote)));
						LCD_Cursor(0);	
					}
					pusRecTime[ucSize-1] = usTime;
					pusRecNote[ucSize-1] = NoteFrequency(ucPrevNote);
					ucSize++;
					usTime = 0;
					ucPrevNote = ucCurNote;
					usNoResp = 0;
					set_PWM(NoteFrequency(ucCurNote));
				}
				else
				{
					if(ucCurNote == 0x00)
					{
						usNoResp++;
					}
				}
				usTime++;
				Rec_State = Rec_Listen;
			}
			else
			{
				pusRecTime[ucSize-1] = usNoResp - usTime;
				pusRecNote[ucSize-1] = NoteFrequency(ucPrevNote);
				//ucSize++;
				Rec_State = Rec_Store;
			}
			break;
		case Rec_Store:
			if(ucRecSong == 1)
			{
				ucRecSong = 2;
				ucTmpSize = eeprom_read_byte(&RecordOneSize);
				if(ucTmpSize > ucSize)
				{
					ucTmpSize = (ucTmpSize - ucSize) + ucSize;
					eeprom_update_byte(&RecordOneSize, ucSize);
					eeprom_update_block((const void*)&pusRecNote[0], (void*)&RecordOneNotes[0], ucTmpSize);
					eeprom_update_block((const void*)&pusRecTime[0], (void*)&RecordOneTime[0], ucTmpSize);
				}
				else
				{
					eeprom_update_byte(&RecordOneSize, ucSize);
					eeprom_update_block((const void*)&pusRecNote[0], (void*)&RecordOneNotes[0], ucSize);
					eeprom_update_block((const void*)&pusRecTime[0], (void*)&RecordOneTime[0], ucSize);
				}
			}
			else
			{
				ucTmpSize = eeprom_read_byte(&RecordTwoSize);
				if(ucTmpSize > ucSize)
				{
					ucTmpSize = (ucTmpSize - ucSize) + ucSize;
					eeprom_update_byte(&RecordOneSize, ucSize);
					eeprom_update_block((const void*)&pusRecNote[0], (void*)&RecordTwoNotes[0], ucTmpSize);
					eeprom_update_block((const void*)&pusRecTime[0], (void*)&RecordTwoTime[0], ucTmpSize);
				}
				else
				{
					eeprom_update_byte(&RecordOneSize, ucSize);
					eeprom_update_block((const void*)&pusRecNote[0], (void*)&RecordTwoNotes[0], ucSize);
					eeprom_update_block((const void*)&pusRecTime[0], (void*)&RecordTwoTime[0], ucSize);
				}
				ucRecSong = 1;
			}
			Rec_State = Rec_Cleanup;
			break;
		case Rec_Cleanup:
			ucSize = 0;
			usTime = 0;
			usNoResp = 0;
			memset(&pusRecNote[0], 0, sizeof(pusRecNote));
			memset(&pusRecTime[0], 0, sizeof(pusRecTime));
			ucCurNote = 0x00;
			ucPrevNote = 0x00;
			gucSongChoice = 0;
			Rec_State = Rec_Init;
			PWM_off();
			break;
		default:
			break;
	}
	switch(Rec_State)
	{
		case Rec_Init:
			break;
		case Rec_Listen:
			break;
		case Rec_Store:
			break;
		case Rec_Cleanup:
			break;
		default:
			break;
	}
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
	Ply_State = -1;
	Rec_State = -1;
	
    while(1)
    {
		TckFct_Display();
		TckFct_Play();
		TckFct_Rec();
        while(!TimerFlag);
        TimerFlag = 0;
    }
}