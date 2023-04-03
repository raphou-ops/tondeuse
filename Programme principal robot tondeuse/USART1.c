/**
* @file USART.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 13 septembre 2021
* @brief Cette bibliothèque offre les definitions des fonctions pour la communication USART a l'aide du atmega32u4 avec le fil usb USART.
*/

#include "USART1.h"

#define TX_BUFFER_SIZE_GPS 128
#define RX_BUFFER_SIZE_GPS 128

volatile uint8_t txBufferGPS[TX_BUFFER_SIZE_GPS];
volatile uint8_t rxBufferGPS[RX_BUFFER_SIZE_GPS];
volatile uint8_t txBufferInGPS = 0;
volatile uint8_t rxBufferInGPS = 0;
uint8_t sendMessageGPS = 1;
uint8_t txBufferOutGPS = 0;
uint8_t rxBufferOutGPS = 0;
uint8_t rxCntGPS = 0;
uint8_t txCntGPS = 0;
uint8_t rxContentGPS = 0;
uint8_t nbOctetGPS = 0;
uint8_t cntStrGPS = 0;

/**
* @brief Cette fonction gere la transmission de donnes un bit a la fois.
* @param uint8_t u8data Recoit la commande qu'on veut faire executer au LCD.
* @return le retour est de type uint8_t et cette fonction retourne sois 0 si le buffer de transmission est plein et retourne 1 s'il est vide.
*/

uint8_t usartGpsSendByte(uint8_t u8Data)
{
	if(txCntGPS<TX_BUFFER_SIZE_GPS)
	{
		txBufferGPS[txBufferInGPS++] = u8Data;
		cli();
		txCntGPS++;
		sei();
		if(txBufferInGPS>=TX_BUFFER_SIZE_GPS)
		txBufferInGPS = 0;
		UCSR1B |= (1<<UDRIE1);
		return 0;
	}
	return 1;
}

/**
* @brief Cette fonction sert a initialiser la tranmission de donne en mode 8n1 a une vitess de 10000 bauds.
* @param uint32_t baudRate Recoit la valeur de vitesse de communication desirer par l'utilisateur.
* @param uint32_t fcpu Recoit la vitesse du cpu de l'utilisateur
*/

void usartGpsInit(uint32_t baudRate, uint32_t fcpu)
{
	
	UCSR1B = (1<<RXEN1)|(1<<TXEN1);//active la transmission et la reception
	//UCSR1C = 0b00100110;//met en mode 8 bit parite paire
	UCSR1C = 0b00000110;
	
	UBRR1 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR1 pour avoir une vitesse de 10000 bauds
	UCSR1B |= (1<<RXCIE1);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre de bits restant pour la reception (place dans le rx Buffer).
* @param aucun
* @return le type de retour est uin8_t qui s'agit du nombre de bits qui occupent le rx Buffer.
*/

uint8_t usartGpsRxAvailable()
{
	return rxCntGPS;
}

/**
* @brief Cette fonction a pour but de recuper la valeur transmise par l'ordinateur et la recuper pour ensuite la metre dans une variable.
* @param aucun
* @return Le type de retour est uint8_t, le retour signifie la valeur decimale entree par l'utilisateur dans la console de communication.
*/

uint8_t usartGpsRemRxData()
{
	if(rxCntGPS)
	{
		rxContentGPS = rxBufferGPS[rxBufferOutGPS++];
		cli();
		rxCntGPS--;
		sei();
		if(rxBufferOutGPS>=RX_BUFFER_SIZE_GPS)
		rxBufferOutGPS = 0;
	}
	return rxContentGPS;
}

/**
* @brief Cette fonction sert recuperer une string a l'aide de la fonction usartSendByte qui transmet bit a bit. Cette fonction envoye la string a transmettre vers la console d'affichage.
* @param const char*str Recoit la string desirer par l'utilisateur a faire afficher sur la console PC.
* @return cette fonction retourne sous type uint8_t la longueur de la string qui vient d'etre envoyer.
*/

uint8_t usartGpsSendString(const char * str)
{
	uint8_t cntStr = 0;
	for(uint8_t i = 0;str[i];i++)
	{
		if(!usartGpsSendByte(str[i]))
		cntStr++;
	}
	
	return cntStr;
}

/**
* @brief Cette fonction gere l'affichage de plusieurs bits de valeurs decimale uint8_t sur la console PC.
* @param const uint8_t * source Est un pointeur qui sert a pointer pour parcourir la valeur decimale a transmettre.
* @param uint8_t size Recoit la longueur de la string a transmettre vers le PC.
* @return nombre d'octets ajoutes a la transmission
*/

uint8_t usartGpsSendBytes(const uint8_t * source, uint8_t size)
{
	uint8_t nbOctet = 0;
	
	for(uint8_t i = 0;i<size;i++)
	{
		if(!usartGpsSendByte(source[i]))
		nbOctet++;
		
	}
	return nbOctet;
}

ISR(USART1_UDRE_vect)//a chaque fois que l'interruption est appeller le programme tansmet des donnes avec le registre UDR1 si il y a bien une donne a transmettre.
{
	if(!txCntGPS)
	{
		UCSR1B &= ~(1<<UDRIE1);//desactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR1 = txBufferGPS[txBufferOutGPS++];//sert a transmettre les donnes du txBuffer vers le PC
		txCntGPS--;
		if(txBufferOutGPS >= TX_BUFFER_SIZE_GPS)
		txBufferOutGPS = 0;
	}
}
volatile uint8_t _usartRxTmpGps;
ISR(USART1_RX_vect)//cette interruption est semblable a la derniere mais celle ci gere la reception des donnes du atmega32u4 en mettant les valeurs du rx buffer dans UDR1.
{
	_usartRxTmpGps = UDR1;
	if(rxCntGPS<RX_BUFFER_SIZE_GPS)
	{
		rxBufferGPS[rxBufferInGPS++] = _usartRxTmpGps;//recoit les valeurs en reception et les met dans le regisre UDR1
		rxCntGPS++;
		if(rxBufferInGPS >= RX_BUFFER_SIZE_GPS)//Si le buffer de reception est plein le buffer se met a 0
		rxBufferInGPS = 0;
	}
}