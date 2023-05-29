/**
* @file LiDAR.h
* @author Adrian Catuneanu et Rapha�l Tazbaz
* @brief Cette biblioth�que poss�de le prototype des fonctions pour le traitement de la trame de reception envoy�e par le LiDAR. De plus, elle poss�de plusieurs getters pour la r�cuperation de la distance, les amp et la temp�rature interne du module.
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