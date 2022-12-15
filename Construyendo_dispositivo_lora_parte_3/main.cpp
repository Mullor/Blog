/*
* Programa para mandar los datos de temperatura y humedad
* cada X minutos al servidor de Helium.    
*/
#include "pines.h"
#include <lmic.h>
#include <hal/hal.h>
#include <RocketScream_LowPowerAVRZero.h>
#include <CayenneLPP.h>
#include <Wire.h>
#include <SHT2x.h>
#include "dispositivos.h"
#include "adc.h"
#include "configuracion.h"

//Instancia de Cayenne, configuramos el tamaño de los datos enviados a Cayenne.
//Cada sensor añade al payload 2 bytes + los bytes de sus datos según el tipo de sensor.
CayenneLPP lpp(12); //Térmometro + Humedad + Lectura de batería

//Instancia del sensor SHT21 copiando el ejemplo de la librería.
uint32_t start;
uint32_t stop;
SHT2x sht;

//Declaración de funciones.
void isr_timer(void);
void do_send(osjob_t* j);

//Variables globales para el control del programa.
//True = hay un envío en curso, no poner el microcontrolador a dormir.
volatile boolean b_envioEnCurso = true;
//True = generamos el pulso de DONE en el bucle principal del programa.
volatile boolean b_pulsoDONE = true;
//True = iniciamos un envio de la radio en el bucle principal del programa.
volatile boolean b_iniciarEnvio = true;

//Variables globales usadas para formar el payload.
float temperatura = 3;
float humedad = 4;
float bateria;

// Device EUI. Al copiar los datos de la consola de Helium hay que indicarle que lo haga en formato "lsb"
static const u1_t PROGMEM DEVEUI[8]={DEVEUI_Sensor_Temperatura_00 };
void os_getDevEui (u1_t* buf) { memcpy_P(buf, DEVEUI, 8);}

// App EUI. Al copiar los datos de la consola de Helium hay que indicarle que lo haga en formato "lsb"
static const u1_t PROGMEM APPEUI[8]={APPEUI_Sensor_Temperatura_00 };
void os_getArtEui (u1_t* buf) { memcpy_P(buf, APPEUI, 8);}

// App Key. Al copiar los datos de la consola de Helium hay que indicarle que lo haga en formato "msb"
static const u1_t PROGMEM APPKEY[16] = { APPKEY_Sensor_Temperatura_00 };
void os_getDevKey (u1_t* buf) {  memcpy_P(buf, APPKEY, 16);}


static osjob_t sendjob;

//Al compilar aparece un aviso de que no tiene el pinmap de la placa y que debemos utilizar un propio.
//Le damos el pinmap de la placa.
const lmic_pinmap lmic_pins = {
    .nss = RFM95_SS,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = RFM95_RST,
    .dio = {RFM95_DIO0, RFM95_DIO1, LMIC_UNUSED_PIN},
};

//Imprime información en hexadecimal en el monitor serial al producirse el evento "Joined"
//Si el dispositivo se ha unido al servidor de red.
void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial2.print('0');
    Serial2.print(v, HEX);
}

//Función declarada en la libreria lmic. Se ejecuta cada vez que se produce un evento.
//Aquí podemos poner código de control.
void onEvent (ev_t ev) {
    #if defined(DEBUG)
    Serial2.print(os_getTime());
    Serial2.print(": ");
    #endif
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            #if defined(DEBUG)
            Serial2.println(F("EV_SCAN_TIMEOUT"));
            #endif
            break;
        case EV_BEACON_FOUND:
            #if defined(DEBUG)
            Serial2.println(F("EV_BEACON_FOUND"));
            #endif
            break;
        case EV_BEACON_MISSED:
            #if defined(DEBUG)
            Serial2.println(F("EV_BEACON_MISSED"));
            #endif
            break;
        case EV_BEACON_TRACKED:
            #if defined(DEBUG)
            Serial2.println(F("EV_BEACON_TRACKED"));
            #endif
            break;
        case EV_JOINING:
            #if defined(DEBUG)
            Serial2.println(F("EV_JOINING"));
            #endif
            break;
        case EV_JOINED:
            #if defined(DEBUG)
            Serial2.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial2.print("netid: ");
              Serial2.println(netid, DEC);
              Serial2.print("devaddr: ");
              Serial2.println(devaddr, HEX);
              Serial2.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i)
              {
                if (i != 0)
                  Serial2.print("-");
                printHex2(artKey[i]);
              }
              Serial2.println("");
              Serial2.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) 
              {
                      if (i != 0)
                              Serial2.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial2.println();
            }
            #endif
            //Deshabilitar la validación de comprobación de enlaces (habilitada automáticamente durante la unión, 
            //pero no es compatible con TTN en este momento).
            LMIC_setLinkCheckMode(0); //0
            break;
        case EV_RFU1:
            #if defined(DEBUG)
            Serial2.println(F("EV_RFU1"));
            #endif
            break;
        case EV_JOIN_FAILED:
            #if defined(DEBUG)
            Serial2.println(F("EV_JOIN_FAILED"));
            #endif
            break;
        case EV_REJOIN_FAILED:
            #if defined(DEBUG)
            Serial2.println(F("EV_REJOIN_FAILED"));
            #endif
            break;
        case EV_TXCOMPLETE:
            #if defined(DEBUG)
            Serial2.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial2.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial2.println(F("Received "));
              Serial2.println(LMIC.dataLen);
              Serial2.println(F(" bytes of payload"));
            }
            #endif
            b_envioEnCurso = false; //Finaliza el envío, podemos dormir.
            delay(250); //(por probar) Esperamos a que terminen de trabajar todos los poeriféricos del
            //microcontrolador antes de dormir. Comprobar consumo en función de este delay.
            break;
        case EV_LOST_TSYNC:
            #if defined(DEBUG)
            Serial2.println(F("EV_LOST_TSYNC"));
            #endif
            break;
        case EV_RESET:
            #if defined(DEBUG)
            Serial2.println(F("EV_RESET"));
            #endif
            break;
        case EV_RXCOMPLETE:
            #if defined(DEBUG)
            // data received in ping slot
            Serial2.println(F("EV_RXCOMPLETE"));
            #endif
            break;
        case EV_LINK_DEAD:
            #if defined(DEBUG)
            Serial2.println(F("EV_LINK_DEAD"));
            #endif
            break;
        case EV_LINK_ALIVE:
            #if defined(DEBUG)
            Serial2.println(F("EV_LINK_ALIVE"));
            #endif
            break;
        case EV_TXSTART:
            #if defined(DEBUG)
            Serial2.println(F("EV_TXSTART"));
            #endif
            break;
        case EV_TXCANCELED:
            #if defined(DEBUG)
            Serial2.println(F("EV_TXCANCELED"));
            #endif
            break;
        case EV_RXSTART:
            /* No utilizar el monitor serial en este evento. */
            break;
        case EV_JOIN_TXCOMPLETE:
            #if defined(DEBUG)
            Serial2.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            #endif
            break;
         default:
            #if defined(DEBUG)
            Serial2.println(F("Unknown event"));
            Serial2.println((unsigned) ev);
            #endif
            break; 
    }
}

//Lanza la transmisión de los datos.
void do_send(osjob_t* j){
    //Comprueba si hay un trabajo TX/RX actual en ejecución
    if (LMIC.opmode & OP_TXRXPEND) {
        #if defined(DEBUG)
        Serial2.println(F("OP_TXRXPEND, not sending"));
        #endif
    } else {
        //Prepara el envío de datos en el próximo momento posible.
        //Enviamos los datos en el formato de Cayenne.      
        lpp.reset();
        lpp.addAnalogInput(1, bateria); //Batería en Voltios.
        lpp.addAnalogInput(2, temperatura);
        lpp.addAnalogInput(3, humedad);
        LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
        #if defined(DEBUG)
        Serial2.println(F("Packet queued"));
        #endif
    }
}

void setup() {
    
    //Configuramos los pines del microcontrolador
    PINES_configuracion();

    //Imprimimos un mensaje por el puerto serie.
    #if defined(DEBUG)
    Serial2.println(F("Programa iniciandose..."));
    #endif

    //Inicializamos el ADC.
    ADC_bateriaInicializarADC();

    //Inicializamos el sensor de temperatura y humedad.
    sht.begin();

    //Configuramos la interrupción en el ATmega4808 que produce el pin de Wake del TPL5010
    attachInterrupt(digitalPinToInterrupt(TPL5010_WAKE), isr_timer, RISING);


    //Generamos el pulso de DONE del TPL5010
    //El TPL5010 comienza a funcionar.
    digitalWrite(TPL5010_DONE, HIGH);
    delay(1);
    digitalWrite(TPL5010_DONE, LOW);

    //Configuración de la librería LMIC.
    // LMIC init
    os_init();
    
    // Restablece el estado MAC. Se descartarán las transferencias de datos de sesión y pendientes.
    LMIC_reset();

    // allow much more clock error than the X/1000 default. See:
    // https://github.com/mcci-catena/arduino-lorawan/issues/74#issuecomment-462171974
    // https://github.com/mcci-catena/arduino-lmic/commit/42da75b56#diff-16d75524a9920f5d043fe731a27cf85aL633
    // the X/1000 means an error rate of 0.1%; the above issue discusses using
    // values up to 10%. so, values from 10 (10% error, the most lax) to 1000
    // (0.1% error, the most strict) can be used.
    LMIC_setClockError(MAX_CLOCK_ERROR * 10 / 100);

    // Sub-band 2 - Helium Network
    LMIC_setLinkCheckMode(0); 

    //Spread Factor y potencia de emisión de la radio.
    //En la primera transmisión la librería manda valores por defecto, esta función tendría efecto a 
    //partir de la segunda transmisión. Modificar librería si se quiere cambiar en la primera.
    LMIC_setDrTxpow(DR_SF7, 14);

    //Iniciar un trabajo (se realiza un prime envío, inicia automáticamente OTAA)
    //Leemos el sensor de humedad y temperatura.
    start = micros();
    sht.read();
    stop = micros();
    temperatura = sht.getTemperature();
    humedad = sht.getHumidity();
    //Leemos el nivel de tensión de la batería.
    bateria = ADC_bateriaLeerVoltaje()/1000.F;
    //Iniciamos el envío
    do_send(&sendjob);

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

    if(b_pulsoDONE == true)
    {
        //Generamos la señal de Done para el timer.
        digitalWrite(TPL5010_DONE, HIGH);
        //Añadimos un delay para el tiempo de señal en alto de DONE del TPL5010.
        for(char i = 0; i < 5; i++){} 
        digitalWrite(TPL5010_DONE, LOW);
        b_pulsoDONE = false;
    }

    if(b_iniciarEnvio == true)
    {
        //Leemos el sensor de humedad y temperatura.
        start = micros();
        sht.read();
        stop = micros();
        temperatura = sht.getTemperature();
        humedad = sht.getHumidity();
        //Leemos el nivel de tensión de la batería.
        bateria = ADC_bateriaLeerVoltaje()/1000.F;
        //Iniciamos un envío por radio.
        os_setCallback (&sendjob, do_send);
        b_iniciarEnvio = false;
    }

    //Realiza el envío en curso si lo hay.
    os_runloop_once();

    //Ponemos el microcontrolador a dormir.
    if(b_envioEnCurso == false)
    {
        #if defined(DEBUG)
        Serial2.print(F("El valor de la batería en mV es: "));
        Serial2.println(bateria, DEC);
        Serial2.print(F("El valor de la temperatura es: "));
        Serial2.println(temperatura, DEC);
        Serial2.print(F("El valor de la humedad es: "));
        Serial2.println(humedad, DEC);
        Serial2.println(F("Microcontrolador a dormir\n"));
        delay(500);
        //Esperamos a que se terminen de mandar los mensajes serie antes de dormir.
        #endif
        LowPower.powerDown();
        //Se despierta aquí después de atender a la interrupción del timer. 
        #if defined(DEBUG)
        Serial2.println(F("Microcontrolador despierto"));
        #endif
    }      
}

//Interrupción del timer.
void isr_timer(void)
{   
    //Variable para enviar cada x minutos.
    static unsigned char contador; //static: inicializada a cero por defecto.

    //Variable para generar el pulso de DONE en el bucle principal.
    b_pulsoDONE = true;

    contador ++; //Un incremento de esta variable es igual a 1 minuto con R = 22k.
    if(contador == MINUTOS_ENVIO)
    {
        contador = 0;
        //Preparamos un envío a Helium.
        b_envioEnCurso = true; //Se pone en false cuando se finaliza el envío.
        b_iniciarEnvio = true; //Se inicia el envío al comenzar el bucle principal.
        
    }
}