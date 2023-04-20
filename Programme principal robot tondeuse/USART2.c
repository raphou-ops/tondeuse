/**
* @file USART.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 13 septembre 2021
* @brief Cette bibliothèque offre les definitions des fonctions pour la communication USART a l'aide du atmega32u4 avec le fil usb USART.
*/

#include "USART2.h"

#define TX_BUFFER_SIZE_BLUETOOTH 64
#define RX_BUFFER_SIZE_BLUETOOTH 64

volatile uint8_t txBufferBluetooth[TX_BUFFER_SIZE_BLUETOOTH];
volatile uint8_t rxBufferBluetooth[RX_BUFFER_SIZE_BLUETOOTH];
volatile uint8_t txBufferInBluetooth = 0;
volatile uint8_t rxBufferInBluetooth = 0;
uint8_t sendMessageBluetooth = 1;
uint8_t txBufferOutBluetooth = 0;
uint8_t rxBufferOutBluetooth = 0;
uint8_t rxCntBluetooth = 0;
uint8_t txCntBluetooth = 0;
uint8_t rxContentBluetooth = 0;
uint8_t nbOctetBluetooth = 0;
uint8_t cntStrBluetooth = 0;

char msgBluetooth[50];
enum etatsBtRx{DEBUT_TRAME,PAYLOAD,VALIDATE};
enum etatsBtRx etats = DEBUT_TRAME;
#define TRAME_BLUETOOTH_SIZE 20
uint8_t trameBluetoothRx[TRAME_BLUETOOTH_SIZE];
uint8_t trameBluetoothValide[TRAME_BLUETOOTH_SIZE];
uint8_t indexBluetooth = 0;
char tabX[5] = "";
char tabY[5] = "";
uint8_t boutonX = 0;
uint8_t boutonO = 0;
uint8_t boutonTriangle = 0;
uint8_t status = 0;
uint8_t z = 0;

/**
* @brief Cette fonction gere la transmission de donnes un bit a la fois.
* @param uint8_t u8data Recoit la commande qu'on veut faire executer au LCD.
* @return le retour est de type uint8_t et cette fonction retourne sois 0 si le buffer de transmission est plein et retourne 1 s'il est vide.
*/

uint8_t usart2SendByte(uint8_t u8Data)
{
	if(txCntBluetooth<TX_BUFFER_SIZE_BLUETOOTH)
	{
		txBufferBluetooth[txBufferInBluetooth++] = u8Data;
		cli();
		txCntBluetooth++;
		sei();
		if(txBufferInBluetooth>=TX_BUFFER_SIZE_BLUETOOTH)
		txBufferInBluetooth = 0;
		UCSR2B |= (1<<UDRIE2);
		return 0;
	}
	return 1;
}

/**
* @brief Cette fonction sert a initialiser la tranmission de donne en mode 8n1 a une vitess de 10000 bauds.
* @param uint32_t baudRate Recoit la valeur de vitesse de communication desirer par l'utilisateur.
* @param uint32_t fcpu Recoit la vitesse du cpu de l'utilisateur
*/

void usart2Init(uint32_t baudRate, uint32_t fcpu)
{
	
	UCSR2B = (1<<RXEN2)|(1<<TXEN2);//active la transmission et la reception
	//UCSR1C = 0b00100110;//met en mode 8 bit parite paire
	UCSR2C = 0b00000110;
	
	UBRR2 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR1 pour avoir une vitesse de 10000 bauds
	UCSR2B |= (1<<RXCIE2);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre de bits restant pour la reception (place dans le rx Buffer).
* @param aucun
* @return le type de retour est uin8_t qui s'agit du nombre de bits qui occupent le rx Buffer.
*/

uint8_t usart2RxAvailable()
{
	return rxCntBluetooth;
}

/**
* @brief Cette fonction a pour but de recuper la valeur transmise par l'ordinateur et la recuper pour ensuite la metre dans une variable.
* @param aucun
* @return Le type de retour est uint8_t, le retour signifie la valeur decimale entree par l'utilisateur dans la console de communication.
*/

uint8_t usart2RemRxData()
{
	if(rxCntBluetooth)
	{
		rxContentBluetooth = rxBufferBluetooth[rxBufferOutBluetooth++];
		cli();
		rxCntBluetooth--;
		sei();
		if(rxBufferOutBluetooth>=RX_BUFFER_SIZE_BLUETOOTH)
		rxBufferOutBluetooth = 0;
	}
	return rxContentBluetooth;
}

/**
* @brief Cette fonction sert recuperer une string a l'aide de la fonction usartSendByte qui transmet bit a bit. Cette fonction envoye la string a transmettre vers la console d'affichage.
* @param const char*str Recoit la string desirer par l'utilisateur a faire afficher sur la console PC.
* @return cette fonction retourne sous type uint8_t la longueur de la string qui vient d'etre envoyer.
*/

uint8_t usart2SendString(const char * str)
{
	uint8_t cntStr = 0;
	for(uint8_t i = 0;str[i];i++)
	{
		if(!usart2SendByte(str[i]))
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

uint8_t usart2SendBytes(const uint8_t * source, uint8_t size)
{
	uint8_t nbOctet = 0;
	
	for(uint8_t i = 0;i<size;i++)
	{
		if(!usart2SendByte(source[i]))
		nbOctet++;
	}
	return nbOctet;
}

ISR(USART2_UDRE_vect)//a chaque fois que l'interruption est appeller le programme tansmet des donnes avec le registre UDR1 si il y a bien une donne a transmettre.
{
	if(!txCntBluetooth)
	{
		UCSR2B &= ~(1<<UDRIE2);//desactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR2 = txBufferBluetooth[txBufferOutBluetooth++];//sert a transmettre les donnes du txBuffer vers le PC
		txCntBluetooth--;
		if(txBufferOutBluetooth >= TX_BUFFER_SIZE_BLUETOOTH)
		txBufferOutBluetooth = 0;
	}
}
volatile uint8_t _usartRxTmpBluetooth;
ISR(USART2_RX_vect)//cette interruption est semblable a la derniere mais celle ci gere la reception des donnes du atmega32u4 en mettant les valeurs du rx buffer dans UDR1.
{
	_usartRxTmpBluetooth = UDR2;
	if(rxCntBluetooth<RX_BUFFER_SIZE_BLUETOOTH)
	{
		rxBufferBluetooth[rxBufferInBluetooth++] = _usartRxTmpBluetooth;//recoit les valeurs en reception et les met dans le regisre UDR1
		rxCntBluetooth++;
		if(rxBufferInBluetooth >= RX_BUFFER_SIZE_BLUETOOTH)//Si le buffer de reception est plein le buffer se met a 0
		rxBufferInBluetooth = 0;
	}
}
uint8_t parseBluetooth(uint8_t data)
{
	uint8_t valide = 0;
	switch(etats)
	{
		case DEBUT_TRAME:
		if(data == '<')
		{
			indexBluetooth = 0;
			//trameBluetoothRx[indexBluetooth++] = data;
			etats = PAYLOAD;
		}
		break;
		
		case PAYLOAD:
		if(data == '>')
		{
			etats = VALIDATE;
		}
		else
		{
			trameBluetoothRx[indexBluetooth++] = data;
		}
		break;
		
		case VALIDATE:
		status = 0;
		z = 0;
		memset(tabX, 0, sizeof(tabX));
		memset(tabY, 0, sizeof(tabY));
		for(uint8_t i = 0; i<indexBluetooth; i++)
		{
			trameBluetoothValide[i] = trameBluetoothRx[i];
			
			if(status == 0)
			tabX[i] = trameBluetoothValide[i];
			
			if(status == 1)
				tabY[z++] = trameBluetoothValide[i];
			
			if(trameBluetoothValide[i] == ';')
				status++;
			
			if(status == 2)
				boutonX = trameBluetoothValide[i];
				
			if(status == 3)
				boutonO = trameBluetoothValide[i];
				
			if(status == 4)
			{
				boutonTriangle = trameBluetoothValide[i];
			}
			
		}
		valide = 1;
		etats = DEBUT_TRAME;
		break;
	}
	return valide;
}
int getJoystickGaucheX()
{
	return atoi(tabX);
}
int getJoystickGaucheY()
{
	return atoi(tabY);
}
uint8_t getBoutonX()
{
	return boutonX;
}
uint8_t getBoutonO()
{
	return boutonO;
}