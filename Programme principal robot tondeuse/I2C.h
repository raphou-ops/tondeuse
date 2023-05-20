/*
* I2C.h
*
* Created: 2022-03-13 15:05:12
*  Author: rapho
*/
#define F_CPU 16000000
#include "avr/io.h"
#include "lcd.h"
#include <math.h>
#include <util/delay.h>

#define RAYON_TERRE	 637800000.0//cm
#ifndef I2C_H_
#define I2C_H_
#define ANGLE_DECLIN  13.783333
#define I2C_DTA_SIZE_MAX 64
#define QMC5883_ADDRESS_MAG (0x0D<<1)

/**
* \brief initialise la vitesse de communication et active la communication i2c
*
*
* \return void
*/
void i2cInit();
/**
* \brief verifie si le master est occup� � lire ou � �crire
*
*
* \return uint8_t retourne 1 s'il est occup�
*/
uint8_t i2cIsBusy();
/**
* \brief v�rifie si une donn�e est disponible
*
*
* \return uint8_t retourne le nombre de donn�e disponible en r�ception
*/
uint8_t i2cDataAvailable();
/**
* \brief r�cup�re la derni�re donn�e entr�
*
*
* \return uint8_t retourne la donn�e
*/
uint8_t i2cGetData();
/**
* \brief verifie s'il y a une erreur quelconque
*
*
* \return uint8_t retourne le code d'erreur
*/
uint8_t i2cGetErrorCode();
/**
* \brief envoie un octet de donn�e sur un esclave
*
* \param add repr�sente l,adresse de l'esclave
* \param data repr�sente l'octet � envoyer
*
* \return void
*/
void i2cWriteByte(uint8_t add,uint8_t reg,uint8_t data);
/**
* \brief  lit un octet de l'escalve
*
* \param add l'adresse de l'esclave
*
* \return void
*/
void i2cReadByte(uint8_t add,uint8_t reg);
/**
* \brief envoie un ou plusieurs octets sur un eslcave
*
* \param add  l'adresse de l'esclave
* \param source pointe l'octet � envoyer
* \param size	repr�sente le nombre d'octet � envoyer
*
* \return void
*/
void i2cWriteBytes(uint8_t add,uint8_t*source,uint8_t size);

/**
* \brief lit un ou plusieurs octet d'un esclave
*
* \param add  l'adresse de l'esclave
* \param size repr�sente le nombre d'octet � lire
*
* \return void
*/
void i2cReadBytes(uint8_t add,uint8_t reg,uint8_t size);

/**
* \brief met l'index de donn�e � 0
*
*
* \return void
*/
void i2cClearDataAvailable();

void setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr);

void setReset();


//qmc5883

int getX();
int getY();
int getZ();
float getAzimuth();


float course(float lat1, float lon1, float lat2, float lon2) ;
float toDegrees(float radians);
float deg_toRad(float deg);
float distance_entre_point(float lat1,float lon1,float lat2,float lon2);


void setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max);
void _applyCalibration();
uint8_t calibration();

void setSmoothing(uint8_t steps, uint8_t adv);
void _smoothing();

#endif /* I2C_H_ */