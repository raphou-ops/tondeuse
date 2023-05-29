/**
* @file USART2.h
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.1
* @date 12 avril 2023
* @brief Cette bibliothèque offre les prototypes des fonctions pour la communication USART à l'aide du atmega2560 avec le module HC-06 Bluetooth. On utilise les fonctions d'envoi pour transmettre les états du robot vers le serveur nodeJs et le fonctions de reception pour recevoir les commandes à partir du site web.
*/


#ifndef USART2_H_
#define USART2_H_
#define F_CPU 16000000
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

uint8_t usart2SendByte(uint8_t u8Data);
void usart2Init(uint32_t baudRate, uint32_t fcpu);
uint8_t usart2RxAvailable();
uint8_t usart2RemRxData();
uint8_t usart2SendString(const char * str);
uint8_t usart2SendBytes(const uint8_t * source, uint8_t size);
uint8_t parseBluetoothManuel(uint8_t data);
uint8_t parseBluetoothAuto(uint8_t data);
int getJoystickGaucheX();
int getJoystickGaucheY();
long getLon();
long getLat();
uint8_t getBoutonX();
uint8_t getBoutonO();

#endif /* USART2_H_ */