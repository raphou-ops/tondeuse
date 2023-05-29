/**
* @file LiDAR.c
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Cette bibliothèque possède la définition des fonctions pour le traitement de la trame de reception envoyée par le LiDAR. De plus, elle possède plusieurs getters pour la récuperation de la distance, les amp et la température interne du module.
* @version 1.0
* @date 12 avril 2023
*
* @copyright Copyright (c) 2023
*
*/

#include "LiDAR.h"

#define TRAME_LIDAR_SIZE 9

enum etatsReception{DEBUT_TRAME,DEBUT_TRAME2,DATA,CHECK_SUM};

enum etatsReception state2 = DEBUT_TRAME;

uint8_t trameLidar[TRAME_LIDAR_SIZE];
uint8_t trameLidarValide[TRAME_LIDAR_SIZE];
uint8_t indexTrame2 = 0;
uint8_t indexPayload2 = 0;
uint8_t checksum = 0;
uint8_t retour = 0;

uint16_t dist = 0;
uint16_t amp = 0;
uint16_t temp = 0;

/**
* @brief Cette fonction sert à traiter les donnés envoyées par le Lidar au atmega2560 et vérifier si la trame recue est valide.
*
* @param u8Data de type uint8_t recoit l'octet envoyé par le TF-Luna mini Lidar et le traite pour récuperer une trame valide de réception.
*
* @return de type uint8_t, elle retourne 1 si la trame recue est valide et 0 si la trame recue est invalide.
*/

uint8_t parseRxLidar(uint8_t u8Data)
{
	switch(state2)
	{
		case DEBUT_TRAME:
		if(u8Data == 0x59)
		{
			checksum = 0;
			indexPayload2 = 0;
			indexTrame2 = 0;
			trameLidar[indexTrame2++] = u8Data;
			state2 = DEBUT_TRAME2;
		}
		break;
		
		case DEBUT_TRAME2:
		if(u8Data == 0x59)
		{
			trameLidar[indexTrame2++] = u8Data;
			state2 = DATA;
		}
		break;
		
		case DATA:
		if(indexPayload2 < TRAME_LIDAR_SIZE)
		{
			trameLidar[indexTrame2++] = u8Data;
		}
		else
		{
			state2 = CHECK_SUM;
		}
		indexPayload2++;
		break;
		
		case CHECK_SUM:
		for(uint8_t i = 0;i<8; i++)
		{
			checksum += trameLidar[i];
		}
		if(trameLidar[8] == checksum)
		{
			retour = 1;
			for(uint8_t x = 0; x<9; x++)
			{
				trameLidarValide[x] = trameLidar[x];
			}
			dist = trameLidarValide[2] + (trameLidarValide[3]<<8);
			amp = trameLidarValide[4] + (trameLidarValide[5]<<8);
			temp = trameLidarValide[6] + (trameLidarValide[7]<<8);
			temp = (temp / 8) - 256;
		}
		else
		{
			retour = 0;
		}
		memset(trameLidar, 0, sizeof(trameLidar));
		state2 = DEBUT_TRAME;
		break;
	}
	return retour;
}

/**
* @brief Cette fonction s'agit d'un getter qui récupere la distance en cm détecté par le Lidar.
*
* @return de type uint16_t retourne la distance en cm détecté par le Lidar.
*/

uint16_t getDist()
{
	return dist;
}

/**
* @brief Cette fonction s'agit d'un getter qui récupere la luminosté en amp par le Lidar. Si cette valeur est supérieur à 65000 si mesure de distance n'est pas bonne.
*
* @return de type uint16_t retourne la luminosté en amp détecté par le Lidar.
*/

uint16_t getAmp()
{
	return amp;
}

/**
* @brief Cette fonction s'agit d'un getter qui récupere la température du module en Celsius détecté par le Lidar.
*
* @return de type uint16_t retourne la température du module en Celsius détecté par le Lidar.
*/

uint16_t getTemp()
{
	return temp;
}