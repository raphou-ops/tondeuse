/**
* @file USART2.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.1
* @date 12 avril 2023
* @brief Cette bibliothèque offre les définitions des fonctions pour la communication USART à l'aide du atmega2560 avec le module HC-06 Bluetooth. On utilise les fonctions d'envoi pour transmettre les états du robot vers le serveur nodeJs et le fonctions de reception pour recevoir les commandes à partir du site web.
*/

#include "USART2.h"

#define TX_BUFFER_SIZE_BLUETOOTH 256
#define RX_BUFFER_SIZE_BLUETOOTH 256
#define TRAME_BLUETOOTH_SIZE 64

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

/*Variables globales pour les fonctions parse*/

enum etatsBtRx{DEBUT_TRAME,PAYLOAD,VALIDATE};
enum etatsBtRx etats = DEBUT_TRAME;
enum etatsBtRx etatProg = DEBUT_TRAME;
uint8_t trameBluetoothRx[TRAME_BLUETOOTH_SIZE];
uint8_t trameBluetoothRxManette[TRAME_BLUETOOTH_SIZE];
uint8_t trameBluetoothValide[TRAME_BLUETOOTH_SIZE];
uint8_t trameBluetoothValideManette[TRAME_BLUETOOTH_SIZE];
uint8_t indexBluetooth = 0;
uint8_t indexBluetoothProg = 0;
char tabX[5] = "";
char tabY[5] = "";
int joystickReceptionX = 0;
int joystickReceptionY = 0;
char tabLat4[9];
char tabLon4[10];
uint8_t boutonX = 0;
uint8_t boutonO = 0;
uint8_t boutonTriangle = 0;
uint8_t status = 0;
uint8_t z = 0;
long latitudeRx = 0;
long longitudeRx = 0;

/**
* @brief Cette fonction gere la transmission de données un octet à la fois.
*
* @param uint8_t u8data Recoit l'octet à transmettre par le lien UART.
*
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
* @brief Cette fonction sert a initialiser la tranmission de données en mode 8N1 à une vitesse de 115200 bauds.
*
* @param uint32_t baudRate Recoit la valeur de vitesse de communication desirer par l'utilisateur.
*
* @param uint32_t fcpu Recoit la vitesse du cpu de l'utilisateur
*/

void usart2Init(uint32_t baudRate, uint32_t fcpu)
{
	
	UCSR2B = (1<<RXEN2)|(1<<TXEN2);//active la transmission et la reception
	//UCSR1C = 0b00100110;//met en mode 8 bit parite paire
	UCSR2C = 0b00000110;
	
	UBRR2 = fcpu / (16.0*baudRate) - 0.5;//calcul de UBBR2 pour avoir une vitesse de 115200 bauds en fonction de la vitesse de clock de 16MHz
	UCSR2B |= (1<<RXCIE2);//active l'interruption pour la reception
	sei();
}

/**
* @brief Cette fonction retourne le nombre d'octets restants pour la reception (place dans le rx Buffer).
*
* @param aucun
*
* @return le type de retour est uint8_t qui s'agit du nombre d'octets qui occupent le Buffer de réception.
*/

uint8_t usart2RxAvailable()
{
	return rxCntBluetooth;
}

/**
* @brief Cette fonction a pour but de recuperer la valeur (octet) transmise par le module HC-06 et ensuite la mettre dans une variable.
*
* @param aucun
*
* @return Le type de retour est uint8_t, le retour signifie l'octet transmis par le module HC-06.
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
* @brief Cette fonction sert recuperer et envoyer une string a l'aide de la fonction usartSendByte qui transmet octet par octet. Cette fonction envoye la string a transmettre vers le module HC-06.
*
* @param const char*str Recoit la string desirer par l'utilisateur a transmettre vers le module HC-06.
*
* @return cette fonction retourne sous type uint8_t la longueur de la string qui vient d'être envoyer.
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
* @brief Cette fonction gere l'envoi de plusieurs octets uint8_t vers le module HC-06.
*
* @param const uint8_t * source Est un pointeur qui sert a pointer le tableau d'octets à envoyer.
*
* @param uint8_t size Recoit la longueur du tableau d'octets à envoyer vers le module HC-06.
*
* @return nombre d'octets ajoutés à la transmission
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

ISR(USART2_UDRE_vect)//à chaque fois que l'interruption est appeller le programme tansmet des donnes avec le registre UDR2 si il y a bien une donne a transmettre.
{
	if(!txCntBluetooth)
	{
		UCSR2B &= ~(1<<UDRIE2);//desactive l'interruption si il n'y a pus de contenu dans txCnt
	}
	else
	{
		UDR2 = txBufferBluetooth[txBufferOutBluetooth++];//sert a transmettre les données du txBuffer vers le module HC-06
		txCntBluetooth--;
		if(txBufferOutBluetooth >= TX_BUFFER_SIZE_BLUETOOTH)
		txBufferOutBluetooth = 0;
	}
}
volatile uint8_t _usartRxTmpBluetooth;
ISR(USART2_RX_vect)//cette interruption est semblable à la dernière, mais celle-ci gere la reception des données du atmega2560 en mettant les valeurs du rx buffer dans UDR2.
{
	_usartRxTmpBluetooth = UDR2;
	if(rxCntBluetooth<RX_BUFFER_SIZE_BLUETOOTH)
	{
		rxBufferBluetooth[rxBufferInBluetooth++] = _usartRxTmpBluetooth;//recoit les valeurs en reception et les met dans le regisre UDR2
		rxCntBluetooth++;
		if(rxBufferInBluetooth >= RX_BUFFER_SIZE_BLUETOOTH)//Si le buffer de reception est plein l'état de reception du buffer se met a 0
		rxBufferInBluetooth = 0;
	}
}

/**
* @brief Cette fonction sert à traiter la trame de réception du HC-06 Bluetooth pour le contrôle du robot en mode manuel à l'aide de la manette de ps4.
*
* @param data de type uint8_t recoit l'octet recu par le atmega2560 du module HC-06.
*
* @return de type uint8_t retourne 1 si la trame recue est vaide et retourne 0 si la trame recue est invalide.
*/

uint8_t parseBluetoothManuel(uint8_t data)
{
	uint8_t valide = 0;
	switch(etats)
	{
		case DEBUT_TRAME:
		if(data == '<')
		{
			indexBluetooth = 0;
			memset(trameBluetoothValideManette, 0, sizeof(trameBluetoothValideManette));
			memset(trameBluetoothRxManette, 0, sizeof(trameBluetoothRxManette));
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
			trameBluetoothRxManette[indexBluetooth++] = data;
		}
		break;
		
		case VALIDATE:
		status = 0;
		z = 0;
		memset(tabX, 0, sizeof(tabX));
		memset(tabY, 0, sizeof(tabY));
		for(uint8_t i = 0; i<indexBluetooth; i++)
		{
			trameBluetoothValideManette[i] = trameBluetoothRxManette[i];
			
			if(status == 0)
			tabX[i] = trameBluetoothValideManette[i];
			
			if(status == 1)
			tabY[z++] = trameBluetoothValideManette[i];
			
			if(trameBluetoothValideManette[i] == ';')
			status++;
			
			if(status == 2)
			boutonX = trameBluetoothValideManette[i];
			
			if(status == 3)
			boutonO = trameBluetoothValideManette[i];
			
			if(status == 4)
			{
				boutonTriangle = trameBluetoothValideManette[i];
			}
			
		}
		joystickReceptionX = atoi(tabX);
		joystickReceptionY = atoi(tabY);
		//joystickReceptionY^=1;
		if((joystickReceptionX!=0)&&(joystickReceptionY!=0))
		{
			valide = 1;
		}
		etats = DEBUT_TRAME;
		break;
	}
	return valide;
}

/**
* @brief Cette fonction sert à traiter la trame de réception du HC-06 Bluetooth pour la réception de points GPS dans le mode prog du robot.
*
* @param data de type uint8_t recoit l'octet recu par le atmega2560 du module HC-06.
*
* @return de type uint8_t retourne 1 si la trame recue est vaide et retourne 0 si la trame recue est invalide.
*/

uint8_t parseBluetoothAuto(uint8_t data)
{
	uint8_t valide = 0;
	switch(etatProg)
	{
		case DEBUT_TRAME:
		if(data == '<')
		{
			indexBluetoothProg = 0;
			etatProg = PAYLOAD;
		}
		break;
		
		case PAYLOAD:
		if(data == '>')
		{
			etatProg = VALIDATE;
		}
		else
		{
			trameBluetoothRx[indexBluetoothProg++] = data;
		}
		break;
		
		case VALIDATE:
		
		for(uint8_t i = 0; i<indexBluetoothProg; i++)
		{
			trameBluetoothValide[i] = trameBluetoothRx[i];
		}
		tabLat4[0]=trameBluetoothValide[2];
		tabLat4[1]=trameBluetoothValide[3];
		tabLat4[2]=trameBluetoothValide[4];
		tabLat4[3]=trameBluetoothValide[5];
		tabLat4[4]=trameBluetoothValide[6];
		tabLat4[5]=trameBluetoothValide[7];
		tabLat4[6]=trameBluetoothValide[8];
		tabLat4[7]=trameBluetoothValide[9];
		//tabLat4[8]=trameBluetoothValide[10];
		tabLat4[8]='\0';
		
		latitudeRx = atol(tabLat4);
		
		tabLon4[0]=trameBluetoothValide[11];
		tabLon4[1]=trameBluetoothValide[12];
		tabLon4[2]=trameBluetoothValide[13];
		tabLon4[3]=trameBluetoothValide[14];
		tabLon4[4]=trameBluetoothValide[15];
		tabLon4[5]=trameBluetoothValide[16];
		tabLon4[6]=trameBluetoothValide[17];
		tabLon4[7]=trameBluetoothValide[18];
		tabLon4[8]=trameBluetoothValide[19];
		//tabLon4[9]=trameBluetoothValide[21];
		tabLon4[9]='\0';
		
		longitudeRx = atol(tabLon4);
		
		if ((longitudeRx!=0)&&(latitudeRx!=0))
		{
			valide = 1;
		}
		etatProg = DEBUT_TRAME;
		break;
	}
	return valide;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne la valeur de l'axe X du joystick gauche de la manette de ps4.
*
* @return de type int retourne la valeur de ce joystick, cette valeur varie entre -512 et 512 en fonction de la position du joystick.
*/

int getJoystickGaucheX()
{
	return joystickReceptionX;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne la valeur de l'axe Y du joystick gauche de la manette de ps4.
*
* @return de type int retourne la valeur de ce joystick, cette valeur varie entre -512 et 512 en fonction de la position du joystick.
*/

int getJoystickGaucheY()
{
	return joystickReceptionY;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne l'état du bouton X de la manette de ps4.
*
* @return de type uint8_t retourne l'etat du bouton, 1 si le bouton est appuyé et 0 si le bouton n'est pas appuyé.
*/

uint8_t getBoutonX()
{
	return boutonX;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne l'état du bouton O de la manette de ps4.
*
* @return de type uint8_t retourne l'etat du bouton, 1 si le bouton est appuyé et 0 si le bouton n'est pas appuyé.
*/

uint8_t getBoutonO()
{
	return boutonO;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne la valeur de longitude recue en mode prog.
*
* @return de type long retourne la valeur de longitude sous format -73123456.
*/

long getLon()
{
	return longitudeRx;
}

/**
* @brief Cette fonction s'agit d'un getter qui retourne la valeur de latitude recue en mode prog.
*
* @return de type long retourne la valeur de longitude sous format 45123456.
*/

long getLat()
{
	return latitudeRx;
}