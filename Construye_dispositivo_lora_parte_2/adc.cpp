/*
*Funciones para calcular el nivel de la batería conectada al dispositivo.
*/
#include "adc.h"

//Función para inicializar el ADC para la lectura de la batería.
void ADC_bateriaInicializarADC(void)
{
    //Configuramos el valor de la referencia a la entrada del ADC.
    VREF.CTRLA |= VREF_AC0REFSEL_1V1_gc; //Referencia a la entrada del ADC 1V1.
    //Configuramos el ADC.
    ADC0.CTRLC |= ADC_REFSEL_VDDREF_gc; //seleccionamos la tensión de la batería como referencia del ADC.
    ADC0.CTRLC |= ADC_PRESC_DIV8_gc; //Frecuencia ADC FCPU/8, 50 kHz a 1.5MHz para 10 bits de res.
    ADC0.MUXPOS = ADC_MUXPOS_DACREF_gc; //Seleccionamos DACREF0 como entrada al ADC.
    ADC0.CTRLD |= ADC_INITDLY_DLY64_gc; //Delay antes de la primera muestra cuando se despierta.
    ADC0.CTRLA |= ADC_ENABLE_bm ; //Habilitamos el ADC.
}

//Devuelve el valor de la batería en mV.
uint16_t  ADC_bateriaLeerVoltaje(void)
{
    ADC0.COMMAND = ADC_STCONV_bm; //Empezar una conversión. 
    while ( !(ADC0.INTFLAGS & ADC_RESRDY_bm) ) //Esperar a que la conversión se haya realizado.
    {
        ;
    }
    ADC0.INTFLAGS = ADC_RESRDY_bm; //Limpiar el Flag que indica que se ha realizado la conversión
    return(1125300/ADC0.RES); //(1023*Vref(1.1)*1000/Vadc) Devuelve el valor de la batería en mV. 
}