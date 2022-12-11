/*
* Manda el valor de la batería por el puerto serie del conector por cada pulso
* del tiemer. El resto del tiempo el microcontrolador está durmiendo.
*/
#include <RocketScream_LowPowerAVRZero.h>
#include "pines.h"
#include "adc.h"

// Declaración ISR del timer.
void isr_timer(void);

//Variable para generar el pulso DONE fuera de la ISR del timer.
volatile boolean b_pulsoDONE = true;

void setup() {
  //Este código se ejecuta solo una vez al inicio del programa.

  //Configuramos los pines del microcontrolador.
  PINES_configuracion();

  //Inicializamos el ADC para la medida de la batería.
  ADC_bateriaInicializarADC();

  //Configuramos la interrupción que produce el pin de Wake del TPL5010.
  //Cada vez que el pin de Wake del ATmega vea un flanco de subida se llama a isr_timer.
  attachInterrupt(digitalPinToInterrupt(TPL5010_WAKE), isr_timer, RISING);

  //Generamos el pulso de DONE del TPL5010. El TPL5010 comienza a funcionar.
  digitalWrite(TPL5010_DONE, HIGH);
  delay(1);
  digitalWrite(TPL5010_DONE, LOW);

  //Parpadeos al iniciarse el programa. Manera visual de detectar un reset de la placa.
  for(char i = 0; i<3; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(250);
    digitalWrite(LED_BUILTIN, LOW);
    delay(250);
  }
}

void loop() {
  //Este código se ejecuta de manera repetida en bucle.
  uint16_t valorBateria = 0;

  //Generamos el pulso DONE para el timer cada vez que se ejecute su ISR.
  if(b_pulsoDONE == true)
  {
      //Generamos la señal de Done para el timer.
      digitalWrite(TPL5010_DONE, HIGH);
      //Añadimos un delay para el tiempo de señal en alto de DONE del TPL5010.
      for(char i = 0; i < 5; i++){} 
      digitalWrite(TPL5010_DONE, LOW); 
      b_pulsoDONE = false;
  }

  //Leemos el voltaje de la batería.
  valorBateria = ADC_bateriaLeerVoltaje();

  Serial2.print(F("El valor de la batería en mV es: "));
  Serial2.println(valorBateria, DEC);
  Serial2.println(F("Microcontrolador a dormir."));
  //Añadimos un retardo para poder enviar el mensaje antes de que se ponga a dormir.
  delay(500);
  //Ponemos el microcontrolador a dormir.
  LowPower.powerDown();
  //El microcontrolador se despierta aquí después de atender a la interrupción del timer.
  Serial2.println(F("Microcontrolador despierto."));
}

/*
*Interrupción que tiene lugar cuando el timer despierta al microcontrolador.
*El timer llamará a esta interrupción cada 60 segundos.
*El pulso DONE del timer lo generaremos siempre fuera de esta ISR.
*/
void isr_timer(void)
{
  //Variable para generar el pulso de DONE en el bucle principal.
  b_pulsoDONE = true;
}