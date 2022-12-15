/*
*Definiciones hardware de los pines del microcontrolador.
*Funciones para la configuración de los pines del microcontrolador.
*/
#ifndef PINES_H
#define PINES_H

#include <Arduino.h>
#include <avr/io.h>
#include <RocketScream_LowPowerAVRZero.h>
#include "configuracion.h"

//Definición de los pines asignados en el esquema del hardware.

//TPL5010 Timer
#define TPL5010_DONE PIN_PC0
#define TPL5010_WAKE PIN_PC2
//RFM95 Radio LoRa
#define RFM95_DIO0 PIN_PC1
#define RFM95_DIO1 PIN_PC3
#define RFM95_RST PIN_PD6
#define RFM95_SS PIN_PA7
#define RFM95_SCK PIN_PA6
#define RFM95_MISO PIN_PA5
#define RFM95_MOSI PIN_PA4
//LED en el PCB
#define LED_BUILTIN PIN_PA0

void PINES_configuracion(void);

#endif