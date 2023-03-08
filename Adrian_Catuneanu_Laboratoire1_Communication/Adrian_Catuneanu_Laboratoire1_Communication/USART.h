/**
* @file USART.h
* @Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 13 novembre 2021
* @brief Cette bibliotheque contient les prototypes de fonctions pour la communication USART a partir du atmega32u4 vers le PC et vice-versa via le fil USART a usb. 
*/


#ifndef USART_H_
#define USART_H_
#define F_CPU 16000000
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t usartSendByte(uint8_t u8Data);
void usartInit(uint32_t baudRate, uint32_t fcpu);
uint8_t usartRxAvailable();
uint8_t usartRemRxData();
uint8_t usartSendString(const char * str);
uint8_t usartSendBytes(const uint8_t * source, uint8_t size);



#endif /* USART_H_ */