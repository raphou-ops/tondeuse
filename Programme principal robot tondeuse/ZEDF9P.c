/*
* ZED_F9P.c
*
* Created: 2023-03-10 10:35:41 AM
*  Author: adino
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

uint8_t getId()
{
	return trame;
}

float getNavPvtLat()
{
	uint8_t tabLat[4];
	
	tabLat[0] = trameGpsNavPvtValide[34];
	tabLat[1] = trameGpsNavPvtValide[35];
	tabLat[2] = trameGpsNavPvtValide[36];
	tabLat[3] = trameGpsNavPvtValide[37];
	
	long latitude = 0;
	memcpy(&latitude, tabLat, sizeof(latitude));
	return ((float)latitude)*1e-7; //en deg
}

float getNavPvtLon()
{
	uint8_t tabLon[4];
	
	tabLon[0] = trameGpsNavPvtValide[30];
	tabLon[1] = trameGpsNavPvtValide[31];
	tabLon[2] = trameGpsNavPvtValide[32];
	tabLon[3] = trameGpsNavPvtValide[33];
	
	long longitude = 0;
	memcpy(&longitude, tabLon, sizeof(longitude));
	return ((float)longitude)*1e-7; //en deg
}

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

uint8_t getNavPvtFlags() //fix status flags
{
	return trameGpsNavPvtValide[27];
}

uint8_t getNavPvtFlags2() //additional flags
{
	return trameGpsNavPvtValide[28];
}

uint8_t getNavPvtValid() //validity flags
{
	return trameGpsNavPvtValide[17];
}

unsigned char getNavPvtFixType()
{
	unsigned char etatFixGps = 0;
	
	etatFixGps = trameGpsNavPvtValide[26];
	
	return etatFixGps;
}

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

void envoieConfigOdo(uint8_t etatOdo,uint8_t etatLowSpeed,uint8_t etatVelocity,uint8_t etatHeading,uint8_t speedThreshold,uint8_t maxAccuracy,uint8_t velocityLevel,uint8_t headingLevel )
{
	uint16_t crc=0;
	uint8_t odoFlags=etatOdo|(etatLowSpeed<<1)|(etatVelocity<<2)|(etatHeading<<3);
	uint8_t trameConfigEnvoi[28]={0xB5, 0x62, 0x06, 0x1E, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, odoFlags, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, speedThreshold, maxAccuracy, 0x00, 0x00, velocityLevel, headingLevel, 0x00, 0x00, 0, 0};
	crc = calculerChecksum(trameConfigEnvoi,20);
	trameConfigEnvoi[26]= crc&0xFF;
	trameConfigEnvoi[27]= crc>>8;
	usartGpsSendBytes(trameConfigEnvoi,28);
}

uint8_t getAckConfigUart1()
{
	return ackConfigUart1;
}

uint8_t getAckConfigRate()
{
	return ackConfigRate;
}

uint8_t getAckConfigMsg()
{
	return ackConfigMsg;
}

uint8_t getAckConfigOdo()
{
	return ackConfigOdo;
}

void getNavPvtTime(char* msg)
{
	sprintf(msg,"%d:%d:%d",trameGpsNavPvtValide[14],trameGpsNavPvtValide[15],trameGpsNavPvtValide[16]);
}
