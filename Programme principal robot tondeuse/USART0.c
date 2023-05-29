/**
* @file USART0.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.1
* @date 12 avril 2023
* @brief Cette bibliothèque offre les définitions des fonctions pour la communication USART à l'aide du atmega2560 avec le module TF-Luna mini Lidar. On utilise seulement les fonctions de réception de cette librairie pour recevoir la distance mesuré par le Lidar.
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
* @brief Cette fonction gere la transmission de données un octet à la fois.
*
* @param uint8_t u8data Recoit l'octet à transmettre par le lien UART.
*
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
* @brief Cette fonction sert a initialiser la tranmission de données en mode 8N1 à une vitesse de 115200 bauds.
*
* @param uint32_t baudRate Recoit la valeur de vitesse de communication desirer par l'utilisateur.
*
* @param uint32_t fcpu Recoit la vitesse du cpu de l'utilisateur
*/

void usart0Init(uint32_t baudRate, uint32_t fcpu)
{
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);//active la transmission et la reception
	//UCSR1C = 0b00100110;//met en mode 8 bit parite paire
	UCSR0C = 0b00000110;
	
	UBRR0 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR0 pour avoir une vitesse de 115200 bauds en fonction de la vitesse de clock de 16MHz
	UCSR0B |= (1<<RXCIE0);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre d'octets restants pour la reception (place dans le rx Buffer).
*
* @param aucun
*
* @return le type de retour est uin8_t qui s'agit du nombre d'octets qui occupent le Buffer de réception.
*/

uint8_t usart0RxAvailable()
{
	return rxCntLiDAR;
}

/**
* @brief Cette fonction a pour but de recuperer la valeur (octet) transmise par le TF-Luna mini Lidar et ensuite la mettre dans une variable.
*
* @param aucun
*
* @return Le type de retour est uint8_t, le retour signifie l'octet transmis par le module TF-Luna Lidar.
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
* @brief Cette fonction sert recuperer et envoyer une string a l'aide de la fonction usartSendByte qui transmet octet par octet. Cette fonction envoye la string a transmettre vers le TF-Luna mini Lidar.
*
* @param const char*str Recoit la string desirer par l'utilisateur a transmettre vers le TF-Luna mini Lidar.
*
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
* @brief Cette fonction gere l'envoi de plusieurs octets uint8_t vers le TF-Luna mini Lidar.
*
* @param const uint8_t * source Est un pointeur qui sert a pointer le tableau d'octets à envoyer.
*
* @param uint8_t size Recoit la longueur du tableau d'octets à envoyer vers le TF-Luna mini Lidar.
*
* @return nombre d'octets ajoutés à la transmission
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

ISR(USART0_UDRE_vect)//à chaque fois que l'interruption est appeller le programme tansmet des données avec le registre UDR0 s'il y a bien une donne à transmettre.
{
	if(!txCntLiDAR)
	{
		UCSR0B &= ~(1<<UDRIE0);//désactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR0 = txBufferLiDAR[txBufferOutLiDAR++];//sert a transmettre les données du txBuffer vers le TF-Luna mini Lidar
		txCntLiDAR--;
		if(txBufferOutLiDAR >= TX_BUFFER_SIZE_LIDAR)
		txBufferOutLiDAR = 0;
	}
}
volatile uint8_t _usartRxTmpLiDAR;
ISR(USART0_RX_vect)//cette interruption est semblable à la dernière, mais celle-ci gere la reception des données du atmega2560 en mettant les valeurs du rx buffer dans UDR0.
{
	_usartRxTmpLiDAR = UDR0;
	if(rxCntLiDAR<RX_BUFFER_SIZE_LIDAR)
	{
		rxBufferLiDAR[rxBufferInLiDAR++] = _usartRxTmpLiDAR;//recoit les valeurs en reception et les met dans le regisre UDR0
		rxCntLiDAR++;
		if(rxBufferInLiDAR >= RX_BUFFER_SIZE_LIDAR)//Si le buffer de reception est plein l'état de reception du buffer se met a 0
		rxBufferInLiDAR = 0;
	}
}