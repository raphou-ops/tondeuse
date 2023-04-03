/**
* @file USART.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 13 septembre 2021
* @brief Cette bibliothèque offre les definitions des fonctions pour la communication USART a l'aide du atmega32u4 avec le fil usb USART.
*/

#include "USART0.h"

#define TX_BUFFER_SIZE_LIDAR 64
#define RX_BUFFER_SIZE_LIDAR 64

volatile uint8_t txBufferLiDAR[TX_BUFFER_SIZE_LIDAR];
volatile uint8_t rxBufferLiDAR[RX_BUFFER_SIZE_LIDAR];
volatile uint8_t txBufferInLiDAR = 0;
volatile uint8_t rxBufferInLiDAR = 0;
uint8_t sendMessageLiDAR = 1;
uint8_t txBufferOutLiDAR = 0;
uint8_t rxBufferOutLiDAR = 0;
uint8_t rxCntLiDAR = 0;
uint8_t txCntLiDAR = 0;
uint8_t rxContentLiDAR = 0;
uint8_t nbOctetLiDAR = 0;
uint8_t cntStrLiDAR = 0;

/**
* @brief Cette fonction gere la transmission de donnes un bit a la fois.
* @param uint8_t u8data Recoit la commande qu'on veut faire executer au LCD.
* @return le retour est de type uint8_t et cette fonction retourne sois 0 si le buffer de transmission est plein et retourne 1 s'il est vide.
*/

uint8_t usart0SendByte(uint8_t u8Data)
{
	if(txCntLiDAR<TX_BUFFER_SIZE_LIDAR)
	{
		txBufferLiDAR[txBufferInLiDAR++] = u8Data;
		cli();
		txCntLiDAR++;
		sei();
		if(txBufferInLiDAR>=TX_BUFFER_SIZE_LIDAR)
		txBufferInLiDAR = 0;
		UCSR0B |= (1<<UDRIE0);
		return 0;
	}
	return 1;
}

/**
* @brief Cette fonction sert a initialiser la tranmission de donne en mode 8n1 a une vitess de 10000 bauds.
* @param uint32_t baudRate Recoit la valeur de vitesse de communication desirer par l'utilisateur.
* @param uint32_t fcpu Recoit la vitesse du cpu de l'utilisateur
*/

void usart0Init(uint32_t baudRate, uint32_t fcpu)
{
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);//active la transmission et la reception
	//UCSR1C = 0b00100110;//met en mode 8 bit parite paire
	UCSR0C = 0b00000110;
	
	UBRR0 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR1 pour avoir une vitesse de 10000 bauds
	UCSR0B |= (1<<RXCIE0);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre de bits restant pour la reception (place dans le rx Buffer).
* @param aucun
* @return le type de retour est uin8_t qui s'agit du nombre de bits qui occupent le rx Buffer.
*/

uint8_t usart0RxAvailable()
{
	return rxCntLiDAR;
}

/**
* @brief Cette fonction a pour but de recuper la valeur transmise par l'ordinateur et la recuper pour ensuite la metre dans une variable.
* @param aucun
* @return Le type de retour est uint8_t, le retour signifie la valeur decimale entree par l'utilisateur dans la console de communication.
*/

uint8_t usart0RemRxData()
{
	if(rxCntLiDAR)
	{
		rxContentLiDAR = rxBufferLiDAR[rxBufferOutLiDAR++];
		cli();
		rxCntLiDAR--;
		sei();
		if(rxBufferOutLiDAR>=RX_BUFFER_SIZE_LIDAR)
		rxBufferOutLiDAR = 0;
	}
	return rxContentLiDAR;
}

/**
* @brief Cette fonction sert recuperer une string a l'aide de la fonction usartSendByte qui transmet bit a bit. Cette fonction envoye la string a transmettre vers la console d'affichage.
* @param const char*str Recoit la string desirer par l'utilisateur a faire afficher sur la console PC.
* @return cette fonction retourne sous type uint8_t la longueur de la string qui vient d'etre envoyer.
*/

uint8_t usart0SendString(const char * str)
{
	uint8_t cntStr = 0;
	for(uint8_t i = 0;str[i];i++)
	{
		if(!usart0SendByte(str[i]))
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

uint8_t usart0SendBytes(const uint8_t * source, uint8_t size)
{
	uint8_t nbOctet = 0;
	
	for(uint8_t i = 0;i<size;i++)
	{
		if(!usart0SendByte(source[i]))
		nbOctet++;
	}
	return nbOctet;
}

ISR(USART0_UDRE_vect)//a chaque fois que l'interruption est appeller le programme tansmet des donnes avec le registre UDR1 si il y a bien une donne a transmettre.
{
	if(!txCntLiDAR)
	{
		UCSR0B &= ~(1<<UDRIE0);//desactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR0 = txBufferLiDAR[txBufferOutLiDAR++];//sert a transmettre les donnes du txBuffer vers le PC
		txCntLiDAR--;
		if(txBufferOutLiDAR >= TX_BUFFER_SIZE_LIDAR)
		txBufferOutLiDAR = 0;
	}
}
volatile uint8_t _usartRxTmpLiDAR;
ISR(USART0_RX_vect)//cette interruption est semblable a la derniere mais celle ci gere la reception des donnes du atmega32u4 en mettant les valeurs du rx buffer dans UDR1.
{
	_usartRxTmpLiDAR = UDR0;
	if(rxCntLiDAR<RX_BUFFER_SIZE_LIDAR)
	{
		rxBufferLiDAR[rxBufferInLiDAR++] = _usartRxTmpLiDAR;//recoit les valeurs en reception et les met dans le regisre UDR1
		rxCntLiDAR++;
		if(rxBufferInLiDAR >= RX_BUFFER_SIZE_LIDAR)//Si le buffer de reception est plein le buffer se met a 0
		rxBufferInLiDAR = 0;
	}
}