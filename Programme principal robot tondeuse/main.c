/*
* Programme principal robot tondeuse.c
*
* Created: 2023-03-31 3:49:12 PM
* Author : adino
*/

#define F_CPU 16000000
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>

#include "USART0.h"
#include "USART1.h"
#include "USART2.h"
#include "lcd.h"
#include "ZEDF9P.h"
#include "LiDAR.h"

/*Variables globales GPS*/
#define USB_ON 1
#define USB_OFF 0
#define USART1_ON 1
#define USART1_OFF 0
#define PVT 7
#define POLSSH 2
#define ODO 9
#define ODO_MESSAGE 0x1E

volatile uint16_t cntDixiemeDeSec = 0;
volatile uint8_t refresh = 1;

uint8_t etat = 0;
char msgGpsLat[16];
char msgGpsLon[16];
char msgTime[20];
uint8_t pos = 0;
uint8_t row = 0;
uint8_t cnt;
float lat = 0;
float lon = 0;
uint8_t dataGps;
uint8_t setRate[14]={0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x37};

/*Variables globales Bluetooth*/
uint8_t dataBluetooth = 0;

/*Variables globales LiDAR*/
uint8_t dataLiDAR = 0;
char msgLiDAR[20];

/*Variables globales encodeurs*/
volatile uint8_t refreshDist = 1;
volatile uint8_t encodeurEtatPrecedent = 0;
volatile uint8_t encodeurEtatPrecedent2 = 0;
volatile uint16_t encodeurGCnt = 0;
volatile uint16_t encodeurDCnt = 0;

char msgEncoD[8];
char msgEncoG[8];

/*Variables globales joystick*/
int joysticX=0;
int joysticY=0;
uint16_t receptionAdc=0;
uint8_t triAdc=0;
int vitesseMoteurG=0;
int vitesseMoteurD=0;

#define BT2() ((PINA&(1<<0))==0)
#define BT1() ((PINA&(1<<1))==0)
#define BT4() ((PINA&(1<<2))==0)
#define BT3() ((PINA&(1<<3))==0)

#define JOYSTICK_X 1
#define JOYSTICK_Y 2
#define VIT_MIN 512

char msg[8];

/*Variables clavier matriciel*/
char key = 0;
char lastKey = 0;
uint8_t cntPassword = 0;
char password[4] = {'1','2','3','4'};
char essaiUser[4];
char passwordValid = 0;
uint8_t modeTondeuse = 0;

uint8_t cntMenu = 0;

uint8_t set = 0;
uint8_t setTemps = 0;
uint8_t heureCoupe = 0;
uint8_t minutesCoupe = 0;
char tempsCoupe[5];

void adcInit();//initialise l'adc
void moteurInit();
char lireClavier();
void controleManuel();

void adcInit()
{
	ADMUX|=(1<<REFS0);//10 bit et compare avec vcc
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2);//divise le clock par 32 et active l'adc

	DIDR0|=0x06;//mets les sortie en adc 1 et adc 2
	DIDR2|=0x01;//mets la pin en adc8
	sei();
}
void moteurInit()
{
	DDRH|=(1<<4)|(1<<5);//moteur arrière2

	TCCR4A=0b10101011;
	TCCR4B=0b00011001;
	OCR4A=1024;//top
	OCR4B=70;
	OCR4C=0;

	DDRL|=(1<<4)|(1<<5);//moteur arrière 1
	TCCR5A=0b10101011;
	TCCR5B=0b00011001;
	OCR5A=1024;//top
	OCR5B=90;
	OCR5C=0;
	
	DDRB|=(1<<6); //moteur pour la lame
	TCCR1A=0b10101011;
	TCCR1B=0b00011001;
	OCR1A=1024;//top
	OCR1B=90;
	OCR1C=0;
}
char lireClavier()
{
	char digit = 0;
	
	if(BT1())
	{
		digit = '1';
	}
	else if(BT2())
	{
		digit = '2';
	}
	else if(BT3())
	{
		digit = '3';
	}
	else if(BT4())
	{
		digit = '4';
	}
	return digit;
}

void controleManuel()
{
	if(joysticY<0)
	{
		joysticY*=-1;

		if(joysticX>0)
		{
			vitesseMoteurG=joysticY-joysticX;
			vitesseMoteurD=joysticY;
		}
		else
		{
			vitesseMoteurD=joysticY+joysticX;
			vitesseMoteurG=joysticY;
		}

		if(vitesseMoteurD<0)
		vitesseMoteurD=0;
		if(vitesseMoteurG<0)
		vitesseMoteurG=0;
		OCR4B=0;
		OCR5B=(VIT_MIN+vitesseMoteurG);
		OCR4C=(VIT_MIN+vitesseMoteurD);
		OCR5C=0;
	}
	else
	{
		if(joysticX>0)
		{
			vitesseMoteurG=joysticY-joysticX;
			vitesseMoteurD=joysticY;
		}
		else
		{
			vitesseMoteurD=joysticY+joysticX;
			vitesseMoteurG=joysticY;
		}
		if(vitesseMoteurD<0)
		vitesseMoteurD=0;
		if(vitesseMoteurG<0)
		vitesseMoteurG=0;
		OCR4B=(VIT_MIN+vitesseMoteurD);
		OCR5B=0;
		OCR4C=0;
		OCR5C=(VIT_MIN+vitesseMoteurG);
	}
}


int main(void)
{
	lcdInit();
	
	PORTA|=0x0F;//initclavier
	
	_delay_ms(100);
	
	lcdPuts("Enter Password:");
	lcdSetPos(0,1);
	
	adcInit();
	moteurInit();
	
	ADCSRA |= (1<<ADSC); //debut conversion ADC.
	
	TCCR0B |= (1<<CS01) | (1<<CS00); //avec diviseur de clock /64.
	TCCR0A |= (1<<WGM01);//Configuration du timer 0 en CTC
	TIMSK0 |= (1<<OCIE0A);//Output Compare Match A Interrupt Enable.
	OCR0A = 249;//Top a la valeur 249 afin de obtenir un periode de 1ms fixe.
	
	PORTJ |= 0b00011000;//active pull-up sur PB4 et PB5
	PCICR |= 0b00000010;//active la lecture sur PCINT4 et PCINT5
	PCMSK1 |= 0b00110000;//active les interruption sur PCINT4 et PCINT5
	
	usart0Init(115200,16000000);
	usartGpsInit(9600,16000000);
	usart2Init(9600,16000000);
	
	envoieConfigPortUart1(9600);
	envoieConfigRate(1);
	envoieConfigMsg(PVT,USART1_ON,USB_ON);
	usartGpsSendBytes(setRate,14);
	
	do{
		key = lireClavier();
		
		if((key)&&(!lastKey))
		{
			lcdSetPos(cntPassword,1);
			essaiUser[cntPassword++] = key;
			lcdPutc(key);
			if(cntPassword == 4)
			{
				for(uint8_t index = 0; index<4; index++)
				{
					if(essaiUser[index] == password[index])
					{
						passwordValid = 1;
					}
					else
					{
						lcdClearScreen();
						lcdPuts("Invalid password");
						lcdSetPos(0,1);
						cntPassword = 0;
						passwordValid = 0;
					}
				}
			}
		}
		lastKey = key;
	}while(!passwordValid);
	passwordValid = 0;
	lcdClearScreen();
	lcdSetPos(0,0);
	lcdPuts("Mode Manuel  <-");
	
	while (1)
	{
		key = lireClavier();
		
		if((key)&&(!lastKey))
		{
			lcdClearScreen();
			lcdSetPos(0,0);
			switch(key)
			{
				case '1':
				modeTondeuse = cntMenu;
				set = 1;
				break;
				
				case '2':
				if(cntMenu>0)
				cntMenu --;
				else
				cntMenu = 4;
				
				break;
				
				case '3':
				if(cntMenu<4)
				cntMenu ++;
				else
				cntMenu = 0;
				break;
				
				case '4':
				cntMenu = modeTondeuse;
				set = 1;
				break;
			}
			if(cntMenu == 0)
			{
				lcdPuts("Mode Manuel ");
			}
			else if(cntMenu == 1)
			{
				lcdPuts("Mode Prog   ");
			}
			else if(cntMenu == 2)
			{
				lcdPuts("Mode Auto   ");
			}
			else if(cntMenu == 3)
			{
				lcdPuts("Set time    ");
			}
			else if(cntMenu == 4)
			{
				lcdPuts("Set password");
			}
			if(set)
			{
				set = 0;
				setTemps = 0;
				passwordValid = 0;
				lcdSetPos(13,0);
				lcdPuts("<-");
			}
		}
		lastKey = key;
		
		if(refresh)
		{
			refresh = 0;
			
			switch(modeTondeuse)
			{
				case 0:
				controleManuel();
				if(getBoutonX() == 0x31)
				{
					OCR1B=1024;
					DDRB|=(1<<6);
				}
				else
				{
					OCR1B=90;
					DDRB &= ~(1<<6);
				}
				lcdSetPos(0,1);
				lcdPuts(msgTime);
				break;
				
				case 1:
				controleManuel();
				if(getBoutonO()==0x31)
				{
					lcdClearScreen();
					lcdSetPos(0,0);
					lcdPuts(msgGpsLon);
					lcdSetPos(0,1);
					lcdPuts(msgGpsLat);
				}
				lcdSetPos(11,1);
				lcdPuts(msgTime);
				break;
				
				case 2:
				lcdSetPos(0,1);
				lcdPuts(msgTime);
				break;
				
				case 3:
				lcdSetPos(0,1);
				sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
				do
				{
					key = lireClavier();
					
					if((key)&&(!lastKey))
					{
						switch(key)
						{
							case '1':
							cntMenu = 3;
							modeTondeuse = 3;
							setTemps = 1;
							lcdClearScreen();
							lcdSetPos(0,0);
							lcdPuts("Set time  OK");
							lcdSetPos(0,1);
							lcdPuts(msgTime);
							break;
							
							case '2':
							if(heureCoupe<23)
							heureCoupe++;
							else
							heureCoupe = 0;
							
							if((minutesCoupe<10) && (heureCoupe<10))
							{
								sprintf(tempsCoupe,"0%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else if(minutesCoupe<10)
							{
								sprintf(tempsCoupe,"%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else if (heureCoupe<10)
							{
								sprintf(tempsCoupe,"0%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else
							{
								sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							lcdSetPos(0,1);
							lcdPuts(tempsCoupe);
							break;
							
							case '3':
							if(minutesCoupe<59)
							minutesCoupe++;
							else
							minutesCoupe = 0;
							
							if((minutesCoupe<10) && (heureCoupe<10))
							{
								sprintf(tempsCoupe,"0%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else if(minutesCoupe<10)
							{
								sprintf(tempsCoupe,"%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else if (heureCoupe<10)
							{
								sprintf(tempsCoupe,"0%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							else
							{
								sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
							}
							lcdSetPos(0,1);
							lcdPuts(tempsCoupe);
							break;
							
							case '4':
							cntMenu = 3;
							modeTondeuse = 3;
							setTemps = 1;
							lcdClearScreen();
							memset(tempsCoupe, 0, sizeof(tempsCoupe));
							lcdSetPos(0,0);
							lcdPuts("Set time    ");
							lcdSetPos(0,1);
							lcdPuts(msgTime);
							break;
						}
					}
					lastKey = key;
				} while (setTemps == 0);
				break;
				
				case 4:
				do{
					key = lireClavier();
					
					if((key)&&(!lastKey))
					{
						lcdSetPos(cntPassword,1);
						essaiUser[cntPassword++] = key;
						lcdPutc(key);
					}
					lastKey = key;
					if(cntPassword == 4)
					{
						for(uint8_t index = 0; index<4; index++)
						{
							password[index] = essaiUser[index];
						}
						cntMenu = 4;
						modeTondeuse = 4;
						passwordValid = 1;
						cntPassword = 0;
						lcdClearScreen();
						lcdSetPos(0,0);
						lcdPuts("Set password OK");
						lcdSetPos(0,1);
						lcdPuts(msgTime);
					}
				}while(!passwordValid);
				break;
			}
		}
		
		if(usart0RxAvailable())
		{
			dataLiDAR = usart0RemRxData();
			if(parseRxLidar(dataLiDAR))
			{
				sprintf(msgLiDAR,"%d",getDist());
				//sprintf(msgBluetooth,"S;%f;%f;%d;%d;%d;%d;",lat,lon,45,3200,6700,70);
			}
		}
		if(usart2RxAvailable())
		{
			dataBluetooth = usart2RemRxData();
			if(parseBluetooth(dataBluetooth))
			{
				joysticX = getJoystickGaucheX();
				joysticY = getJoystickGaucheY();
			}
		}
		if(usartGpsRxAvailable())
		{
			dataGps = usartGpsRemRxData();
			if(parseRxUbxNavPvt(dataGps))
			{
				sprintf(msgGpsLat, "%f", getNavPvtLat());
				sprintf(msgGpsLon, "%f", getNavPvtLon());
				getNavPvtTime(msgTime);
			}
		}
		
		// 		if(refreshDist)
		// 		{
		// 			refreshDist = 0;
		// 			lcdClearScreen();
		// 			lcdSetPos(0,0);
		// 			sprintf(msgEncoD,"%d",encodeurDCnt);
		// 			lcdPuts(msgEncoD);
		// 			lcdSetPos(0,1);
		// 			sprintf(msgEncoG,"%d",encodeurGCnt);
		// 			lcdPuts(msgEncoG);
		// 		}
	}
}
/**
* @brief //A chaque interruption du timer 0 le programme execute DixiemeDeSec++ comme ca on verifie l'etat des boutons a chaque 100ms.
*/
ISR(TIMER0_COMPA_vect)//Quand l'interruption globale est appeller le programme vient executer le vecteur Comparatif.
{
	cntDixiemeDeSec++;
	if(cntDixiemeDeSec <= 200)
	{
		cntDixiemeDeSec -= 200;
		refresh = 1;
	}
}
ISR(PCINT1_vect)//A chaque detection du capteur une interruption se fait sur PCINT4 et PCINT5 qui correspond a la PINB4 et PINB5 du atmega32u4
{
	if(!(PINJ&(1<<3))&&(encodeurEtatPrecedent & (1<<3)))//si l'etat du capteur est low et qu'il y eu une detection comme etat precedant
	{
		encodeurDCnt++;//le compteur d'enchoches s'incremente
		refreshDist = 1;//on active le refresh l'affichache sur le LCD
	}
	encodeurEtatPrecedent = PINJ&(1<<3);
	if(!(PINJ&(1<<4))&&(encodeurEtatPrecedent2 & (1<<4)))//si l'etat du capteur est low et qu'il y eu une detection comme etat precedant
	{
		encodeurGCnt++;//le compteur d'enchoches s'incremente
		refreshDist = 1;//on active le refresh l'affichache sur la console
	}
	encodeurEtatPrecedent2 = PINJ&(1<<4);
}

ISR(ADC_vect)
{
	receptionAdc=ADC;
	triAdc=((ADCSRB&&0x8)<<5)|(ADMUX&0x1F);
	switch(triAdc)
	{
		case 0x0:
		// 		sprintf(msg,"p:%d",receptionAdc);
		// 		lcdSetPos(8,0);
		// 		lcdPuts(msg);
		ADMUX=0x41;
		break;

		case 0x1:
		/*joysticX=receptionAdc-512;*/
		// 		lcdSetPos(0,0);
		// 		lcdPuts("      ");
		// 		sprintf(msg,"x:%d",joysticX);
		// 		lcdSetPos(0,0);
		// 		lcdPuts(msg);

		ADMUX=0x42;
		break;

		case 0x2:
		/*joysticY=receptionAdc-512;*/
		// 		sprintf(msg,"y:%d",joysticY);
		// 		lcdSetPos(0,1);
		// 		lcdPuts("      ");
		// 		lcdSetPos(0,1);
		// 		lcdPuts(msg);
		ADMUX=0x40;
		ADCSRB|=(1<<MUX5);
		break;

		case 0x20:
		// 		sprintf(msg,"b:%d",receptionAdc);
		// 		lcdSetPos(8,1);
		// 		lcdPuts(msg);
		ADMUX=0x40;
		ADCSRB&=~(1<<MUX5);
		break;

		default:
		break;
	}
	ADCSRA|=(1<<ADSC);
}