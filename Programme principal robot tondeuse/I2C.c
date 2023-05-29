/**
* @file I2C.c
* @author Adrian Catuneanu et Raphaël Tazbaz
* @brief Cette bibliothèque possède la définition des fonctions pour la configuration et la gestion de la communication I2C entre le atmega2560 et le compas magnétique QMC5883L. De plus, cette librairie possède aussi des fonctions pour le calibrage du QMC5883L et la récuperation de données, comme l'azimuth, l'angle en X, l'angle en Y et l'angle en Z. Ces données sont récuperés à l'aide de calculs mathématiques de la librairie MATH.h.
* @version 1.0
* @date 2023-04-24
*
* @copyright Copyright (c) 2023
*/


/*
FROM QST QMC5883L Datasheet [https://nettigo.pl/attachments/440]
-----------------------------------------------
MODE CONTROL (MODE)
Standby			0x00
Continuous		0x01

OUTPUT DATA RATE (ODR)
10Hz        	0x00
50Hz        	0x04
100Hz       	0x08
200Hz       	0x0C

FULL SCALE (RNG)
2G          	0x00
8G          	0x10

OVER SAMPLE RATIO (OSR)
512         	0x00
256         	0x40
128         	0x80
64          	0xC0

*/

#include "I2C.h"
#include <avr/io.h>
#include <avr/interrupt.h>

//smoothing

uint8_t _smoothUse = 0;
uint8_t _smoothSteps = 5;
uint8_t _smoothAdvanced = 0;
int _vHistory[10][3];
int _vScan = 0;
long _vTotals[3] = {0,0,0};
int _vSmooth[3] = {0,0,0};
//calibration
#define NB_SAMPLE 20000
int _vCalibration[3][2];
int calibrationData[3][2]={{32767,-32768},{32767,-32768},{32767,-32768}};
int _vCalibrated[3];
uint8_t _calibrationUse = 0;
uint16_t cntSample=0;

//calcul direction

float _angle=0;
char pointCardinal[3];
int rawValue[3] = {0,0,0};

//communication i2c

volatile uint8_t _i2cErrorCode=0;
volatile uint8_t _i2cBusy=0;
volatile uint8_t _i2cDataCnt=0;
volatile uint8_t _i2cRxDataCnt=0;
volatile uint8_t statePointeur=0;
uint8_t _i2cRegistre=0;
uint8_t _i2cAdd=0;
uint8_t _i2cModeRead=1;
uint8_t _i2cData[I2C_DTA_SIZE_MAX];
uint8_t _i2cDataSize=0;

/**
* @brief Cette fonction est un getter qui récupère la valeur de la variation de l'angle en X du compas QMC5883L.
*
* @return de type int retourne de la variation de l'angle en X du compas QMC5883L.
*/

int getX()
{
	if ( _smoothUse )
	return _vSmooth[0];
	if ( _calibrationUse )
	return _vCalibrated[0];
	
	return rawValue[0];
}

/**
* @brief Cette fonction est un getter qui récupère la valeur de la variation de l'angle en Y du compas QMC5883L.
*
* @return de type int retourne de la variation de l'angle en Y du compas QMC5883L.
*/

int getY()
{
	if ( _smoothUse )
	return _vSmooth[1];
	if ( _calibrationUse )
	return _vCalibrated[1];
	
	return rawValue[1];
}

/**
* @brief Cette fonction est un getter qui récupère la valeur de la variation de l'angle en Z du compas QMC5883L.
*
* @return de type int retourne de la variation de l'angle en Z du compas QMC5883L.
*/

int getZ()
{
	if ( _smoothUse )
	return _vSmooth[2];
	if ( _calibrationUse )
	return _vCalibrated[2];
	return rawValue[2];
}

uint8_t i2cIsBusy()
{
	return _i2cBusy;
}

uint8_t i2cDataAvailable()
{
	return _i2cRxDataCnt;
}

uint8_t i2cGetData()
{
	if(_i2cRxDataCnt)
	return _i2cData[(_i2cRxDataCnt--)-1];
	return 0;
}

uint8_t i2cGetErrorCode()
{
	return _i2cErrorCode;
}

void i2cWriteByte(uint8_t add,uint8_t reg,uint8_t data)
{
	while(_i2cBusy || (TWCR &(1<<TWSTO)));
	
	_i2cErrorCode=0;
	_i2cAdd=add;
	_i2cModeRead=0;
	_i2cData[0]=data;
	_i2cDataSize=1;
	_i2cDataCnt=0;
	_i2cBusy=1;
	
	_i2cRegistre=reg;
	
	
	
	TWCR|=(1<<TWINT)|(1<<TWEN)|(1<<TWSTA)|(1<<TWIE);
}

/**
* @brief Cette fonction sert à configurer le module compas QMC5883L selon les paramètres désirés de l'utilisateur.
*
* @param mode de type uint8_t recoit le mode de fonctionnement du compas QMC5883L.
*
* @param odr de type uint8_t recoit le "OUTPUT DATA RATE" désiré.
*
* @param rng de type uint8_t recoit le facteur "FULL SCALE" désiré.
*
* @param osr de type uint8_t recoit le "OVER SAMPLE RATIO" désiré.
*/

void setMode(uint8_t mode, uint8_t odr, uint8_t rng, uint8_t osr)
{
	i2cWriteByte(QMC5883_ADDRESS_MAG,0x09,mode|odr|rng|osr);
}

/**
* @brief Cette fonction sert à réinitialiser la communication I2C entre le atmega2560 et le QMC5883L.
*/

void setReset()
{
	i2cWriteByte(QMC5883_ADDRESS_MAG,0x0A,0x80);
}

/**
* @brief Fonction qui sert a pointer sur le registre I2C désiré du QMC5883L.
*
* @param add de type uint8_t correspond à l'adresse I2C désiré.
*
* @param reg de type uint8_t correspond au registre I2C désiré.
*/

void i2cSetPointeur(uint8_t add,uint8_t reg)
{
	while(_i2cBusy || (TWCR &(1<<TWSTO)));
	_i2cAdd=add;
	_i2cErrorCode=0;
	_i2cModeRead=0;
	_i2cRxDataCnt=0;
	_i2cDataSize=1;
	_i2cBusy=1;
	statePointeur=1;
	_i2cRegistre=reg;
	
	TWCR|=(1<<TWINT)|(1<<TWEN)|(1<<TWSTA)|(1<<TWIE);
}

/**
* @brief Fonction qui sert a lire les registres à l'adresse I2C désiré du module QMC5883L.
*
* @param add de type uint8_t correspond à l'adresse I2C désiré.
*
* @param reg de type uint8_t correspond au registre I2C désiré.
*
* @param size de type uint8_t correspond au nombre de registres I2C à lire.
*/

void i2cReadBytes(uint8_t add,uint8_t reg,uint8_t size)
{
	i2cSetPointeur(add,reg);
	while(_i2cBusy || (TWCR &(1<<TWSTO)));
	
	_i2cErrorCode=0;
	_i2cAdd=add |1;
	_i2cModeRead=1;
	_i2cRxDataCnt=0;
	_i2cDataSize=size;
	_i2cBusy=1;
	
	
	TWCR|=(1<<TWINT)|(1<<TWEN)|(1<<TWSTA)|(1<<TWIE);
}

/**
* @brief Vider le buffer de reception du registre I2C.
*/
void i2cClearDataAvailable()
{
	_i2cRxDataCnt=0;
}

void i2cInit()
{
	TWBR=72;//100kbs
	TWCR|=(1<<TWEN);
	//i2cWriteByte(QMC5883_ADDRESS_MAG,0x0B,0x01);
	//setMode(0x01,0x0C,0x10,0X00);
	sei();
}

/**
* @brief Cette fonction sert à récuperer l'angle d'orientation entre 0 et 360 degrés en fonction des valeurs en X et en Y du compas QMC5883L.
*
* @return de type float retourne l'angle d'orientation entre 0 et 360 degrés.
*/

float getAzimuth()
{
	_angle=0;
	_angle = atan2( getY(),getX()) * 180.0 / M_PI;
	return _angle < 0 ? 360 + _angle : _angle;;
}

/**
SET CALIBRATION
Set calibration values for more accurate readings

@author Claus Näveke - TheNitek [https://github.com/TheNitek]

@since v1.1.0
**/
void setCalibration(int x_min, int x_max, int y_min, int y_max, int z_min, int z_max)
{
	_calibrationUse = 1;

	_vCalibration[0][0] = x_min;
	_vCalibration[0][1] = x_max;
	_vCalibration[1][0] = y_min;
	_vCalibration[1][1] = y_max;
	_vCalibration[2][0] = z_min;
	_vCalibration[2][1] = z_max;
}

/**
* @brief Fonction qui converti les valeurs en degrées à une valeur en radians.
*
* @param deg de type float recoit la valeur en degrées à convertir
*
* @return de type float retourne la valeur convertie en radians.
*/

float deg_toRad(float deg)
{
	return deg*M_PI/180.0;
}

/**
* @brief Cette fonction calcule la distance entre le point de départ et le point de destination. Les deux points sont définis pas la longitude et la latitude de chacun.
*
* @param lat1 de type float correspond à la latitude de départ
*
* @param lon1 de type float correspond à la longitude de départ
*
* @param lat2 de type float correspond à la latitude de destination
*
* @param lon2 de type float correspond à la longitude de destination
*
* @return de type float retourne la distance à parcourir pour se rendre au point de destination
*/

float distance_entre_point(float lat1,float lon1,float lat2,float lon2)
{
	float dlat = deg_toRad(lat2 - lat1);
	float dlon = deg_toRad(lon2 - lon1);
	float a = sin(dlat/2) * sin(dlat/2) + cos(deg_toRad(lat1)) * cos(deg_toRad(lat2)) * sin(dlon/2) * sin(dlon/2);
	float c = 2 * atan2(sqrt(a), sqrt(1 - a));
	float distance = RAYON_TERRE * c;
	
	return distance;
}

/**
* @brief Fonction qui converti les valeurs en radians à une valeur en degrées.
*
* @param deg de type float recoit la valeur en radians à convertir
*
* @return de type float retourne la valeur convertie en degrées.
*/

float toDegrees(float radians)
{
	return (radians * 180.0) / M_PI;
}

/**
* @brief Cette fonction calcule la l'angle entre le point de départ et le point de destination. Les deux points sont définis pas la longitude et la latitude de chacun.
*
* @param lat1 de type float correspond à la latitude de départ
*
* @param lon1 de type float correspond à la longitude de départ
*
* @param lat2 de type float correspond à la latitude de destination
*
* @param lon2 de type float correspond à la longitude de destination
*
* @return de type float retourne l'angle à s'orienter pour se rendre au point de destination
*/

float course(float lat1, float lon1, float lat2, float lon2)
{
	float phi1 = deg_toRad(lat1);
	float phi2 = deg_toRad(lat2);
	float delta_lambda = (lon2 - lon1)*(M_PI/180.0);

	float y = sin(delta_lambda) * cos(phi2);
	float x = (cos(phi1) * sin(phi2)) - (sin(phi1) * cos(phi2) * cos(delta_lambda));

	float theta = atan2(y, x);

	return toDegrees(fmod(theta + 2 * M_PI, 2 * M_PI));
}

/**
APPLY CALIBRATION
Set calibration values for more accurate readings

@author Claus Näveke - TheNitek [https://github.com/TheNitek]

@since v1.1.0
**/
void _applyCalibration()
{
	int x_offset = (_vCalibration[0][0] + _vCalibration[0][1])/2;
	int y_offset = (_vCalibration[1][0] + _vCalibration[1][1])/2;
	int z_offset = (_vCalibration[2][0] + _vCalibration[2][1])/2;
	int x_avg_delta = (_vCalibration[0][1] - _vCalibration[0][0])/2;
	int y_avg_delta = (_vCalibration[1][1] - _vCalibration[1][0])/2;
	int z_avg_delta = (_vCalibration[2][1] - _vCalibration[2][0])/2;

	int avg_delta = (x_avg_delta + y_avg_delta + z_avg_delta) / 3;

	float x_scale = (float)avg_delta / x_avg_delta;
	float y_scale = (float)avg_delta / y_avg_delta;
	float z_scale = (float)avg_delta / z_avg_delta;

	_vCalibrated[0] = (rawValue[0] - x_offset) * x_scale;
	_vCalibrated[1] = (rawValue[1] - y_offset) * y_scale;
	_vCalibrated[2] = (rawValue[2] - z_offset) * z_scale;
}

/**
* @brief Cette fonction sert à calibrer le module QMC5883L pendant le mode calibration du robot.
*
* @return de type uint8_t retourne 1 si la calibration a été complétée.
*/

uint8_t calibration()
{
	if(_calibrationUse)
	_calibrationUse=0;
	while(cntSample<NB_SAMPLE)
	{
		i2cReadBytes(QMC5883_ADDRESS_MAG,0,6);
		if(getX() < calibrationData[0][0]) {
			calibrationData[0][0] = getX();
			
		}
		if(getX() > calibrationData[0][1]) {
			calibrationData[0][1] = getX();
			
		}

		if(getY() < calibrationData[1][0]) {
			calibrationData[1][0] = getY();
			
		}
		if(getY() > calibrationData[1][1]) {
			calibrationData[1][1] = getY();
			
		}

		if(getZ() < calibrationData[2][0]) {
			calibrationData[2][0] = getZ();
			
		}
		if(getZ() > calibrationData[2][1]) {
			calibrationData[2][1] = getZ();
			
		}
		cntSample++;
		return 0;
	}
	cntSample=0;
	setCalibration(calibrationData[0][0],calibrationData[0][1],calibrationData[1][0],calibrationData[1][1],calibrationData[2][0],calibrationData[2][1]);
	
	return 1;
}
//1 = adv 0 =regulier

/**
SET SMOOTHING
Set calibration values for more accurate readings

@author Claus Näveke - TheNitek [https://github.com/TheNitek]

@since v1.1.0
**/
void setSmoothing(uint8_t steps, uint8_t adv){
	_smoothUse = 1;
	_smoothSteps = ( steps > 10) ? 10 : steps;
	_smoothAdvanced = (adv == 1) ? 1 : 0;
}

/**
SMOOTHING
Set calibration values for more accurate readings

@author Claus Näveke - TheNitek [https://github.com/TheNitek]

@since v1.1.0
**/
void _smoothing(){
	uint8_t max = 0;
	uint8_t min = 0;
	
	if ( _vScan > _smoothSteps - 1 ) { _vScan = 0; }
	
	for ( int i = 0; i < 3; i++ ) {
		if ( _vTotals[i] != 0 ) {
			_vTotals[i] = _vTotals[i] - _vHistory[_vScan][i];
		}
		_vHistory[_vScan][i] = ( _calibrationUse ) ? _vCalibrated[i] : rawValue[i];
		_vTotals[i] = _vTotals[i] + _vHistory[_vScan][i];
		
		if ( _smoothAdvanced ) {
			max = 0;
			for (int j = 0; j < _smoothSteps - 1; j++) {
				max = ( _vHistory[j][i] > _vHistory[max][i] ) ? j : max;
			}
			
			min = 0;
			for (int k = 0; k < _smoothSteps - 1; k++) {
				min = ( _vHistory[k][i] < _vHistory[min][i] ) ? k : min;
			}
			
			_vSmooth[i] = ( _vTotals[i] - (_vHistory[max][i] + _vHistory[min][i]) ) / (_smoothSteps - 2);
			} else {
			_vSmooth[i] = _vTotals[i]  / _smoothSteps;
		}
	}
	
	_vScan++;
}

#define TW_START			0x08
#define TW_RESTART			0x10
#define TW_MR_SLAVE_ACK		0X40
#define TW_MR_SLAVE_NACK	0X38
#define TW_MR_DATA_NACK		0X58
#define TW_MT_SLAVE_ACK		0X18
#define TW_MT_DATA_ACK		0X28
#define TW_MT_SLAVE_NACK	0X20
#define TW_MT_DATA_NACK		0X30
#define TW_MR_DATA_ACK		0x50

/**
* @brief Interruption qui gère la transmission et la réception des données entre le maître et l'esclave en fonction du code de status entrée.
*/

ISR(TWI_vect)
{
	switch(TWSR & 0xF8)
	{
		case TW_START:
		TWDR=_i2cAdd;
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		break;
		
		case TW_RESTART:
		if(_i2cModeRead)
		TWDR=_i2cAdd | 1;
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		break;
		case TW_MT_SLAVE_ACK:
		TWDR=_i2cRegistre;
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		break;
		
		case TW_MT_DATA_ACK:
		if(statePointeur)
		{
			statePointeur=0;
			_i2cBusy=0;
			TWCR|=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO)|(1<<TWIE);
		}
		else
		{
			if(_i2cDataCnt>=_i2cDataSize)
			{
				TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
				_i2cBusy=0;
			}
			else
			{
				TWDR=_i2cData[_i2cDataCnt++];
				TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
			}
		}
		break;
		case  TW_MR_SLAVE_ACK:
		if(_i2cDataSize>1)
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA);
		else
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		break;
		case TW_MR_DATA_NACK:
		rawValue[0] = (int)(_i2cData[0] |_i2cData[1]<<8);
		rawValue[1] = (int)(_i2cData[2] |_i2cData[3]<<8);
		rawValue[2] = (int)(_i2cData[4] |_i2cData[5]<<8);
		
		if ( _calibrationUse ) {
			_applyCalibration();
		}
		if ( _smoothUse ) {
			_smoothing();
		}

		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
		_i2cBusy=0;
		break;
		case TW_MR_DATA_ACK:
		_i2cData[_i2cRxDataCnt++]=TWDR;
		if(_i2cRxDataCnt>=_i2cDataSize)
		{
			TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE);
		}
		else
		{
			TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWIE)|(1<<TWEA);
		}
		break;
		case TW_MT_SLAVE_NACK:
		case TW_MR_SLAVE_NACK:
		_i2cErrorCode=1;
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
		_i2cBusy=0;
		break;
		
		case TW_MT_DATA_NACK:
		_i2cErrorCode=2;
		TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWSTO);
		_i2cBusy=0;
		break;
		default:
		_i2cErrorCode=3;
		TWCR=(1<<TWINT)|(1<<TWSTO);
		_i2cBusy=0;
		break;
	}
}
