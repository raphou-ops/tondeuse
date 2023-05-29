/**
* @file ZEDF9P.h
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Cette bibliothèque possède le prototype de fonctions qui servent à traiter les donnés GPS du module ZEDF9P recues par le UART au atmega2560. De plus, cette bibliothèque possède plusieurs getters pour convertir et réecuperer plusieurs données comme le type de fix du module, la longitude, la latitude, la précision en 2D, etc. On fonctionne avec la datasheet de U-BLOX qui explique le protocol UBX. Finalement, on utilise majoritairement la trame UBX-NAV-PVT pour récupere ces données et on a aussi programmé des fonctions pour la configuration du module par UART à partir du atmega2560.
* @version 1.1
* @date 2023-05-10
*
* @copyright Copyright (c) 2023
*/

#ifndef ZEDF9P_H_
#define ZEDF9P_H_
#define F_CPU 16000000
#include <stdio.h>
#include <string.h>

//Les fonctions en commentaire sont présentes dans le fichier au besoin

uint8_t parseRxUbxNavPvt(uint8_t u8Data);
uint16_t calculerChecksum(uint8_t*trame,uint8_t size);
void envoieConfigMsg( uint8_t id,uint8_t etatUart1,uint8_t etatUsb);
void envoieConfigRate(uint8_t frequence);
void envoieConfigPortUart1(uint32_t baudrate);
void envoieConfigOdo(uint8_t etatOdo,uint8_t etatLowSpeed,uint8_t etatVelocity,uint8_t etatHeading,uint8_t speedThreshold,uint8_t maxAccuracy,uint8_t velocityLevel,uint8_t headingLevel );

uint8_t getAckConfigUart1();
uint8_t getAckConfigRate();
uint8_t getAckConfigMsg();
uint8_t getAckConfigOdo();
uint8_t getId();

// uint8_t getNavPvtiTOW();
// uint8_t getNavPvtYear();
// uint8_t getNavPvtMonth();
// uint8_t getNavPvtDay();

void getNavPvtTime(char* msg);
uint8_t getNavPvtValid();

// uint8_t getNavPvtTacc();
// uint8_t getNavPvtNano();

unsigned char getNavPvtFixType();
uint8_t getNavPvtFlags();
uint8_t getNavPvtFlags2();

// uint8_t getNavPvtNumSv();

float getNavPvtLon();
float getNavPvtLat();

// uint8_t getNavPvtHeight();
// uint8_t getNavPvtHmsl();

unsigned long getNavPvtHacc();
unsigned long getNavPvtVacc();

// uint8_t getNavPvtVacc();
// uint8_t getNavPvtVelN();
// uint8_t getNavPvtVelE();
// uint8_t getNavPvtVelD();

long getNavPvtGspeed();
float getNavPvtHeadMot();
unsigned long getNavPvtSacc();
float getNavPvtHeadAcc();
float getNavPvtPdop();

// uint8_t getNavPvtReserved1();

float getNavPvtHeadVeh();

// uint8_t getNavPvtMagDec();
// uint8_t getNavPvtMagAcc();


#endif /* ZED-F9P_H_ */