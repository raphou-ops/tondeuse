/**
* @file main.c
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Fichier qui possède les différentes fonctions pour le contrôle du robot et l'interface utilisateur par clavier matriciel. Dans ce fichier on appelle toutes les différentes bibliothèques pour contrôler le robot dans tout les modes: Manuel, Prog, Auto, Set Heure de coupe et Set Nip.
* @version 1.2
* @date 24 mai 2023
*
* @copyright Copyright (c) 2023
*/

#define F_CPU 16000000
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "USART0.h"
#include "USART1.h"
#include "USART2.h"
#include "lcd.h"
#include "ZEDF9P.h"
#include "LiDAR.h"
#include "I2C.h"

/*Variables globales et constantes GPS*/

#define USB_ON 1
#define USB_OFF 0
#define USART1_ON 1
#define USART1_OFF 0
#define PVT 7
#define POLSSH 2
#define ODO 9
#define ODO_MESSAGE 0x1E

uint8_t etat = 0;
char msgGpsLat[16];
char msgGpsLon[16];
char msgTime[20];
uint8_t pos = 0;
uint8_t row = 0;
uint8_t cnt;
float lat = 0.000000;
float lon = 0.000000;
unsigned long verticalAcc=0.0;
unsigned long horizontalAcc=0.0;
uint8_t dataGps;
uint8_t setRate[14]={0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x01, 0x00, 0x00, 0x00, 0x00, 0x37};

/*Variables globales Bluetooth*/

uint8_t dataBluetooth = 0;

uint16_t nbPointGpsDispo=3;//mettre 0  pour les données js
enum EtapeModeAuto{ORIENTER,TOURNER,AJUSTER};
enum EtapeModeAuto etapeModeAuto=ORIENTER;
uint16_t indexDestination=0; //0 si jamais le traitement bogue
float tabLatMem[2]={45.64461,45.64454};
float tabLonMem[2]={-73.84276,-73.84287};

float tabLatMemJs[100];
float tabLonMemJs[100];

uint8_t cntLatMemoireJs = 0;
uint8_t cntLonMemoireJs = 0;
enum Orientation{DISTANCE,ANGLE,FINI};
enum Orientation orientation=DISTANCE;

//controle manette ps4 joystickX et joystickY dans section joystick physique

/*Variables globales LiDAR*/

uint8_t dataLiDAR = 0;
char msgLiDAR[20];

/*Variables globales encodeurs*/

volatile uint8_t refreshDist = 1;
volatile uint8_t encodeurEtatPrecedent = 0;
volatile uint8_t encodeurEtatPrecedent2 = 0;
volatile uint16_t encodeurGCnt = 0;
volatile uint16_t encodeurDCnt = 0;

char msgEncoD[8];
char msgEncoG[8];

/*Variables globales joystick physique*/

int joysticX = 0;
int joysticY = 0;
uint16_t receptionAdc = 0;
uint8_t triAdc = 0;
int vitesseMoteurG = 0;
int vitesseMoteurD= 0;

#define BT2() ((PINA&(1<<0))==0)
#define BT1() ((PINA&(1<<1))==0)
#define BT4() ((PINA&(1<<2))==0)
#define BT3() ((PINA&(1<<3))==0)

#define JOYSTICK_X 1
#define JOYSTICK_Y 2
#define VIT_MIN 512

/*Variables clavier matriciel*/

char key = 0;
char lastKey = 0;
uint8_t cntPassword = 0;
char password[4] = {'1','2','3','4'};
char essaiUser[4];
char passwordValid = 0;
uint8_t modeTondeuse = 0;

uint8_t cntMenu = 0;

uint8_t set = 0;
uint8_t setTemps = 0;
uint8_t heureCoupe = 0;
uint8_t minutesCoupe = 0;
char tempsCoupe[5];

enum gestionModes{MANUEL,PROG,AUTO,SET_TIME,SET_NIP,DEFAULT};
enum gestionModes modeActuel = MANUEL;

/*États et pourcentage de la batterie*/

volatile float niveauBatterie = 0;

/*Variables pour PID*/

#define KP 28
#define KI 1
#define KD 0.1

#define TAILLE_FENETRE 10
int window[TAILLE_FENETRE];
int erreur = 0;//déjé calculé, proportionnel
int valSommeErreur = 0;// intégrale
int erreurPrec = 0;
int variation = 0;//dérivé
int valPid = 0;
uint8_t cntSommeErreur = 0;
int derivation=0;
/*Variables timer0 refresh du système*/

volatile uint8_t cntDixiemeDeSec = 0;
volatile uint16_t cntDixiemeDeSecLCD = 0;
volatile uint8_t refreshManuel = 1;
volatile uint8_t refreshClavier = 0;
volatile uint8_t refreshLCD = 0;
volatile uint8_t refreshEnvoi = 0;
volatile uint8_t refreshAuto = 0;

/*Variables calcul compas*/

float distanceDest=0;
float angleDest=0;
float angleActuel=0;
float distanceMoyParcouru=0;

/*Variables Bluetooth envoi vers la tondeuse*/

char msgEnvoi[50];
uint8_t flagPluie = 0;
uint8_t flagLidar = 0;
uint16_t distanceObjet = 20;
uint8_t etatCoupe = 0;
uint8_t flagCalibration = 0;
uint8_t toucheManette = 0;
uint8_t flagEnvoi = 0;
uint8_t flagRetourMaison=0;

/*Déclaration des fonctions*/

void adcInit();//initialise l'adc
void moteurInit();
char lireClavier();
void controleManuel();
uint8_t tourner(uint16_t angleDest, uint16_t angleActuel);
void ajusterDrirection(uint16_t angleDest, uint16_t angleActuel);//avec pid
void retourMaison(float latitudeMaison,float longitudeMaison);
void sortieMaison();
void effectuerCalibration();
void initBoussole();

/*Programme principal*/

/**
* @brief La fonction main possède l'initialisation du module QMC5883L, l'intialisation de la communication I2C (QMC5883L), USART0 (LiDAR), USART1 (GPS RTK) et USART2 (Bluetooth), l'initialisation du timer0 pour le rafraîchissement, l'envoi des messages de configuration pour le module simpleRTK2B (ZEDF9P), l'initialisation des interruptions externes sur PCINT1 et PCINT2, l'initialisation des registres PWM (OCR1 (lame), OCR4 (moteur gauche), OCR5 (moteur droit) du atmega2560, l'initialisation des trois registres ADC utilisées pour les capteurs, l'activation de la pull-up interne sur le PORTA du atmega2560 pour le clavier matriciel,l'initialisation de l'écran LCD HD44780 et la gestion du mot de passe rentrée par l'utilisateur.
*/

int main(void)
{
	lcdInit();
	
	initBoussole();
	_delay_ms(100); //délai pour laisser le temps au compas de bien s'initialiser
	setSmoothing(10,1);
	
	PORTA|=0x0F;//initclavier
	
	_delay_ms(100);
	
	lcdPuts("Enter Password:");
	lcdSetPos(0,1);
	
	adcInit();
	moteurInit();
	
	ADCSRA |= (1<<ADSC); //debut conversion ADC.
	
	TCCR0B |= (1<<CS01) | (1<<CS00); //avec diviseur de clock /64.
	TCCR0A |= (1<<WGM01);//Configuration du timer 0 en CTC
	TIMSK0 |= (1<<OCIE0A);//Output Compare Match A Interrupt Enable.
	OCR0A = 249;//Top a la valeur 249 afin de obtenir un periode de 1ms fixe.
	
	sei();
	
	PORTJ |= 0b00011000;//active pull-up sur PB4 et PB5
	PCICR |= 0b00000010;//active la lecture sur PCINT4 et PCINT5
	PCMSK1 |= 0b00110000;//active les interruption sur PCINT4 et PCINT5
	
	usart0Init(115200,16000000);
	usartGpsInit(9600,16000000);
	usart2Init(115200,16000000);
	
	do{
		if(refreshClavier)
		{
			refreshClavier = 0;
			key = lireClavier();
			
			if((key)&&(!lastKey))
			{
				lcdSetPos(cntPassword,1);
				essaiUser[cntPassword++] = key;
				lcdPutc(key);
				if(cntPassword == 4)
				{
					for(uint8_t index = 0; index<4; index++)
					{
						if(essaiUser[index] == password[index])
						{
							passwordValid = 1;
						}
						else
						{
							lcdClearScreen();
							lcdPuts("Invalid password");
							lcdSetPos(0,1);
							cntPassword = 0;
							passwordValid = 0;
						}
					}
				}
			}
			lastKey = key;
		}
	}while(!passwordValid);
	envoieConfigPortUart1(9600);
	envoieConfigRate(1);
	envoieConfigMsg(PVT,USART1_ON,USB_ON);
	passwordValid = 0;
	cntPassword = 0;
	lcdClearScreen();
	lcdSetPos(0,0);
	lcdPuts("Mode Manuel  <-");
	
	//Partie a décommenter pour les tests du mode auto (facilite les tests)
	/*
	static uint8_t executed = 1;
	if (executed) {
	effectuerCalibration();
	executed = 0;
	}
	*/
	
	/**
	* @brief La boucle principale du programme qui gére le différents modes du robot tondeuse: Manuel, Prog, Auto, Set Time et Set Nip. Dans le mode manuel, l'utilisateur peut contrôler le robot à l'aide du joystick physique ou la manette de ps4 par connection bluetooth entre le hc-05 et le serveur nodeJs. Dans le mode prog, l'utilisateur peut envoyer les donnés de longitude latitude génerés sur le serveur local nodeJS au robot tondeuse, ces points correspondent aux différents coordonnées à atteindre pendant son fonctionnement automatique en ordre (son tracé).
	*/
	while (1)
	{
		if(refreshClavier)
		{
			refreshClavier = 0;
			key = lireClavier();
			
			if((key)&&(!lastKey))
			{
				lcdClearScreen();
				lcdSetPos(0,0);
				switch(key)
				{
					case '1':
					modeTondeuse = cntMenu;
					if(modeTondeuse == 0)
					{
						modeActuel = MANUEL;
					}
					else if(modeTondeuse == 1)
					{
						modeActuel = PROG;
					}
					else if(modeTondeuse == 2)
					{
						modeActuel = AUTO;
					}
					else if(modeTondeuse == 3)
					{
						modeActuel = SET_TIME;
					}
					else if(modeTondeuse == 4)
					{
						modeActuel = SET_NIP;
					}
					cntLatMemoireJs = 0;
					cntLonMemoireJs = 0;
					set = 1;
					break;
					
					case '2':
					if(cntMenu>0)
					cntMenu --;
					else
					cntMenu = 4;
					
					break;
					
					case '3':
					if(cntMenu<4)
					cntMenu ++;
					else
					cntMenu = 0;
					break;
					
					case '4':
					cntMenu = modeTondeuse;
					set = 1;
					break;
				}
				if(cntMenu == 0)
				{
					lcdPuts("Mode Manuel ");
				}
				else if(cntMenu == 1)
				{
					lcdPuts("Mode Prog   ");
				}
				else if(cntMenu == 2)
				{
					lcdPuts("Mode Auto   ");
				}
				else if(cntMenu == 3)
				{
					lcdPuts("Set time    ");
				}
				else if(cntMenu == 4)
				{
					lcdPuts("Set password");
				}
				if(set)
				{
					set = 0;
					setTemps = 0;
					passwordValid = 0;
					lcdSetPos(13,0);
					lcdPuts("<-");
					flagEnvoi = 1;
				}
			}
			lastKey = key;
		}
		switch(modeActuel)
		{
			case MANUEL:
			etatCoupe=0;
			controleManuel();
			if(usart0RxAvailable())
			{
				dataLiDAR = usart0RemRxData();
				if(parseRxLidar(dataLiDAR))
				{
					distanceObjet = getDist();
				}
			}
			if(distanceObjet <= 14)
			{
				flagLidar = 1;
				OCR4B=0;
				OCR5B=0;
				OCR4C=0;
				OCR5C=0;
				joysticX = 0;
				joysticY = 0;
			}
			else
			{
				flagLidar = 0;
				//DDRB|=(1<<6);
			}
			
			if(usart2RxAvailable())
			{
				dataBluetooth = usart2RemRxData();
				parseBluetoothManuel(dataBluetooth);
			}
			joysticX = getJoystickGaucheX();
			joysticY = getJoystickGaucheY();
			if(getBoutonX() == 0x31)
			{
				OCR1B=1024;
				DDRB|=(1<<6); //activer la lame
			}
			else
			{
				OCR1B=90;
				DDRB &= ~(1<<6); //désactiver la lame
			}
			toucheManette = getBoutonO();
			if((toucheManette == 0x31)&&(flagCalibration!=0x31))
			{
				effectuerCalibration();
			}
			flagCalibration = toucheManette;
			
			// À décommenter pour l'affichage de l'heure sur le LCD
			
			// 			if(refreshLCD)
			// 			{
			// 				refreshLCD = 0;
			// 				lcdSetPos(0,1);
			// 				lcdPuts(msgTime);
			// 			}
			break;
			
			case PROG:
			if(usart2RxAvailable())
			{
				dataBluetooth = usart2RemRxData();
				if(parseBluetoothAuto(dataBluetooth))
				{
					tabLatMemJs[cntLatMemoireJs++] = (float)(getLat()*(1e-6));
					tabLonMemJs[cntLonMemoireJs++] = (float)(getLon()*(1e-6));
					usart2SendString("C;ack;");
					nbPointGpsDispo++;
				}
			}
			else
			{
				// Tests pour le reset des cnt dans la mémoire
				
				//cntLonMemoireJs = 0;
				//cntLatMemoireJs = 0;
				
				if(refreshEnvoi)
				{
					float latEnvoi = getNavPvtLat();
					float longEnvoi = getNavPvtLon();
					sprintf(msgEnvoi,"S;%.2f;%d;%d;%d;%f;%f;%d;",niveauBatterie,flagPluie,etatCoupe,flagLidar,latEnvoi,longEnvoi,modeTondeuse);
					usart2SendString(msgEnvoi);
					refreshEnvoi = 0;
				}
			}
			break;
			
			case AUTO:
			
			// À décommenter pour l'affichage de l'heure sur le LCD
			
			// 			if(refreshLCD)
			// 			{
			// 				refreshLCD = 0;
			// 				lcdSetPos(0,1);
			// 				lcdPuts(msgTime);
			// 			}
			
			if(usart0RxAvailable())
			{
				dataLiDAR = usart0RemRxData();
				if(parseRxLidar(dataLiDAR))
				{
					distanceObjet = getDist();
				}
			}
			if(distanceObjet <= 14)
			{
				flagLidar = 1;
			}
			else
			{
				flagLidar = 0;
			}
			
			if(etapeModeAuto!=ORIENTER)
			{
				usartGpsRemRxData();
			}
			
			lcdSetPos(0,1);
			sprintf(msgTime,"%f %d %f",distanceMoyParcouru,indexDestination,angleActuel);
			lcdPuts(msgTime);
			
			//Ligne à décommenter pour récuperer le temps actuel à partir du module GPS
			
			//getNavPvtTime(msgTime);
			
			if(flagPluie||(niveauBatterie<50 && niveauBatterie!=0))
			{
				flagRetourMaison=1;
			}
			if(flagRetourMaison)
			{
				retourMaison(tabLatMemJs[0],tabLonMemJs[0]);
			}
			
			if(!flagLidar && !flagRetourMaison)
			{
				if(nbPointGpsDispo>1)
				{
					switch(etapeModeAuto)
					{
						case ORIENTER:
						etatCoupe=1;
						dataGps = usartGpsRemRxData();
						if(parseRxUbxNavPvt(dataGps))
						{
							if(getNavPvtFixType()>=3)//3d fix
							{
								lat=getNavPvtLat();
								lon=getNavPvtLon();
								horizontalAcc=getNavPvtVacc();//en mm
								if(horizontalAcc<=50)//check precision 2d,horizontal
								{
									switch (orientation)
									{
										case DISTANCE:
										cli();
										distanceDest=distance_entre_point(lat,lon,tabLatMemJs[indexDestination],tabLonMemJs[indexDestination]);//lat actuel, lon actuel, lat dest,lon dest
										sei();
										orientation=ANGLE;
										break;
										
										case ANGLE:
										cli();
										angleDest=course(lat, lon,tabLatMemJs[indexDestination],tabLonMemJs[indexDestination]);//index=1 depart
										sei();
										orientation=FINI;
										break;
										
										case FINI:
										etapeModeAuto=TOURNER;
										orientation=DISTANCE;
										_delay_ms(100);
										break;
									}
								}
							}
						}
						break;
						
						case TOURNER:
						etatCoupe=2;
						i2cReadBytes(QMC5883_ADDRESS_MAG,0x0,0x06);//lis registre x,y,z
						angleActuel=getAzimuth();//récuperer le bearing
						
						_delay_ms(10); //délai essentiel pour permettre au atmega2560 de recevoir les données du compas digitale
						
						if(tourner(angleDest,angleActuel))
						{
							encodeurDCnt=0;
							encodeurGCnt=0;
							etapeModeAuto=AJUSTER;
						}
						break;
						case AJUSTER:
						etatCoupe=3;
						i2cReadBytes(QMC5883_ADDRESS_MAG,0x0,0x06);//lis registre x,y,z
						angleActuel=getAzimuth();
						
						_delay_ms(10); //délai essentiel pour permettre au atmega2560 de recevoir les données du compas digitale
						
						ajusterDrirection(angleDest,angleActuel);
						
						if(distanceMoyParcouru>=distanceDest)
						{
							etapeModeAuto=ORIENTER;
							indexDestination++;
							nbPointGpsDispo--;
						}
						else if(derivation>=-5 && derivation<=5)
						{
							distanceMoyParcouru = (((float)(encodeurDCnt+encodeurGCnt))/2.0);
							distanceMoyParcouru = distanceMoyParcouru * 3.55;
						}
						
						break;
					}
				}
				else
				{
					flagRetourMaison=1;
				}
			}
			break;
			
			
			case SET_TIME:
			_delay_ms(200);
			lcdSetPos(0,1);
			sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
			do
			{
				if(refreshClavier)
				{
					refreshClavier = 0;
					key = lireClavier();
					
					switch(key)
					{
						case '1':
						cntMenu = 3;
						modeActuel = DEFAULT;
						setTemps = 1;
						lcdClearScreen();
						lcdSetPos(0,0);
						lcdPuts("Set time  OK");
						lcdSetPos(0,1);
						lcdPuts(msgTime);
						break;
						
						case '2':
						if(heureCoupe<23)
						heureCoupe++;
						else
						heureCoupe = 0;
						
						if((minutesCoupe<10) && (heureCoupe<10))
						{
							sprintf(tempsCoupe,"0%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else if(minutesCoupe<10)
						{
							sprintf(tempsCoupe,"%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else if (heureCoupe<10)
						{
							sprintf(tempsCoupe,"0%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else
						{
							sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						lcdSetPos(0,1);
						lcdPuts(tempsCoupe);
						break;
						
						case '3':
						if(minutesCoupe<59)
						minutesCoupe++;
						else
						minutesCoupe = 0;
						
						if((minutesCoupe<10) && (heureCoupe<10))
						{
							sprintf(tempsCoupe,"0%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else if(minutesCoupe<10)
						{
							sprintf(tempsCoupe,"%d:0%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else if (heureCoupe<10)
						{
							sprintf(tempsCoupe,"0%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						else
						{
							sprintf(tempsCoupe,"%d:%d",heureCoupe,minutesCoupe/*,trameGpsNavPvtValide[16]*/);
						}
						lcdSetPos(0,1);
						lcdPuts(tempsCoupe);
						break;
						
						case '4':
						cntMenu = 3;
						modeActuel = DEFAULT;
						modeTondeuse = 5;
						setTemps = 1;
						lcdClearScreen();
						memset(tempsCoupe, 0, sizeof(tempsCoupe));
						lcdSetPos(0,0);
						lcdPuts("Set time    ");
						lcdSetPos(0,1);
						lcdPuts(msgTime);
						break;
					}
				}
			} while (setTemps == 0);
			break;
			
			case SET_NIP:
			_delay_ms(200);
			do{
				if(refreshClavier)
				{
					refreshClavier = 0;
					key = lireClavier();
					
					if((key)&&(!lastKey))
					{
						lcdSetPos(cntPassword,1);
						essaiUser[cntPassword++] = key;
						lcdPutc(key);
						if(cntPassword == 4)
						{
							for(uint8_t index = 0; index<4; index++)
							{
								password[index] = essaiUser[index];
							}
							cntMenu = 4;
							modeActuel = DEFAULT;
							modeTondeuse = 5;
							passwordValid = 1;
							cntPassword = 0;
							lcdClearScreen();
							lcdSetPos(0,0);
							lcdPuts("Set password OK");
							lcdSetPos(0,1);
							lcdPuts(msgTime);
						}
					}
					lastKey = key;
				}
			}while(!passwordValid);
			break;
			
			case DEFAULT:
			//case pour la sortie de menus mot de passe et set time
			break;
		}
		
		if(modeTondeuse!=1) //condition pour l'envoi Bluetooth, gestion différente pour le mode Prog de la tondeuse (modeTondeuse!=1).
		{
			if(refreshEnvoi)
			{
				float latEnvoi = lat;
				float longEnvoi = lon;
				sprintf(msgEnvoi,"S;%.2f;%d;%d;%d;%f;%f;%d;",niveauBatterie,flagPluie,etatCoupe,flagLidar,latEnvoi,longEnvoi,modeTondeuse);
				usart2SendString(msgEnvoi);
				refreshEnvoi = 0;
			}
		}
	}
	
	// 		if(refreshDist)
	// 		{
	// 			refreshDist = 0;
	// 			lcdClearScreen();
	// 			lcdSetPos(0,0);
	// 			sprintf(msgEncoD,"%d",encodeurDCnt);
	// 			lcdPuts(msgEncoD);
	// 			lcdSetPos(0,1);
	// 			sprintf(msgEncoG,"%d",encodeurGCnt);
	// 			lcdPuts(msgEncoG);
	// 		}
	
}

/*Section vecteurs d'interruption*/

/**
* @brief À chaque interruption du timer 0 le programme execute DixiemeDeSec++ comme ça on verifie l'etat des boutons a chaque 100ms. De plus, cette interruption sert à rafraîchir l'affichage LCD et l'envoi bluetooth.
*/
ISR(TIMER0_COMPA_vect)//Quand l'interruption globale est appeller le programme vient executer le vecteur Comparatif.
{
	cntDixiemeDeSec++;
	cntDixiemeDeSecLCD++;
	refreshAuto = 1; //test pour le rafraîchissement de la lecture du compas dans le mode automatique
	if(cntDixiemeDeSec >= 100)
	{
		cntDixiemeDeSec -= 100;
		refreshManuel = 1;
		refreshClavier = 1;
	}
	if(cntDixiemeDeSecLCD >= 300)
	{
		cntDixiemeDeSecLCD -= 300;
		refreshLCD = 1;
		refreshEnvoi = 1;
	}
}
/**
* @brief À chaque interruption externe déclencher par les encodeurs optiques, ce vecteur d'interruption est appelé et il incremente les variables des encodeurs pour ensuite calculer la distance que le robot a parcourue en effectuant un calcul à l'aide du diamètre des ses roues dans le mode automatique.
*/
ISR(PCINT1_vect)//A chaque detection du capteur une interruption se fait sur PCINT1 et PCINT2 qui correspondent aux pins PINJ3 et PINB5 du atmega2560
{
	if(derivation>=-5 && derivation<=5)
	{
		if(!(PINJ&(1<<3))&&(encodeurEtatPrecedent & (1<<3)))//si l'etat du capteur est low et qu'il y eu une detection comme etat precedant
		{
			encodeurDCnt++;//le compteur d'enchoches s'incremente
			refreshDist = 1;//on active le refresh l'affichache sur le LCD
		}
		encodeurEtatPrecedent = PINJ&(1<<3);
		if(!(PINJ&(1<<4))&&(encodeurEtatPrecedent2 & (1<<4)))//si l'etat du capteur est low et qu'il y eu une détection comme etat precedant
		{
			encodeurGCnt++;//le compteur d'enchoches s'incremente
			refreshDist = 1;//on active le refresh de l'affichage sur la console pour le débogage
		}
		encodeurEtatPrecedent2 = PINJ&(1<<4);
	}
}

/**
* @brief À chaque interruption causé par la lecture de l'ADC, ce vecteur d'interruption est appelé. Ensuite, on vérifie l'origine de réception de cette valeur (Batterie, joystick ou capteur de pluie) et on récupere cette valeur dans une variable en fonction de son origine.
*/

ISR(ADC_vect)
{
	receptionAdc=ADC; //récupère la valeur de tension lue par le ADC sur un total de 0 à 1024 (10 bits)
	triAdc=((ADCSRB&&0x8)<<5)|(ADMUX&0x1F); //récupere le registre de lecture ADC affecté
	switch(triAdc)
	{
		case 0x0: //case capteur de pluie
		if(receptionAdc<=512) //après la calibration le capteur détecte la pluie à 512 sur 1024
		{
			lcdSetPos(8,1);
			lcdPuts("pluie");
			flagPluie = 1;
		}
		else
		{
			flagPluie = 0;
		}
		ADMUX=0x41; //changer le registe ADC pour la prochaine lecture
		break;

		case 0x1:
		//joysticX=receptionAdc-512;
		ADMUX=0x42; //changer le registe ADC pour la prochaine lecture
		break;

		case 0x2:
		//joysticY=receptionAdc-512;
		ADMUX=0x40; //changer le registe ADC pour la prochaine lecture
		ADCSRB|=(1<<MUX5);
		break;

		case 0x20:
		niveauBatterie=((float)receptionAdc/1023.0)*100.0;
		niveauBatterie=(niveauBatterie-75.0)*4.0;
		ADMUX=0x40; //changer le registe ADC pour la prochaine lecture
		ADCSRB&=~(1<<MUX5);
		break;

		default:
		break;
	}
	ADCSRA|=(1<<ADSC);
}

/*Définition des fonctions*/

/**
* @brief Fonction qui intialise les trois ADC du atmega2560 qui servent à mesurer la tension de la batterie, la tension à la sortie du capteur de pluie en fonction de son état et la tension de l'axe Y et l'axe X du joystick analogique pour le controle manuel du robot.
*/

void adcInit()
{
	ADMUX|=(1<<REFS0);//10 bit et compare avec vcc
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2);//divise le clock par 32 et active l'adc

	DIDR0|=0x06;//mets les sortie en adc 1 et adc 2
	DIDR2|=0x01;//mets la pin en adc8
	sei();
}

/**
* @brief Cette fonction initialise les registres PWM du atmega2560 pour le contrôle des trois moteurs du robot tondeuse. Tout les PWM pour les moteurs sont intialisés à une fréquence de 15.6 kHz en mode FAST PWM avec un top de 1024.
*/

void moteurInit()
{
	DDRH|=(1<<4)|(1<<5);//moteur arrière 2 gauche

	TCCR4A=0b10101011;
	TCCR4B=0b00011001;
	OCR4A=1024;//top
	OCR4B=70;
	OCR4C=0;

	DDRL|=(1<<4)|(1<<5);//moteur arrière 1 droite
	TCCR5A=0b10101011;
	TCCR5B=0b00011001;
	OCR5A=1024;//top
	OCR5B=90;
	OCR5C=0;
	
	DDRB|=(1<<6); //moteur pour la lame
	TCCR1A=0b10101011;
	TCCR1B=0b00011001;
	OCR1A=1024;//top
	OCR1B=90;
	OCR1C=0;
}

/**
* @brief Cette fonction vérifie l'état des 4 pins du clavier matriciel et elle retourne le caractère correpondant à la touche appuyé.
*
* @return De type char correspond au caractère ASCII de chaque touche.
*/

char lireClavier()
{
	char digit = 0;
	
	if(BT1()) //touche 1
	{
		digit = '1';
	}
	else if(BT2()) //touche 2
	{
		digit = '2';
	}
	else if(BT3()) //touche 3
	{
		digit = '3';
	}
	else if(BT4()) //touche 4
	{
		digit = '4';
	}
	return digit;
}

/**
* @brief Cette fonction utilise la valeur de l'axe X et Y du joystick, ensuite elle gère le sens de rotation et la vitesse des deux moteurs pour les roues. Elle utilise les valeurs du joystick analogique connecté au ADC et la valeur recue du joytstick sur la manette de PS4. On vérifie le cadran en X et Y de la valeur recue et ensuite on effectue les calculs nécessaire pour limiter les registres PWM des moteurs entre 0 et 1024 et on inverse le sens de rotation aussi en fonction des ces cadrans.
*/

void controleManuel()
{
	if(refreshManuel)
	{
		refreshManuel = 0;
		if(joysticY<0)
		{
			joysticY*=-1;

			if(joysticX>0)
			{
				vitesseMoteurG=joysticY-joysticX;
				vitesseMoteurD=joysticY;
			}
			else
			{
				vitesseMoteurD=joysticY+joysticX;
				vitesseMoteurG=joysticY;
			}

			if(vitesseMoteurD<0)
			vitesseMoteurD=0;
			if(vitesseMoteurG<0)
			vitesseMoteurG=0;
			OCR4B=0;
			OCR5B=(VIT_MIN+vitesseMoteurG);
			OCR4C=(VIT_MIN+vitesseMoteurD);
			OCR5C=0;
		}
		else
		{
			if(joysticX>0)
			{
				vitesseMoteurG=joysticY-joysticX;
				vitesseMoteurD=joysticY;
			}
			else
			{
				vitesseMoteurD=joysticY+joysticX;
				vitesseMoteurG=joysticY;
			}
			if(vitesseMoteurD<0)
			vitesseMoteurD=0;
			if(vitesseMoteurG<0)
			vitesseMoteurG=0;
			OCR4B=(VIT_MIN+vitesseMoteurD);
			OCR5B=0;
			OCR4C=0;
			OCR5C=(VIT_MIN+vitesseMoteurG);
		}
	}
}

/**
* @brief Cette fonction possède l'algorithme de type PID qui ajuste la direction du robot pour le faire avancer toujours vers l'angle de destination. De plus, cette fonction permet de reajuster le robot vers la bonne direction même si une personne le déplace pendant son fonctionnement. On récupère le deltaDerive et on utilise un algorithme à fenêtre glissante pour attenuer le bruit au niveau de la lecture de l'angle en on affecte les registres PWM des moteurs à l'aide de la variable valPID pour permettre au robot d'avancer d'une façon stable et continue.
*
* @param angleDest de type uint16_t recoit l'angle calculée à atteindre par le robot.
*
* @param angleActuel de type uint16_t correspond à l'angle actuel du robot envoyée par le module QMC5883L.
*/

void ajusterDrirection(uint16_t angleDest, uint16_t angleActuel)//deltadervive=angleActuel-angleDest peut-être mettre juste une fonction pour tourner et ajuster
{
	//si pas de derive avance en ligne droite avec les valeurs de pid vitesseMoteur max =512

	//ocr1B controle de la lame 1024 actif 0 inactif
	//if(OCR1B!=1024)
	//OCR1B=1024;
	static uint8_t cote;
	int deltaDerive=angleDest-angleActuel;
	uint16_t over=angleActuel+180;
	//quel côté affecter
	uint16_t angle=0;

	if(deltaDerive<0)
	{
		deltaDerive=-1;
		angle=deltaDerive+360;
	}
	else
	angle=deltaDerive;
	
	if(over<angleDest)
	{
		deltaDerive-=360;
		deltaDerive*=-1;
	}

	if(angle<180)
	{// tourne à gauche
		cote=0;
	}
	else
	{//tourne à droite
		cote=1;
	}
	derivation=deltaDerive;

	//fenêtre
	for (int i = (TAILLE_FENETRE-1); i > 0; i--) {
		window[i] = window[i - 1];
	}
	window[0] = deltaDerive;
	//ki
	valSommeErreur = 0;
	if(deltaDerive>=-15 &&deltaDerive<=15)
	{
		for (int i = 0; i < TAILLE_FENETRE; i++) {
			valSommeErreur += window[i];
		}
	}

	//kd
	variation = window[0] - window[TAILLE_FENETRE-1];

	valPid=KP*deltaDerive+KI*valSommeErreur+KD*variation;
	//vérification
	if(valPid>412)
	valPid=412;
	if(valPid<0)
	valPid=0;
	//application du pid
	if(cote==0)
	{//droite
		OCR4B=0;
		OCR5B=(VIT_MIN+450-valPid);
		OCR4C=(VIT_MIN+512);
		OCR5C=0;
	}
	else
	{//gauche
		OCR4B=0;
		OCR5B=(VIT_MIN+450);
		OCR4C=(VIT_MIN+512-valPid);
		OCR5C=0;
	}
	//erreurPrec=deltaDerive;
}

/**
* @brief Cette fonction sert à orienter le robot vers l'angle de destination du point à atteindre. On effectue la différence entre l'angle de destination calculée et l'angle actuel du robot envoyé par le module QMC5883L et en fonction du cadran qu'on se retrouve on fait tourner le robot vers la direction la plus proche de l'angle à atteindre. Une fois que le robot a atteint l'angle de destination il arrête à +- 2 degrées de précision.
*
* @param angleDest est de type uint16_t et elle recoit l'angle de destination calculée entre deux coordonées GPS.
*
* @param angleActuel est de type uint16_t et elle recoit l'angle actuel du robot envoyée par le module compas magnétique QMC5883L.
*
* @return est de type uint8_t et retourne 0 si le robot n'a pas encore atteint la bonne orientation, sinon 1 s'il a atteint l'angle de destination.
*/

uint8_t tourner(uint16_t angleDest, uint16_t angleActuel)
{
	//if(OCR1B!=1024)//active la lame, enlevé pour les tests de la fonction
	//OCR1B=1024;
	//appeler lorsque l'on arrive à un waypoint
	int deltaDerive=angleDest-angleActuel;
	static uint8_t direction;
	uint16_t angle=0;

	if(deltaDerive<0)
	angle=deltaDerive+360;
	else
	angle=deltaDerive;

	if(angle<180)
	{
		direction=0;//doit corriger vers la droite

	}
	else
	{
		direction=1;//doit corriger vers la gauche

	}


	if(direction)
	{
		if(deltaDerive<2 && deltaDerive>-2)//tourner vers gauche
		{

			return 1;
		}
		else
		{
			OCR4B=(VIT_MIN+512);//on vitesse à déterminer VIT_MIN+vitesseMoteurD;
			OCR5B=(VIT_MIN+450);//on 69 diff
			OCR4C=0;
			OCR5C=0;
		}
	}
	else
	{
		if(deltaDerive>-2 && deltaDerive<2)//tourner vers droite
		{
			return 1;
		}
		else
		{
			OCR4B=0;
			OCR5B=0;
			OCR4C=(VIT_MIN+512);//moteurs inégal 512
			OCR5C=(VIT_MIN+450);//443

		}
	}
	return 0;
}

/**
* @brief Cette fonction active les deux moteurs du robot et le fait tourner sur lui-même. Cette fonction est essentielle au démarrage du robot pour bien calibrer le compas digital.
*/

void effectuerCalibration()
{
	while(!calibration())//effectuer tant que la calibration du module
	{
		OCR4B=0;
		OCR5B=0;
		OCR4C=(VIT_MIN+512);//moteurs inégal
		OCR5C=(VIT_MIN+450);//moteur droit
	}
	OCR4B=0;
	OCR5B=0;
	OCR4C=0;
	OCR5C=0;
}

/**
* @brief Cette fonction initialise la communication I2C entre le atmega2560 et le compas QMC5883L. Ensuite, elle applique la configuration du module QMC5883L, comme sa vitesse de communication son nombre de samples et le mode de communication master->slave.
*/

void initBoussole()
{
	i2cInit();
	i2cWriteByte(QMC5883_ADDRESS_MAG,0x0B,0x01);
	setMode(0x01,0x0C,0x10,0X00);
}


/**
* @brief Cette fonction est identique à l'algorithme de contrôle automatique du robot. En premier, elle recupère le positionnement (longitude, latitude) du robot à partir du module GPS si la précision 2D de la mesure est inférieure à 5 cm. À l'aide de ces données en combinaison avec les coordonnées de destination envoyées par le serveur local, on calcule la distance à parcourir et l'angle de destination. Ensuite, on effectue le case tourner pour orienter le robot vers le bon angle de 0 à 360 à l'aide du module QMC5883L. Une fois que le robot est dans la bonne orientation, il avance vers le point de destination et il garde la bonne direction à l'aide de l'algorithme PID dans la fonction ajusterDirection. La distance parcourue par le robot est calculée à l'aide de la moyenne des deux encodeurs optiques. Cette fonction retourne le robot à la coordonné 0 du tableau de points, qui correspond au point d'origine (retour à la maison).
*
* @param latitudeMaison est de type float et cette variable recoit la latitude de la coordonée maison.
*
* @param longitudeMaison est de type float et cette variable recoit la longitude de la coordonée maison.
*/

void retourMaison(float latitudeMaison,float longitudeMaison)
{
	etatCoupe=4;
	switch(etapeModeAuto)
	{
		case ORIENTER:
		dataGps = usartGpsRemRxData();
		if(parseRxUbxNavPvt(dataGps))
		{
			if(getNavPvtFixType()>=3)//3d fix
			{
				lat=getNavPvtLat();
				lon=getNavPvtLon();
				horizontalAcc=getNavPvtVacc();//en mm
				
				if(horizontalAcc<=40 )//check precision 2d,horizon, et 3d,vertical,
				{
					//pour test tabLatMem[indexDestination]
					switch (orientation)
					{
						case DISTANCE:
						cli();
						distanceDest=distance_entre_point(lat,lon,latitudeMaison,longitudeMaison);//lat actuel, lon actuel, lat dest,lon dest
						sei();
						orientation=ANGLE;
						break;

						case ANGLE:
						cli();
						angleDest=course(lat, lon,latitudeMaison,longitudeMaison);//index=1 depart
						sei();
						orientation=FINI;
						break;

						case FINI:
						etapeModeAuto=TOURNER;
						orientation=DISTANCE;
						break;
					}
				}
			}
			break;
			case TOURNER:
			if(tourner(angleDest,angleActuel))
			{
				encodeurDCnt=0;
				encodeurGCnt=0;
				etapeModeAuto=AJUSTER;
			}


			break;
			case AJUSTER:
			ajusterDrirection(angleDest,angleActuel);
			distanceMoyParcouru= ((encodeurDCnt+encodeurGCnt)/2)*3.45; //calcul pour effectuer la moyenne de la valeur des deux encodeurs
			if(distanceMoyParcouru>=distanceDest)
			{
				//remettre alerte pluie ou batterie à 0
				flagRetourMaison=0;
				if(nbPointGpsDispo>1)
				nbPointGpsDispo=1;
				OCR4B=0;
				OCR5B=0;
				OCR4C=0;
				OCR5C=0;
			}
			break;
		}
	}
}