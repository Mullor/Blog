/*
*Funciones relacionadas con el uso del ADC
*/
#ifndef ADC_H
#define ADCH

#include <avr/io.h>
uint16_t ADC_bateriaLeerVoltaje(void);
void ADC_bateriaInicializarADC(void);

#endif