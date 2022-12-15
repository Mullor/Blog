/*
*Funciones para configurar los pines del microcontrolador.
*/
#include "pines.h"

void PINES_configuracion(void)
{
    //Pines del ATmega4808 no utilizados en la aplicación.
    //Identificamos a cada pin por su nombre en el microcontrolador o por la notación de Arduino.
    #if defined(DEBUG)
    const uint8_t unusedPins[] = {PIN_PD0, PIN_PD1, PIN_PD2, PIN_PD3, PIN_PD4, PIN_PD5,
     PIN_PD7, PIN_PA1, PIN_PA2, PIN_PA3};
    //Habilitamos el puerto serie conectado al programador.
    Serial2.begin(115200);    
    #else
    const uint8_t unusedPins[] = {PIN_PD0, PIN_PD1, PIN_PD2, PIN_PD3, PIN_PD4, PIN_PD5,
     PIN_PD7, PIN_PA1, PIN_PA2, PIN_PA3, PIN_PF0, PIN_PF1};
    #endif

    //Reduce el consumo de los pines que no se van a usar en el ATmega4808.
    uint8_t index;
    for (index = 0; index < sizeof(unusedPins); index++)
    {
      pinMode(unusedPins[index], INPUT_PULLUP);
      LowPower.disablePinISC(unusedPins[index]);
    }

    //Configuración de pines entrada/salidas.
    pinMode(LED_BUILTIN, OUTPUT);   
    pinMode(TPL5010_WAKE, INPUT);   
    pinMode(TPL5010_DONE, OUTPUT);

    //I2C inicializado en el setup del main, se puede mover aquí (mirar dependencias Arduino librerías) 
}