/**
* @file lcd.c
* @author Adrian Catuneanu (adinone3@gmail.com)
* @version 1.0
* @date 17 septembre 2021
* @brief Cette bibliothèque offre les definitions des fonctions pour l’utilisation d’un afficheur LCD HD44780 à l'aide du atmega2560.
*/

#include "lcd.h"

//Prototypes des fonctions privees

void _lcdWrite4bits(uint8_t data, uint8_t rs);
void _lcdWrite8Bits(uint8_t data, uint8_t rs);

/**
* @brief Cette fonction gere l'affichage du LCD en mode 4 bits. (Le specifications de la datasheet du HD44780)
* @param uint8_t data Recoit la commande qu'on veut faire executer au LCD.
* @param uint8_t rs Recoit la validation et execute la commande data si rs est a 1.
* @return void
*/

void _lcdWrite4bits(uint8_t data, uint8_t rs)
{
	if(rs)
	LCD_RS_ON();
	else
	LCD_RS_OFF();
	
	LCD_E_ON();
	
	if(data & 1)
	LCD_DB4_ON();
	else
	LCD_DB4_OFF();
	
	if(data & 2)
	LCD_DB5_ON();
	else
	LCD_DB5_OFF();
	
	if(data & 4)
	LCD_DB6_ON();
	else
	LCD_DB6_OFF();
	
	if(data & 8)
	LCD_DB7_ON();
	else
	LCD_DB7_OFF();
	
	_delay_us(1);
	
	LCD_E_OFF();
	
	_delay_us(1);
}

/**
* @brief Cette fonction gere l'affichage du LCD en mode 8 bits a l'aide du mode 4bits et de la variable rs (pour economiser des pins)
* @param uint8_t data Recoit la commande qu'on veut faire executer au LCD.
* @param uint8_t rs Recoit la validation et execute la commande data si rs est a 1.
* @return void
*/

void _lcdWrite8Bits(uint8_t data, uint8_t rs)
{
	_lcdWrite4bits(data>>4,rs);
	_lcdWrite4bits(data,rs);
	_delay_us(40);
}

/**
* @brief Cette fonction s'agit des etapes requises dans la datasheet pour inititialiser le LCD  sur deux lignes en mode 4 bits 32 caracateres avec le curseur a on qui clignote
* @return void
*/

void lcdInit()
{
	LCD_RS_INIT();
	LCD_E_INIT();
	LCD_DB4_INIT();
	LCD_DB5_INIT();
	LCD_DB6_INIT();
	LCD_DB7_INIT();
	
	
	_delay_ms(20);
	
	_lcdWrite4bits(3,0);
	
	_delay_ms(5);
	
	_lcdWrite4bits(3,0);
	
	_delay_ms(1);
	
	_lcdWrite4bits(3,0);
	
	_delay_ms(1);
	
	_lcdWrite4bits(2,0); // mode 4 bits active
	
	_delay_ms(1);
	
	_lcdWrite8Bits(0x28,0); // mode 4 bits - 2 lignes
	
	_lcdWrite8Bits(0x08,0); // LCD a off
	
	_lcdWrite8Bits(0x01,0); // Clear LCD
	
	_delay_ms(2);
	
	_lcdWrite8Bits(0x06,0); // Incremente vers la gauche
	
	_lcdWrite8Bits(0x0F,0); // LCD a on, curseur a on qui clignote
}

/**
* @brief Cette fonction sert a afficher un caractere de type char sur le LCD.
* @param char c, recoit le caractere desirer qu'il va imprimmer sur le LCD.
* @return void
*/

void lcdPutc(char c)
{
	_lcdWrite8Bits(c,1);
}

/**
* @brief Cette fonction sert a afficher les caracteres 1 a 1 pour une chaine d'un maximum de 255 caracteres.
* @param str chaine de caracteres (char *) longueur maximum de 255. Recoit la string desirer qu'il va imprimmer sur le LCD.
* @return void
*/

void lcdPuts(char*str)
{
	for(uint8_t i = 0; str[i] && i < 255 ; i++)
	{
		lcdPutc(str[i]);
	}
}

/**
* @brief Cette fonction gere le positionnement en horizontale (x) et en verticale (y) du curseur.
* @param uint8_t x, Recoit la position horizontale qu'on veut envoyer le curseur.
* @param uint8_t y, Recoit la position verticale qu'on veut envoyer le curseur.
* @return void
*/

void lcdSetPos(uint8_t x, uint8_t y)
{
	uint8_t cmd = 0x80; // commande pour postionner l'adresse DDRAM
	
	if (y)
	cmd |= (1<<6); // positionner l'adresse a 0x40 pour la ligne 2.
	cmd |= (x & 0xF);
	_lcdWrite8Bits(cmd,0);
}

/**
* @brief Cete fonction sert à reset l'ecriture afficher sur le LCD. En résumé elle enleve tout les caractères affichés sur le LCD
*/

void lcdClearScreen()
{
	_lcdWrite8Bits(0x01,0); // Clear LCD
	_delay_ms(2);
}

