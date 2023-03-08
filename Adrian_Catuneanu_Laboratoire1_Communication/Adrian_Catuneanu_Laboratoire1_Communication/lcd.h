/**
* @file lcd.h
* @Adrian Catuneanu (adinone3@gmail.com)
* @version 0.01
* @date 17 septembre 2021
* @brief Cette bibliotheque contient la declaration des macros pour pouvoir interagir avec le HD44780 a partir du atmega32u4 ainsi que le prototype des fonction dans le fichier .lcd
*/


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#define F_CPU 16000000 //fixer la vitesse du microprocesseur
#include <util/delay.h> //inclure la librairie publique delay

#define LCD_RS_INIT()    DDRH |= (1<<4)//PD1 pin 10
#define LCD_RS_ON()      PORTH |= (1<<4)
#define LCD_RS_OFF()     PORTH &= ~(1<<4)                                                       

#define LCD_E_INIT()     DDRH |= (1<<3)//PD0 pin 9
#define LCD_E_ON()       PORTH |= (1<<3)
#define LCD_E_OFF()      PORTH &= ~(1<<3)

#define LCD_DB4_INIT()   DDRE |= (1<<3)//PD4 pin 8
#define LCD_DB4_ON()     PORTE |= (1<<3)
#define LCD_DB4_OFF()    PORTE &= ~(1<<3)

#define LCD_DB5_INIT()   DDRG |= (1<<5)//PC6 pin 7
#define LCD_DB5_ON()     PORTG |= (1<<5)
#define LCD_DB5_OFF()    PORTG &= ~(1<<5)

#define LCD_DB6_INIT()   DDRE |= (1<<5)//PD7 pin 6
#define LCD_DB6_ON()     PORTE |= (1<<5)
#define LCD_DB6_OFF()    PORTE &= ~(1<<5)

#define LCD_DB7_INIT()   DDRE |= (1<<4)//PE6 pin 5
#define LCD_DB7_ON()     PORTE |= (1<<4)
#define LCD_DB7_OFF()    PORTE &= ~(1<<4)

//Prototypes des fonctions publiques

void lcdInit();

void lcdPutc(char c);

void lcdPuts(char*str);

void lcdSetPos(uint8_t x, uint8_t y);
void lcdClearScreen();
void lcdPuts(char*str);

void lcdSetPos(uint8_t x, uint8_t y);




#endif /* LCD_H_ */