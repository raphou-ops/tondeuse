/**
* @file Laboratoire1_Systemes et communication_LCD et PC
* @brief Programme principal qui permet a l'utilisateur via un atmega32u4 et un fil USART a USB de communiquer avec la console PC ou un ecran LCD HD44780. Ce montage comporte aussi deux bouttons qui en fonction du boutton appuyer le programme affiche respecetivement 1 (pour SW1) ou 2 (pourSW2) sur la console PC. Si on ecrit un des caracteres dans console PC ceux-ci vont etre envoyes via la communication usart 8E1 de 10000 bauds et ces caracteres vont etre affiches sur le LCD.
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 06 fevrier 2022
*
* @mainpage
* Programme qui utilise ainsi les pull up interne pour recevoir le signal des deux boutons. De plus la communication USART est de type 8E1 10000 bauds et le montage est monter sur une plaquette blanche avec deux potentiometres pour gerer la luminosite et le contraste du LCD 44780.
* @author Adrian Catuneanu
* @section MainSection1 Description
*
* Ce programme propose les fonctionalites suivantes :
*
* - <b>Ecrire des caracteres sur l'ecran LCD</b> : L'utilisateur peut ecrire n'importe quel mot sur le lcd ainsi que retourner a la ligne 1 position 1 avec \r et aller a la deuxieme ligne 2 eme position en envoyant \n sur la Console vers l'Arduino Micro.
* - <b>Afficher 1 ou 2 sur la console PC</b> : Si l'utilisateur appuye sur SW1 le caracter '1' se ait transmettre vers la console pc et '2' comme caracter quand la SW2 est appuyer. Le refresh de boutons se fait aux 100ms via un timer0.
* - <b>Controle de la luminosite et contraste du LCD</b> : La luminosite ainsi que le contraste du HD44780 peut se faire ajuster a l'aide de deux potentiometres RV1 et RV2.
*/

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "lcd.h"
#include "USART.h"
#include "ZEDF9P.h"
#include <stdlib.h>

#define USB_ON 1
#define USB_OFF 0
#define USART1_ON 1
#define USART1_OFF 0
#define PVT 7
#define POLSSH 2
#define ODO 6

volatile uint16_t cntDixiemeDeSec = 0;
volatile uint8_t refreshBouton = 1;
uint8_t etatPrecedant = 0;
uint8_t etatPrecedant2 = 0;
char variable[8];
char *input_cr;
uint8_t etat = 0;
char msg[32];
uint8_t pos = 0;
uint8_t row=0;
uint8_t cnt;
float lat = 0;
float lon = 0;

unsigned int i;
char buffer [sizeof(unsigned int)*8+1];

uint8_t tmp;

uint16_t calculerCRC(uint8_t*trame,uint8_t size);	
void envoieConfigMsg( uint8_t id,uint8_t etatUart1,uint8_t etatUsb);
void envoieConfigRate(uint8_t frequence);
void envoieConfigPortUart1(uint32_t baudrate);
uint8_t trameResetOdometre[8]={0xB5,0x62,0x01,0x10,0x00,0x00,0x11,0x34};
uint16_t calculerCRC(uint8_t*trame,uint8_t size)
{
	uint8_t checksumA=0;
	uint8_t checksumB=0;
	uint16_t retour=0;
	size=size+6;
	for(uint8_t n=2;n<size;n++)
	{
		checksumA = checksumA + trame[n];
		checksumB = checksumB + checksumA;
	}
	retour=checksumB<<8|checksumA;
	return retour;
}

void envoieConfigPortUart1(uint32_t baudrate)
{
	//{0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x79}
	uint16_t crc=0;
	uint8_t trameConfigEnvoi[28]={0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, (baudrate&0xFF),(baudrate>>8&0xFF),(baudrate>>16&0xFF), (baudrate>>24&0xFF), 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0};
	crc=calculerCRC(trameConfigEnvoi,20);
	trameConfigEnvoi[26]=crc&0xFF;
	trameConfigEnvoi[27]=crc>>8;
	usartSendBytes(trameConfigEnvoi,28);
}

void envoieConfigRate(uint8_t frequence)
{
	//{0xB5,0x62,0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00,0x01,0x39}
	uint16_t crc=0;
	uint16_t freq=1000/frequence;
	uint8_t trameConfigEnvoi[14]={0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, (freq&0xFF),(freq>>8), 0x01,0x00 , 0x01, 0x00, 0, 0};
	crc=calculerCRC(trameConfigEnvoi,6);
	trameConfigEnvoi[12]=crc&0xFF;
	trameConfigEnvoi[13]=crc>>8;
	usartSendBytes(trameConfigEnvoi,14);	
}

void envoieConfigMsg(uint8_t id,uint8_t etatUart1,uint8_t etatUsb)
{
	
	uint16_t crc=0;
	uint8_t trameConfigEnvoi[16]={0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, id, 0x00, etatUart1, 0x00, etatUsb, 0x00, 0x00, 0, 0};
	crc=calculerCRC(trameConfigEnvoi,8);
	trameConfigEnvoi[14]=crc&0xFF;
	trameConfigEnvoi[15]=crc>>8;
	usartSendBytes(trameConfigEnvoi,16);
	
}		

int main(void)
{
	DDRB |=(1<<7);//led on board
	// 	BT1_INIT();
	// 	BT2_INIT();
	lcdInit();
	usartInit(9600,16000000);
	lcdPuts("Hello world");
	TCCR0B |= (1<<CS01) | (1<<CS00); //avec diviseur de clock /64.
	TCCR0A |= (1<<WGM01);//Configuration du timer 0 en CTC
	TIMSK0 |= (1<<OCIE0A);//Output Compare Match A Interrupt Enable.
	OCR0A = 249;//Top a la valeur 249 afin de obtenir un periode de 1ms fixe.
	sei();//fait appel aux interruptions global
	
	envoieConfigPortUart1(9600);
	envoieConfigRate(1);
	envoieConfigMsg(PVT,USART1_ON,USART1_OFF);
	PORTL |= (1<<7);//pull ups internes pour bouton odo reset
	
	
	
	//pour les pwm du pont H 1 driver
	/*DDRH|=(1<<3)|(1<<4);
	TCCR4A=0b10100011;
	TCCR4B=0b00011001;
	OCR4A=250;
	OCR4B=240;*/
	
	while (1)
	{
		
 		if(pos>=16)
 		{
 			pos=0;
 			row=!row;
 			
		}
		if((PINL &(1<<7))==0)
		{
			usartSendBytes(trameResetOdometre,8);
		}
		
		if(usartRxAvailable())
		{
			tmp=usartRemRxData();
			if(parseRxUbxNavPvt(tmp))
			{
				PORTB^=(1<<7);
				lcdClearScreen();
				lon = getNavPvtLon();
				lat = getNavPvtLat();
				sprintf(msg, "%f", (lat*1e-7));
				lcdSetPos(0,0);
				lcdPuts(msg);
				sprintf(msg, "%f", (lon*1e-7));
				lcdSetPos(0,1);
				lcdPuts(msg);
			}
		}
		if(refreshBouton==1)
		{
			refreshBouton = 0;
		}
		
		
	}
}
/**
* @brief //A chaque interruption du timer 0 le programme execute DixiemeDeSec++ comme ca on verifie l'etat des boutons a chaque 100ms.
*/
ISR(TIMER0_COMPA_vect)//Quand l'interruption globale est appeller le programme vient executer le vecteur Comparatif.
{
	cntDixiemeDeSec++;
	if(cntDixiemeDeSec <= 1000)
	{
		cntDixiemeDeSec -= 1000;
		refreshBouton = 1;
	}
}
