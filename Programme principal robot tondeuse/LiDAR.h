/*
* LiDAR.h
*
* Created: 2023-03-28 10:54:14 AM
*  Author: adino
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