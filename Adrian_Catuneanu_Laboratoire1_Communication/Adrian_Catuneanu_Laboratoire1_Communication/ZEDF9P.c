/*
* ZED_F9P.c
*
* Created: 2023-03-10 10:35:41 AM
*  Author: adino
*/

#include "ZEDF9P.h"

#define TRAME_UBX_NAV_PVT_SIZE 100
#define TRAME_UBX_NAV_ODO_SIZE 28
#define TRAME_UBX_NAV_POSLLH_SIZE 36
#define PVT 0x07
#define ODO 0x09
#define POSLLH 0x02

enum etatsReception{DEBUT_TRAME,DEBUT_TRAME_DEUX,CLASSE,ID,LENGTH_A,LENGTH_B,PAYLOAD,CHECKSUM_VERIF};

enum etatsReception state = DEBUT_TRAME;

enum etatsMessage{NAV_PVT,NAV_ODO,NAV_POLLSH,NONE};

enum etatsMessage id = NONE;


uint8_t trameGpsRx[TRAME_UBX_NAV_PVT_SIZE];
uint8_t trameGpsNavPvtValide[TRAME_UBX_NAV_PVT_SIZE];
uint8_t trameGpsNavOdoValide[TRAME_UBX_NAV_ODO_SIZE];
uint8_t trameGpsNavPosllhValide[TRAME_UBX_NAV_POSLLH_SIZE];

uint8_t indexTrame = 0;
uint8_t indexPayload = 0;
uint8_t checksumA = 0;
uint8_t checksumB = 0;
uint8_t retour = 0;
uint8_t length = 0;
char test[1]={'1'};
long longitude = 0;
long latitude = 0;

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
			retour = 0;
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
		else
		{
			state = DEBUT_TRAME;
		}
		break;
		
		case ID:
		if(u8Data == PVT)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = NAV_PVT;
			state = LENGTH_A;
		}
		else if(u8Data == ODO)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = NAV_ODO;
			state = LENGTH_A;
		}
		else if(u8Data == POSLLH)
		{
			trameGpsRx[indexTrame++] = u8Data;
			id = NAV_POLLSH;
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
			retour = 1;
			switch(id)
			{
				case NAV_PVT:
				for(uint8_t x = 0; x<TRAME_UBX_NAV_PVT_SIZE; x++)
				{
					trameGpsNavPvtValide[x] = trameGpsRx[x];
				}
				break;
				case NAV_POLLSH:
				for(uint8_t x = 0; x<TRAME_UBX_NAV_POSLLH_SIZE; x++)
				{
					trameGpsNavPosllhValide[x] = trameGpsRx[x];
				}
				break;
				case NAV_ODO:
				for(uint8_t x = 0; x<TRAME_UBX_NAV_ODO_SIZE; x++)
				{
					trameGpsNavOdoValide[x] = trameGpsRx[x];
				}
				break;
			}
		}
		else
		{
			retour = 0;
		}
		memset(trameGpsRx, 0, sizeof(trameGpsRx));
		state = DEBUT_TRAME;
		break;
	}
	return retour;
}
float getNavPvtLat()
{
	uint8_t tabLat[4];
	
	tabLat[0] = trameGpsNavPvtValide[34];
	tabLat[1] = trameGpsNavPvtValide[35];
	tabLat[2] = trameGpsNavPvtValide[36];
	tabLat[3] = trameGpsNavPvtValide[37];
	
	latitude = 0;
	memcpy(&latitude, tabLat, sizeof(latitude));
	return (float)latitude;
}
float getNavPvtLon()
{
	uint8_t tabLon[4];
	
	tabLon[0] = trameGpsNavPvtValide[30];
	tabLon[1] = trameGpsNavPvtValide[31];
	tabLon[2] = trameGpsNavPvtValide[32];
	tabLon[3] = trameGpsNavPvtValide[33];
	
	longitude = 0;
	memcpy(&longitude, tabLon, sizeof(longitude));
	return (float)longitude;
}


