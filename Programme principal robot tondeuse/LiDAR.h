/**
* @file LiDAR.h
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Cette bibliothèque possède le prototype des fonctions pour le traitement de la trame de reception envoyée par le LiDAR. De plus, elle possède plusieurs getters pour la récuperation de la distance, les amp et la température interne du module.
* @version 1.0
* @date 12 avril 2023
*
* @copyright Copyright (c) 2023
*
*/

#ifndef LIDAR_H_
#define LIDAR_H_

#define F_CPU 16000000
#include <stdio.h>
#include <string.h>

uint8_t parseRxLidar(uint8_t u8Data);

uint16_t getDist();
uint16_t getAmp();
uint16_t getTemp();

#endif /* LIDAR_H_ */