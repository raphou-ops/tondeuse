/*
* ZED_F9P.h
*
* Created: 2023-03-10 10:36:13 AM
*  Author: adino
*/


#ifndef ZEDF9P_H_
#define ZEDF9P_H_
#define F_CPU 16000000
#include <stdio.h>
#include <string.h>

uint8_t parseRxUbxNavPvt(uint8_t u8Data);
/*uint8_t sendGpsData(uint8_t *u8Data);*/

// uint8_t getNavPvtiTOW();
// uint8_t getNavPvtYear();
// uint8_t getNavPvtMonth();
// uint8_t getNavPvtDay();
// uint8_t getNavPvtHour();
// uint8_t getNavPvtMin();
// uint8_t getNavPvtSec();
// uint8_t getNavPvtValid();
// uint8_t getNavPvtTacc();
// uint8_t getNavPvtNano();
// uint8_t getNavPvtFixType();
// uint8_t getNavPvtFlags();
// uint8_t getNavPvtFlags2();
// uint8_t getNavPvtNumSv();
float getNavPvtLon();
float getNavPvtLat();
// uint8_t getNavPvtHeight();
// uint8_t getNavPvtHmsl();
// uint8_t getNavPvtHacc();
// uint8_t getNavPvtVacc();
// uint8_t getNavPvtVelN();
// uint8_t getNavPvtVelE();
// uint8_t getNavPvtVelD();
// uint8_t getNavPvtGspeed();
// uint8_t getNavPvtHeadMot();
// uint8_t getNavPvtSacc();
// uint8_t getNavPvtHeadAcc();
// uint8_t getNavPvtPdop();
// uint8_t getNavPvtReserved1();
// uint8_t getNavPvtHeadVeh();
// uint8_t getNavPvtMagDec();
// uint8_t getNavPvtMagAcc();




#endif /* ZED-F9P_H_ */