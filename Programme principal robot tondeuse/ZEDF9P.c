/**
* @file ZEDF9P.c
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Cette bibliothèque possède la définition de fonctions qui servent à traiter les donnés GPS du module ZEDF9P recues par le UART au atmega2560. De plus, cette bibliothèque possède plusieurs getters pour convertir et réecuperer plusieurs données comme le type de fix du module, la longitude, la latitude, la précision en 2D, etc. On fonctionne avec la datasheet de U-BLOX qui explique le protocol UBX. Finalement, on utilise majoritairement la trame UBX-NAV-PVT pour récupere ces données et on a aussi programmé des fonctions pour la configuration du module par UART à partir du atmega2560.
* @version 1.1
* @date 2023-05-10
*
* @copyright Copyright (c) 2023
*/

#include "ZEDF9P.h"
#include "USART1.h"

#define TRAME_UBX_NAV_PVT_SIZE 100
#define TRAME_UBX_NAV_ODO_SIZE 28
#define TRAME_UBX_NAV_POSLLH_SIZE 36
#define PVTRX 0x07
#define ODORX 0x09
#define POSLLHRX 0x02
#define ACKRX 0x01

enum etatsReception{DEBUT_TRAME,DEBUT_TRAME_DEUX,CLASSE,ID,LENGTH_A,LENGTH_B,PAYLOAD,CHECKSUM_VERIF};

enum etatsReception state = DEBUT_TRAME;

enum etatsMessage{NAV_PVT,NAV_POLLSH,ACK_STATUS,NONE};

enum etatsMessage id = NONE;

uint8_t ackConfigUart1 = 0;
uint8_t ackConfigRate = 0;
uint8_t ackConfigMsg = 0;
uint8_t ackConfigOdo = 0;
uint8_t trame;
uint8_t trameGpsRx[TRAME_UBX_NAV_PVT_SIZE];
uint8_t trameGpsNavPvtValide[TRAME_UBX_NAV_PVT_SIZE];
uint8_t trameGpsNavOdoValide[TRAME_UBX_NAV_ODO_SIZE];
uint8_t trameGpsNavPosllhValide[TRAME_UBX_NAV_POSLLH_SIZE];

uint8_t indexTrame = 0;
uint8_t indexPayload = 0;
uint8_t checksumA = 0;
uint8_t checksumB = 0;
uint8_t retour2 = 0;
uint8_t length = 0;
uint8_t ack = 0;

uint8_t navPvtFlags = 0;
uint8_t navPvtFlags2 = 0;
uint8_t navPvtValide = 0;
// unsigned long distance = 0;
// unsigned long distanceTotale = 0;

/**
* @brief Cette fonction traite la trame UBX-NAV-PVT recue octet par octet. De plus, cette fonction vérifie la validité de la trame GPS recue en calculant le checksum et en comparant la valeur obtenue avec celle recue. Elle traite et valide aussi la trame POSLLH pour effectuer des tests de performance pour la communication entre le robot et le module GPS.
*
* @param u8Data de type uint8_t, récupere l'octet recu par le lien uart avec le module GPS, le traite et valide la trame recue.
*
* @return de type uint8_t retourne 1 si la trame recue est valide et retourne 0 si la trame recue n'est pas valide.
*/

uint8_t parseRxUbxNavPvt(uint8_t u8Data)
{
	
	switch(state)
	{
		case DEBUT_TRAME:
		if(u8Data == 0xB5)
		{
			checksumA = 0;
			checksumB = 0;
			indexTrame = 0;
			indexPayload = 0;
			retour2 = 0;
			trameGpsRx[indexTrame++] = u8Data;
			state = DEBUT_TRAME_DEUX;
		}
		break;
		
		case DEBUT_TRAME_DEUX:
		if(u8Data == 0x62)
		{
			trameGpsRx[indexTrame++] = u8Data;
			state = CLASSE;
		}
		else
		{
			state = DEBUT_TRAME;
		}
		break;
		
		case CLASSE:
		if(u8Data == 0x01)
		{
			trameGpsRx[indexTrame++] = u8Data;
			state = ID;
		}
		else if(u8Data == 0x05)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = ACK_STATUS;
			state = ID;
		}
		else
		{
			state = DEBUT_TRAME;
		}
		break;
		
		case ID:
		if(u8Data == PVTRX)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = NAV_PVT;
			state = LENGTH_A;
		}
		// 		else if(u8Data == ODORX)
		// 		{
		// 			trameGpsRx[indexTrame++] = u8Data;
		// 			id = NAV_ODO;
		// 			state = LENGTH_A;
		// 		}
		else if(u8Data == POSLLHRX)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = NAV_POLLSH;
			state = LENGTH_A;
		}
		else if(u8Data == ACKRX)
		{
			trameGpsRx[indexTrame++] = u8Data;
			state = LENGTH_A;
		}
		else
		{
			id = NONE;
			state = DEBUT_TRAME;
		}
		break;
		
		case LENGTH_A:
		trameGpsRx[indexTrame++] = u8Data;
		length = u8Data + 6;
		state = LENGTH_B;
		break;
		
		case LENGTH_B:
		if(u8Data == 0)
		{
			trameGpsRx[indexTrame++] = u8Data;
			state = PAYLOAD;
		}
		else
		{
			state = DEBUT_TRAME;
		}
		break;
		
		case PAYLOAD:
		if(indexPayload < TRAME_UBX_NAV_PVT_SIZE)
		{
			trameGpsRx[indexTrame++] = u8Data;
		}
		else
		state = CHECKSUM_VERIF;
		
		indexPayload++;
		break;
		
		case CHECKSUM_VERIF:
		
		for(uint8_t n=2;n<length;n++)
		{
			checksumA = checksumA + trameGpsRx[n];
			checksumB = checksumB + checksumA;
		}		if((checksumA == trameGpsRx[length]) && (checksumB == trameGpsRx[length+1]))
		{
			retour2 = 1;
			switch(id)
			{
				case NAV_PVT:
				trame=7;
				for(uint8_t x = 0; x<TRAME_UBX_NAV_PVT_SIZE; x++)
				{
					trameGpsNavPvtValide[x] = trameGpsRx[x];
				}
				break;
				case NAV_POLLSH:
				trame=2;
				for(uint8_t x = 0; x<TRAME_UBX_NAV_POSLLH_SIZE; x++)
				{
					trameGpsNavPosllhValide[x] = trameGpsRx[x];
				}
				break;
				// 				case NAV_ODO:
				// 				trame=9;
				// 				for(uint8_t x = 0; x<TRAME_UBX_NAV_ODO_SIZE; x++)
				// 				{
				// 					trameGpsNavOdoValide[x] = trameGpsRx[x];
				// 				}
				// 				break;
				
				case ACK_STATUS:
				if((trameGpsRx[6] == 0x06) && (trameGpsRx[7] == 0x00))
				{
					ackConfigUart1 = 1;
				}
				else if((trameGpsRx[6] == 0x06) && (trameGpsRx[7] == 0x08))
				{
					ackConfigRate = 1;
				}
				else if((trameGpsRx[6] == 0x06) && (trameGpsRx[7] == 0x01))
				{
					ackConfigMsg = 1;
				}
				else if((trameGpsRx[6] == 0x06) && (trameGpsRx[7] == 0x1E))
				{
					ackConfigOdo = 1;
				}
				break;
				
				case NONE:
				break;
			}
		}
		else
		{
			retour2 = 0;
		}
		memset(trameGpsRx, 0, sizeof(trameGpsRx));
		state = DEBUT_TRAME;
		break;
	}
	return retour2;
}

/**
* @brief Cette fonction est un getter qui retourne le ID de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le ID de la dernière trame valide UBX-NAV-PVT recue.
*/

uint8_t getId()
{
	return trame;
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne la latitude de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne la latitude de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtLat()
{
	uint8_t tabLat[4];
	
	tabLat[0] = trameGpsNavPvtValide[34];
	tabLat[1] = trameGpsNavPvtValide[35];
	tabLat[2] = trameGpsNavPvtValide[36];
	tabLat[3] = trameGpsNavPvtValide[37];
	
	long latitude = 0;
	memcpy(&latitude, tabLat, sizeof(latitude));
	//return latitude;
	return ((float)latitude)*1e-7; //en deg
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne la longitude de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne la longitude de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtLon()
{
	uint8_t tabLon[4];
	
	tabLon[0] = trameGpsNavPvtValide[30];
	tabLon[1] = trameGpsNavPvtValide[31];
	tabLon[2] = trameGpsNavPvtValide[32];
	tabLon[3] = trameGpsNavPvtValide[33];
	
	long longitude = 0;
	memcpy(&longitude, tabLon, sizeof(longitude));
	//return longitude;
	return ((float)longitude)*1e-7; //en deg
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le heading of motion (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le heading of motion (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtHeadMot() //heading motion 2-D
{
	uint8_t tabHeadMot[4];
	
	tabHeadMot[0] = trameGpsNavPvtValide[70];
	tabHeadMot[1] = trameGpsNavPvtValide[71];
	tabHeadMot[2] = trameGpsNavPvtValide[72];
	tabHeadMot[3] = trameGpsNavPvtValide[73];
	
	long headMot = 0;
	memcpy(&headMot, tabHeadMot, sizeof(headMot));
	return ((float)headMot)*1e-5; //en deg
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le ground speed (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le ground speed (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

long getNavPvtGspeed() //ground speed 2-D
{
	uint8_t tabGspeed[4];
	
	tabGspeed[0] = trameGpsNavPvtValide[66];
	tabGspeed[1] = trameGpsNavPvtValide[67];
	tabGspeed[2] = trameGpsNavPvtValide[68];
	tabGspeed[3] = trameGpsNavPvtValide[69];
	
	long gSpeed = 0;
	memcpy(&gSpeed, tabGspeed, sizeof(gSpeed));
	return gSpeed; //en mm/s
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le speed accuracy estimate (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le speed accuracy estimate (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

unsigned long getNavPvtSacc() //speed accuracy estimate
{
	uint8_t tabSacc[4];
	
	tabSacc[0] = trameGpsNavPvtValide[74];
	tabSacc[1] = trameGpsNavPvtValide[75];
	tabSacc[2] = trameGpsNavPvtValide[76];
	tabSacc[3] = trameGpsNavPvtValide[77];
	
	unsigned long sAcc = 0;
	memcpy(&sAcc, tabSacc, sizeof(sAcc));
	return sAcc; //en mm/s
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le heading accuracy estimate (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le heading accuracy estimate (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtHeadAcc() // heading accuracy estimate
{
	uint8_t tabHeadAcc[4];
	
	tabHeadAcc[0] = trameGpsNavPvtValide[78];
	tabHeadAcc[1] = trameGpsNavPvtValide[79];
	tabHeadAcc[2] = trameGpsNavPvtValide[80];
	tabHeadAcc[3] = trameGpsNavPvtValide[81];
	
	unsigned long headAcc = 0;
	memcpy(&headAcc, tabHeadAcc, sizeof(headAcc));
	return ((float)headAcc)*1e-5; //en deg
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le heading of vehicle (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le heading of vehicle (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtHeadVeh() // heading of vehicle 2-D
{
	uint8_t tabHeadVeh[4];
	
	tabHeadVeh[0] = trameGpsNavPvtValide[90];
	tabHeadVeh[1] = trameGpsNavPvtValide[91];
	tabHeadVeh[2] = trameGpsNavPvtValide[92];
	tabHeadVeh[3] = trameGpsNavPvtValide[93];
	
	long headVeh = 0;
	memcpy(&headVeh, tabHeadVeh, sizeof(headVeh));
	return ((float)headVeh)*1e-5; //en deg
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le horizontal accuracy estimate (2D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le horizontal accuracy estimate (2D) de la dernière trame valide UBX-NAV-PVT recue.
*/

unsigned long getNavPvtHacc() //horizontal accuracy estimate
{
	uint8_t tabHacc[4];
	
	tabHacc[0] = trameGpsNavPvtValide[46];
	tabHacc[1] = trameGpsNavPvtValide[47];
	tabHacc[2] = trameGpsNavPvtValide[48];
	tabHacc[3] = trameGpsNavPvtValide[49];
	
	unsigned long hAcc = 0;
	memcpy(&hAcc, tabHacc, sizeof(hAcc));
	return hAcc; //en mm
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le vertical accuracy estimate (3D) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le vertical accuracy estimate (3D) de la dernière trame valide UBX-NAV-PVT recue.
*/

unsigned long getNavPvtVacc() //vertical accuracy estimate
{
	uint8_t tabVacc[4];
	
	tabVacc[0] = trameGpsNavPvtValide[50];
	tabVacc[1] = trameGpsNavPvtValide[51];
	tabVacc[2] = trameGpsNavPvtValide[52];
	tabVacc[3] = trameGpsNavPvtValide[53];
	
	unsigned long vAcc = 0;
	memcpy(&vAcc, tabVacc, sizeof(vAcc));
	return vAcc; //en mm
}

/**
* @brief Cette fonction est un getter qui retourne le fix status flags de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le fix status flags de la dernière trame valide UBX-NAV-PVT recue.
*/

uint8_t getNavPvtFlags() //fix status flags
{
	return trameGpsNavPvtValide[27];
}

/**
* @brief Cette fonction est un getter qui retourne le deuxième fix status flags de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le deuxième fix status flags de la dernière trame valide UBX-NAV-PVT recue.
*/

uint8_t getNavPvtFlags2() //additional flags
{
	return trameGpsNavPvtValide[28];
}

/**
* @brief Cette fonction est un getter qui retourne les validity flags de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne les validity flags de la dernière trame valide UBX-NAV-PVT recue.
*/

uint8_t getNavPvtValid() //validity flags
{
	return trameGpsNavPvtValide[17];
}

/**
* @brief Cette fonction est un getter qui retourne le type de fix de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le type de fix de la dernière trame valide UBX-NAV-PVT recue.
*/

unsigned char getNavPvtFixType()
{
	unsigned char etatFixGps = 0;
	
	etatFixGps = trameGpsNavPvtValide[26];
	
	return etatFixGps;
}

/**
* @brief Cette fonction est un getter qui convertit au bon format et retourne le PDOP (chiffre inférieur si le nombre de sattelites percus augmente) de la trame UBX-NAV-PVT recue.
*
* @return de type uint8_t, retourne le PDOP (chiffre inférieur si le nombre de sattelites percus augmente) de la dernière trame valide UBX-NAV-PVT recue.
*/

float getNavPvtPdop()
{
	uint16_t pDOP = 0;
	uint8_t tabPdop[2];
	
	tabPdop[0] = trameGpsNavPvtValide[82];
	tabPdop[1] = trameGpsNavPvtValide[83];
	
	memcpy(&pDOP, tabPdop, sizeof(pDOP));
	//return longitude;
	return ((float)pDOP)/100.0; //en deg
}

//Partie de code pour la recuperation de la distance parcourue de l'odomètre du module GPS

// unsigned long getNavOdoDistance()
// {
// 	uint8_t tabDist[4];
//
// 	tabDist[0] = trameGpsNavOdoValide[14];
// 	tabDist[1] = trameGpsNavOdoValide[15];
// 	tabDist[2] = trameGpsNavOdoValide[16];
// 	tabDist[3] = trameGpsNavOdoValide[17];
//
// 	distance = 0;
// 	memcpy(&distance, tabDist, sizeof(distance));
// 	return distance;
//
// }
// unsigned long getNavOdoTotalDistance()
// {
// 	uint8_t tabTotalDist[4];
//
// 	tabTotalDist[0] = trameGpsNavOdoValide[18];
// 	tabTotalDist[1] = trameGpsNavOdoValide[19];
// 	tabTotalDist[2] = trameGpsNavOdoValide[20];
// 	tabTotalDist[3] = trameGpsNavOdoValide[21];
//
// 	distanceTotale = 0;
// 	memcpy(&distanceTotale, tabTotalDist, sizeof(distanceTotale));
// 	return distanceTotale;
// }

/**
* @brief Cette fonction calcule le checksum de la trame de configuration à envoyer vers le module GPS. Ce calcul est spécifié dans la datasheet du protocol UBX.
*
* @param trame est un pointeur qui sert à parcourir un tableau de type uint8_t. Le tableau correspond à la trame que l'utilisateur veut envoyer vers le module simpleRTK2b.
*
* @param size de type uint8_t est la taille de la trame que l'utilisateur veut envoyer
*
* @return de type uint16_t retourne le résultat du checksum
*/

uint16_t calculerChecksum(uint8_t*trame,uint8_t size)
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

/**
* @brief Cette fonction sert à configurer le baudrate du port UART1 sur le module GPS en créant une trame et en l'envoyant vers le module GPS.
*
* @param baudrate de type uint8_t correspond au baudrate que l'utilisateur veut configurer la communication du UART1 sur le module GPS.
*/

void envoieConfigPortUart1(uint32_t baudrate)
{
	//{0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, 0x80, 0x25, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9A, 0x79}
	uint16_t crc=0;
	uint8_t trameConfigEnvoi[28]={0xB5, 0x62, 0x06, 0x00, 0x14, 0x00, 0x01, 0x00, 0x00, 0x00, 0xD0, 0x08, 0x00, 0x00, (baudrate&0xFF),(baudrate>>8&0xFF),(baudrate>>16&0xFF), (baudrate>>32&0xFF), 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0, 0};
	crc=calculerChecksum(trameConfigEnvoi,20);
	trameConfigEnvoi[26]=crc&0xFF;
	trameConfigEnvoi[27]=crc>>8;
	usartGpsSendBytes(trameConfigEnvoi,28);
	ackConfigUart1 = 0;
}

/**
* @brief Cette fonction sert à configurer la fréquence d'envoi du module GPS à partir du atmega2560 en créant une trame de configuration et en l'envoyant par UART vers le module GPS.
*
* @param frequence de type uint8_t correspond à la fréquence d'envoi de données du module GPS dont l'utilisateur désire appliquer.
*/

void envoieConfigRate(uint8_t frequence)
{
	//{0xB5,0x62,0x06,0x08,0x06,0x00,0xE8,0x03,0x01,0x00,0x01,0x00,0x01,0x39}
	
	uint16_t crc=0;
	uint16_t freq=1000/frequence;
	uint8_t trameConfigEnvoi[14]={0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, (freq&0xFF),(freq>>8), 0x01,0x00 , 0x01, 0x00, 0, 0};
	crc=calculerChecksum(trameConfigEnvoi,6);
	trameConfigEnvoi[12]=crc&0xFF;
	trameConfigEnvoi[13]=crc>>8;
	
	usartGpsSendBytes(trameConfigEnvoi,14);
	ackConfigRate = 0;
}

/**
* @brief Cette fonction sert a créer une trame de configuration pour la communication USB et UART du module GPS. L'utilisateur peut avec cette fonction désactiver ou activer les différentes méthodes de communication du module GPS. Finalement, avec le id il peut activer la réception des différentes trames UBX sur le lien de communication désiré.
*
* @param id de type uint8_t recoit le type de trame UBX à activer sur le module GPS
*
* @param etatUart1 de type uint8_t recoit 1 lorsque l'utilisateur veut activer la réception de ce type de message par le uart1 du module GPS.
*
* @param etatUsb de type uint8_t recoit 1 lorsque l'utilisateur veut activer la réception de ce type de message par le USB du module GPS.
*/

void envoieConfigMsg(uint8_t id,uint8_t etatUart1,uint8_t etatUsb)
{
	uint16_t crc=0;
	uint8_t trameConfigEnvoi[16]={0xB5, 0x62, 0x06, 0x01, 0x08, 0x00, 0x01, id, 0x00, etatUart1, 0x00, etatUsb, 0x00, 0x00, 0, 0};
	
	crc=calculerChecksum(trameConfigEnvoi,8);
	trameConfigEnvoi[14]=crc&0xFF;
	trameConfigEnvoi[15]=crc>>8;
	usartGpsSendBytes(trameConfigEnvoi,16);
	
	ackConfigMsg = 0;
}

//Décommenter si on utilise la configuration de l'odomètre

// void envoieConfigOdo(uint8_t etatOdo,uint8_t etatLowSpeed,uint8_t etatVelocity,uint8_t etatHeading,uint8_t speedThreshold,uint8_t maxAccuracy,uint8_t velocityLevel,uint8_t headingLevel )
// {
// 	uint16_t crc=0;
// 	uint8_t odoFlags=etatOdo|(etatLowSpeed<<1)|(etatVelocity<<2)|(etatHeading<<3);
// 	uint8_t trameConfigEnvoi[28]={0xB5, 0x62, 0x06, 0x1E, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, odoFlags, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, speedThreshold, maxAccuracy, 0x00, 0x00, velocityLevel, headingLevel, 0x00, 0x00, 0, 0};
// 	crc = calculerChecksum(trameConfigEnvoi,20);
// 	trameConfigEnvoi[26]= crc&0xFF;
// 	trameConfigEnvoi[27]= crc>>8;
// 	usartGpsSendBytes(trameConfigEnvoi,28);
// }

/**
* @brief Cette fonction est un getter qui récupere l'état du Ack pour la configuration du port UART1 du module GPS.
*
* @return de type uint8_t retourne 1 si le module GPS a bien recue la commande.
*/

uint8_t getAckConfigUart1()
{
	return ackConfigUart1;
}

/**
* @brief Cette fonction est un getter qui récupere l'état du Ack pour la configuration de la fréquence d'envoi des données du module GPS.
*
* @return de type uint8_t retourne 1 si le module GPS a bien recue la commande.
*/

uint8_t getAckConfigRate()
{
	return ackConfigRate;
}

/**
* @brief Cette fonction est un getter qui récupere l'état du Ack pour la configuration des messages UBX.
*
* @return de type uint8_t retourne 1 si le module GPS a bien recue la commande.
*/

uint8_t getAckConfigMsg()
{
	return ackConfigMsg;
}

/**
* @brief Cette fonction est un getter qui récupere l'état du Ack pour la configuration de l'odomètre.
*
* @return de type uint8_t retourne 1 si le module GPS a bien recue la commande.
*/

uint8_t getAckConfigOdo()
{
	return ackConfigOdo;
}

/**
* @brief Cette fonction est un getter qui recupère l'heure du module GPS et la met sous le bon format (HH:MM) pour l'affichage et l'heure de coupe du robot tondeuse.
*
* @param msg est un pointeur de type char qui recoit le tableau de caractères que l'utilisateur désire affecter avec l'heure actuelle.
*/

void getNavPvtTime(char* msg)
{
	uint8_t heure = trameGpsNavPvtValide[14];
	uint8_t minutes = trameGpsNavPvtValide[15];
	
	if(getNavPvtFixType())
	{
		heure = heure - 4;
	}
	if((minutes<10) && (heure<10))
	{
		sprintf(msg,"0%d:0%d",heure,minutes/*,trameGpsNavPvtValide[16]*/);
	}
	else if(minutes<10)
	{
		sprintf(msg,"%d:0%d",heure,minutes/*,trameGpsNavPvtValide[16]*/);
	}
	else if (heure<10)
	{
		sprintf(msg,"0%d:%d",heure,minutes/*,trameGpsNavPvtValide[16]*/);
	}
	else
	{
		sprintf(msg,"%d:%d",heure,minutes/*,trameGpsNavPvtValide[16]*/);
	}
}
