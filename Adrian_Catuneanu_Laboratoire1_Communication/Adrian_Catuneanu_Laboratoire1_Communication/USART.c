/**
* @file USART.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 13 septembre 2021
* @brief Cette bibliothèque offre les definitions des fonctions pour la communication USART a l'aide du atmega32u4 avec le fil usb USART.
*/

#include "USART.h"
 
#define TX_BUFFER_SIZE 128
#define RX_BUFFER_SIZE 128

volatile uint8_t txBuffer[TX_BUFFER_SIZE];
volatile uint8_t rxBuffer[RX_BUFFER_SIZE];
volatile uint8_t txBufferIn = 0;
volatile uint8_t rxBufferIn = 0;
uint8_t sendMessage = 1;
uint8_t txBufferOut = 0;
uint8_t rxBufferOut = 0;
uint8_t rxCnt = 0;
uint8_t txCnt = 0;
uint8_t rxContent = 0;
uint8_t nbOctet = 0;
uint8_t cntStr = 0;

/**
* @brief Cette fonction gere la transmission de donnes un bit a la fois.
* @param uint8_t u8data Recoit la commande qu'on veut faire executer au LCD.
* @return le retour est de type uint8_t et cette fonction retourne sois 0 si le buffer de transmission est plein et retourne 1 s'il est vide.
*/

uint8_t usartSendByte(uint8_t u8Data)
{
	if(txCnt<TX_BUFFER_SIZE)
	{
		txBuffer[txBufferIn++] = u8Data;
		cli();
		txCnt++;
		sei();
		if(txBufferIn>=TX_BUFFER_SIZE)
			txBufferIn = 0;
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

void usartInit(uint32_t baudRate, uint32_t fcpu)
{
	
	UCSR1B = (1<<RXEN1)|(1<<TXEN1);//active la transmission et la reception
	UCSR1C=0b00000110;
	UBRR1 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR1 pour avoir une vitesse de 10000 bauds
	UCSR1B |= (1<<RXCIE1);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre de bits restant pour la reception (place dans le rx Buffer).
* @param aucun
* @return le type de retour est uin8_t qui s'agit du nombre de bits qui occupent le rx Buffer.
*/

uint8_t usartRxAvailable()
{
	return rxCnt; 	
}

/**
* @brief Cette fonction a pour but de recuper la valeur transmise par l'ordinateur et la recuper pour ensuite la metre dans une variable.
* @param aucun
* @return Le type de retour est uint8_t, le retour signifie la valeur decimale entree par l'utilisateur dans la console de communication. 
*/

uint8_t usartRemRxData()
{
	if(rxCnt)
	{
		rxContent = rxBuffer[rxBufferOut++];
		cli();
		rxCnt--;
		sei();
		if(rxBufferOut>=RX_BUFFER_SIZE)
			rxBufferOut = 0;
	}
	return rxContent;
}

/**
* @brief Cette fonction sert recuperer une string a l'aide de la fonction usartSendByte qui transmet bit a bit. Cette fonction envoye la string a transmettre vers la console d'affichage.
* @param const char*str Recoit la string desirer par l'utilisateur a faire afficher sur la console PC.
* @return cette fonction retourne sous type uint8_t la longueur de la string qui vient d'etre envoyer.
*/

uint8_t usartSendString(const char * str)
{
	uint8_t cntStr = 0;
	for(uint8_t i = 0;str[i];i++)
	{
		if(!usartSendByte(str[i]))
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

uint8_t usartSendBytes(const uint8_t * source, uint8_t size)
{
	uint8_t nbOctet = 0;
	
 	for(uint8_t i = 0;i<size;i++)
	{
		if(!usartSendByte(source[i]))
			nbOctet++;
		
	}
	return nbOctet;
}

ISR(USART1_UDRE_vect)//a chaque fois que l'interruption est appeller le programme tansmet des donnes avec le registre UDR1 si il y a bien une donne a transmettre.
{
	if(!txCnt)
	{
		UCSR1B &= ~(1<<UDRIE1);//desactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR1 = txBuffer[txBufferOut++];//sert a transmettre les donnes du txBuffer vers le PC
		txCnt--;
		if(txBufferOut >= TX_BUFFER_SIZE)
			txBufferOut = 0;
	}
}
volatile uint8_t _usartRxTmp;
ISR(USART1_RX_vect)//cette interruption est semblable a la derniere mais celle ci gere la reception des donnes du atmega32u4 en mettant les valeurs du rx buffer dans UDR1.
{
	_usartRxTmp = UDR1;
	if(rxCnt<RX_BUFFER_SIZE)
	{
		rxBuffer[rxBufferIn++] = _usartRxTmp;//recoit les valeurs en reception et les met dans le regisre UDR1
		rxCnt++;
		if(rxBufferIn >= RX_BUFFER_SIZE)//Si le buffer de reception est plein le buffer se met a 0
			rxBufferIn = 0;
	}
	
}