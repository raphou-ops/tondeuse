/**
* @file lcd.h
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.0
* @date 17 septembre 2021
* @brief Cette bibliothèque contient la declaration des macros pour pouvoir interagir avec le HD44780 à partir du atmega2560 ainsi que le prototype des fonction dans le fichier lcd.c
*/


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#define F_CPU 16000000 //fixer la vitesse du microprocesseur
#include <util/delay.h> //inclure la librairie publique delay

#define LCD_RS_INIT()    DDRC |= (1<<2)//PD1 pin 10
#define LCD_RS_ON()      PORTC |= (1<<2)
#define LCD_RS_OFF()     PORTC &= ~(1<<2)

#define LCD_E_INIT()     DDRC |= (1<<7)//PD0 pin 9
#define LCD_E_ON()       PORTC |= (1<<7)
#define LCD_E_OFF()      PORTC &= ~(1<<7)

#define LCD_DB4_INIT()   DDRC |= (1<<3)//PD4 pin 8
#define LCD_DB4_ON()     PORTC |= (1<<3)
#define LCD_DB4_OFF()    PORTC &= ~(1<<3)

#define LCD_DB5_INIT()   DDRC |= (1<<4)//PC6 pin 7
#define LCD_DB5_ON()     PORTC |= (1<<4)
#define LCD_DB5_OFF()    PORTC &= ~(1<<4)

#define LCD_DB6_INIT()   DDRC |= (1<<5)//PD7 pin 6
#define LCD_DB6_ON()     PORTC |= (1<<5)
#define LCD_DB6_OFF()    PORTC &= ~(1<<5)

#define LCD_DB7_INIT()   DDRC |= (1<<6)//PE6 pin 5
#define LCD_DB7_ON()     PORTC |= (1<<6)
#define LCD_DB7_OFF()    PORTC &= ~(1<<6)

//Prototypes des fonctions publiques

void lcdInit();

void lcdPutc(char c);

void lcdPuts(char*str);

void lcdSetPos(uint8_t x, uint8_t y);
void lcdClearScreen();
void lcdPuts(char*str);

void lcdSetPos(uint8_t x, uint8_t y);




#endif /* LCD_H_ */