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
#include <stdlib.h>

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

unsigned int i;
char buffer [sizeof(unsigned int)*8+1];
uint8_t trameEnable[18]={0xB5,0x62,0x06,0x01,0x08,0x00,0x01,0x07,0x00,0x01,0x00,0x00,0x00,0x00,0x18,0xE1};
unsigned char trameDisable[18]={0xb5,0x62,0x06,0x01,0x08,0x00,0x01,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x17,0xdc};
uint8_t trameSave[21]={0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x21, 0xAF};
uint8_t tmp;
// #define BT1_APPUYE()	((PINB & (1<<7))==0)
// #define BT2_APPUYE()	((PIND &(1<<6))==0)
// #define BT1_INIT() 	    PORTB |= (1<<7)
// #define BT2_INIT() 	    PORTD |= (1<<6)//pull ups intern
int main(void)
{
	DDRB = DDRB | (1<<7);
	// 	BT1_INIT();
	// 	BT2_INIT();
	lcdInit();
	usartInit(9600,16000000);
	
	TCCR0B |= (1<<CS01) | (1<<CS00); //avec diviseur de clock /64.
	TCCR0A |= (1<<WGM01);//Configuration du timer 0 en CTC
	TIMSK0 |= (1<<OCIE0A);//Output Compare Match A Interrupt Enable.
	OCR0A = 249;//Top a la valeur 249 afin de obtenir un periode de 1ms fixe.
	sei();//fait appel aux interruptions global
	
	while (1)
	{
		if(usartRxAvailable())
		{
			tmp=usartRemRxData();
			lcdPuts(utoa(tmp,buffer,16));
			pos+=2;
			lcdSetPos(pos,row);
			
			/*switch(tmp)
			{
				
				case 0xB5:
				lcdPuts(utoa(tmp,buffer,16));
				lcdSetPos(pos,row);
				pos+=2;
				break;
				case 0x62:
				lcdPuts(utoa(tmp,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
				break;
				case 0x01:
				lcdPuts(utoa(tmp,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
				break;
				case 0x07:
				lcdPuts(utoa(tmp,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
				break;
			}*/
		}
		
		if(refreshBouton==1)
		{
		//usartSendBytes(trameSave,21);
			refreshBouton = 0;
		
			if(pos>=16)
			{
				pos=0;
				row=!row;
			
			}
		//	usartSendBytes(trameDisable,sizeof(trameDisable));
		}
		
		/*if(usartRxAvailable())
		{
			i = usartRemRxData();

		
			
			//lcdPuts(utoa(i,buffer,16));
			
			if(i == 0xB5)
			{
				lcdPuts(utoa(i,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
				
			}
			else if(i == 0x62)
			{
				lcdPuts(utoa(i,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
			}
			else if(i == 0x01)
			{
				lcdPuts(utoa(i,buffer,16));
				lcdSetPos(pos,row);
				pos = pos+2;
			}
			
			
			
		
		}*/
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
