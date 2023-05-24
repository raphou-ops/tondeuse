/*
* Programme principal robot tondeuse.c
*
* Created: 2023-03-31 3:49:12 PM
* Author : adino
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
//long tabLatMem[80];
//long tabLonMem[80];

uint16_t nbPointGpsDispo=3;//mettre 0  pour les donn/es js
enum EtapeModeAuto{ORIENTER,TOURNER,AJUSTER};
enum EtapeModeAuto etapeModeAuto=ORIENTER;
uint16_t indexDestination=0; //0 si jamais le traitement bogue
float tabLatMem[2]={45.64461,45.64454};
float tabLonMem[2]={-73.84276,-73.84287};

float tabLatMemJs[100];
float tabLonMemJs[100];
//long tabLonMem[100];
//long tabLatMem[100];

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

/*Etats et pourcentage de la batterie*/

volatile float niveauBatterie=0;

/*Variables pour pid*/
#define KP 125
#define KI 0.1
#define KD 3

//#define SOMME_MAX 10
#define TAILLE_FENETRE 8
//int sommeErreur[SOMME_MAX];
int window[TAILLE_FENETRE];
int erreur=0;//déjà calculé, proportionnel
int valSommeErreur=0;// intégrale
int erreurPrec=0;
int variation=0;//dérivé
int valPid=0;
uint8_t cntSommeErreur=0;

/*Variables timer0 refresh du systeme*/

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


float PDOP = 0;
/*Declaration des fonctions*/

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

int main(void)
{
	indexDestination=0;
	lcdInit();
	
	initBoussole();
	_delay_ms(100);
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
	
	static uint8_t executed = 1;
	if (executed) {
		effectuerCalibration();
		executed = 0; // Set the flag to indicate execution
	}
	
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
			controleManuel();
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
				DDRB|=(1<<6);
			}
			else
			{
				OCR1B=90;
				DDRB &= ~(1<<6);
			}
			toucheManette = getBoutonO();
			if((toucheManette == 0x31)&&(flagCalibration!=0x31))
			{
				effectuerCalibration();
			}
			flagCalibration = toucheManette;
			
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
			
			
			//lcdSetPos(0,0);
			//sprintf(msgTime, "%f %f ",angleDest, distanceDest);
			//lcdPuts(msgTime);
			lcdSetPos(0,1);
			sprintf(msgTime,"%f %d %f",distanceMoyParcouru,indexDestination,angleActuel);
			//sprintf(msgTime,"%f",angleActuel);
			lcdPuts(msgTime);
			
			
			//getNavPvtTime(msgTime);
			if(nbPointGpsDispo>1)
			{
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
							//lat=45.64462;
							//lon=-73.84251;
							horizontalAcc=getNavPvtVacc();//en mm
							//verticalAcc=getNavPvtHacc();//en mm
							if(horizontalAcc<=50)//check precision 2d,horizontal
							{
								//indexIndentation pour test tabLatMem[indexDestination]
								switch (orientation)
								{
									case DISTANCE:
									distanceDest=distance_entre_point(lat,lon,tabLatMemJs[indexDestination],tabLonMemJs[indexDestination]);//lat actuel, lon actuel, lat dest,lon dest
									orientation=ANGLE;
									break;
									
									case ANGLE:
									angleDest=course(lat, lon,tabLatMemJs[indexDestination],tabLonMemJs[indexDestination]);//index=1 depart
									//angleDest=course(39.09991, -94.58121,38.62709,-90.20020);//index=1 depart
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
					i2cReadBytes(QMC5883_ADDRESS_MAG,0x0,0x06);//lis registre x,y,z
					angleActuel=getAzimuth();
					_delay_ms(10);
					if(tourner(angleDest,angleActuel))
					{
						encodeurDCnt=0;
						encodeurGCnt=0;
						etapeModeAuto=AJUSTER;
					}
					break;
					case AJUSTER:
					i2cReadBytes(QMC5883_ADDRESS_MAG,0x0,0x06);//lis registre x,y,z
					angleActuel=getAzimuth();
					
					_delay_ms(10);
					
					ajusterDrirection(angleDest,angleActuel);
					//_delay_ms(10);
					
					if(distanceMoyParcouru>=distanceDest)
					{
						etapeModeAuto=ORIENTER;
						//orientation=DISTANCE;
						indexDestination++;
						nbPointGpsDispo--;
					}
					else
					{
						distanceMoyParcouru = (((float)(encodeurDCnt+encodeurGCnt))/2.0);
						distanceMoyParcouru = distanceMoyParcouru * 3.55;
					}
					
					break;
				}
			}
			else
			{
				retourMaison(tabLatMemJs[0],tabLonMemJs[0]);
				OCR4B=0;
				OCR5B=0;
				OCR4C=0;
				OCR5C=0;
			}
			//_delay_ms(200);
			//getNavPvtTime(msgTime);

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
		
		if(modeTondeuse!=1)
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
* @brief //A chaque interruption du timer 0 le programme execute DixiemeDeSec++ comme ca on verifie l'etat des boutons a chaque 100ms.
*/
ISR(TIMER0_COMPA_vect)//Quand l'interruption globale est appeller le programme vient executer le vecteur Comparatif.
{
	cntDixiemeDeSec++;
	cntDixiemeDeSecLCD++;
	refreshAuto = 1;
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
ISR(PCINT1_vect)//A chaque detection du capteur une interruption se fait sur PCINT4 et PCINT5 qui correspond a la PINB4 et PINB5 du atmega32u4
{
	if(!(PINJ&(1<<3))&&(encodeurEtatPrecedent & (1<<3)))//si l'etat du capteur est low et qu'il y eu une detection comme etat precedant
	{
		encodeurDCnt++;//le compteur d'enchoches s'incremente
		refreshDist = 1;//on active le refresh l'affichache sur le LCD
	}
	encodeurEtatPrecedent = PINJ&(1<<3);
	if(!(PINJ&(1<<4))&&(encodeurEtatPrecedent2 & (1<<4)))//si l'etat du capteur est low et qu'il y eu une detection comme etat precedant
	{
		encodeurGCnt++;//le compteur d'enchoches s'incremente
		refreshDist = 1;//on active le refresh l'affichache sur la console
	}
	encodeurEtatPrecedent2 = PINJ&(1<<4);
}

ISR(ADC_vect)
{
	receptionAdc=ADC;
	triAdc=((ADCSRB&&0x8)<<5)|(ADMUX&0x1F);
	switch(triAdc)
	{
		case 0x0:
		if(receptionAdc<=512)
		{
			lcdSetPos(8,1);
			lcdPuts("pluie");
			flagPluie = 1;
		}
		else
		{
			flagPluie = 0;
		}
		ADMUX=0x41;
		break;

		case 0x1:
		//joysticX=receptionAdc-512;
		ADMUX=0x42;
		break;

		case 0x2:
		//joysticY=receptionAdc-512;
		ADMUX=0x40;
		ADCSRB|=(1<<MUX5);
		break;

		case 0x20:
		niveauBatterie=((float)receptionAdc/1023.0)*100.0;
		niveauBatterie=(niveauBatterie-75.0)*4.0;
		ADMUX=0x40;
		ADCSRB&=~(1<<MUX5);
		break;

		default:
		break;
	}
	ADCSRA|=(1<<ADSC);
}

/*Definition des fonctions*/

void adcInit()
{
	ADMUX|=(1<<REFS0);//10 bit et compare avec vcc
	ADCSRA|=(1<<ADEN)|(1<<ADIE)|(1<<ADPS2);//divise le clock par 32 et active l'adc

	DIDR0|=0x06;//mets les sortie en adc 1 et adc 2
	DIDR2|=0x01;//mets la pin en adc8
	sei();
}
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
char lireClavier()
{
	char digit = 0;
	
	if(BT1())
	{
		digit = '1';
	}
	else if(BT2())
	{
		digit = '2';
	}
	else if(BT3())
	{
		digit = '3';
	}
	else if(BT4())
	{
		digit = '4';
	}
	return digit;
}

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
void ajusterDrirection(uint16_t angleDest, uint16_t angleActuel)//deltadervive=angleActuel-angleDest peut-être mettre juste une fonction pour tourner et ajuster
{
	//si pas de derive avance en ligne droite avec les valeurs de pid vitesseMoteur max =512

	//ocr1B controle de la lame 1024 actif 0 inactif
	//if(OCR1B!=1024)
	//OCR1B=1024;
	uint8_t cote=0;
	int deltaDerive=angleDest-angleActuel;
	//variation=erreur-erreurPrec;
	//quel côté affecter
	uint16_t angle=0;

	if(deltaDerive<0)
	{
		deltaDerive=-1;
		angle=deltaDerive+360;
	}
	else
	angle=deltaDerive;

	if(angle<180)
	{// tourne à droite
		cote=0;
	}
	else
	{//tourne à gauche
		cote=1;
	}


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
uint8_t tourner(uint16_t angleDest, uint16_t angleActuel)
{
	//if(OCR1B!=1024)//active la lame
	//OCR1B=1024;
	//appeler lorsque l'on arrive à un waypoints
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
void effectuerCalibration()
{
	while(!calibration())
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
void initBoussole()
{
	i2cInit();
	i2cWriteByte(QMC5883_ADDRESS_MAG,0x0B,0x01);
	setMode(0x01,0x0C,0x10,0X00);
}
void retourMaison(float latitudeMaison,float longitudeMaison)
{
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
				verticalAcc=getNavPvtHacc();//en mm
				if(horizontalAcc<=40 && verticalAcc<=40)//check precision 2d,horizon, et 3d,vertical,
				{
					//pour test tabLatMem[indexDestination]
					switch (orientation)
					{
						case DISTANCE:
						distanceDest=distance_entre_point(lat,lon,latitudeMaison,longitudeMaison);//lat actuel, lon actuel, lat dest,lon dest
						orientation=ANGLE;
						break;

						case ANGLE:
						angleDest=course(lat, lon,latitudeMaison,longitudeMaison);//index=1 depart
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
			distanceMoyParcouru= ((encodeurDCnt+encodeurGCnt)/2)*3.45;
			if(distanceMoyParcouru>=distanceDest)
			{
				//remettre alerte pluie ou batterie à 0
				OCR4B=0;
				OCR5B=0;
				OCR4C=0;
				OCR5C=0;
			}
			break;
		}
	}
}