/**
* @file USART1.h
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.1
* @date 20 mars 2023
* @brief Cette bibliotheque contient les prototypes de fonctions pour la communication USART à partir du atmega2560 avec le module GPS RTK simpleRTK2B de ardusimple avec la chip ZEDF9P. On utilise majoritairement les fonctions de réception pour recevoir la position en latitude longitude et le différents paramètres de la trame UBX-NAV-PVT. De plus, on utilise les fonctions d'envoi pour configurer le module GPS au démmarage du robot.
*/


#ifndef USART1_H_
#define USART1_H_
#define F_CPU 16000000
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t usartGpsSendByte(uint8_t u8Data);
void usartGpsInit(uint32_t baudRate, uint32_t fcpu);
uint8_t usartGpsRxAvailable();
uint8_t usartGpsRemRxData();
uint8_t usartGpsSendString(const char * str);
uint8_t usartGpsSendBytes(const uint8_t * source, uint8_t size);



#endif /* USART1_H_ */