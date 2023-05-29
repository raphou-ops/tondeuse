/**
* @file USART0.h
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.1
* @date 12 avril 2023
* @brief Cette bibliotheque contient les prototypes de fonctions pour la communication USART à partir du atmega2560 avec le module TF-Luna mini Lidar. On utilise seulement les fonctions de réception de cette librairie pour recevoir la distance mesuré par le Lidar.
*/

#ifndef USART0_H_
#define USART0_H_
#define F_CPU 16000000
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t usart0SendByte(uint8_t u8Data);
void usart0Init(uint32_t baudRate, uint32_t fcpu);
uint8_t usart0RxAvailable();
uint8_t usart0RemRxData();
uint8_t usart0SendString(const char * str);
uint8_t usart0SendBytes(const uint8_t * source, uint8_t size);

#endif /* USART0_H_ */