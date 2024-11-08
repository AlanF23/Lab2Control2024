/*
  Sistemas de control y automatizacion - 2024
*/

#include <Arduino.h>
#include "peripheral_config.h"

// Variables globalesm
int adc_7;
const int T_SAMPLE = 6;
volatile bool run = 0, buffer_full = 0, data_sent = 0;
volatile int counter_N = 0;
volatile float ref = 0, y = 0;
volatile float u = 0;
float s_ref[N_SAVED], s_y[N_SAVED], s_u[N_SAVED];


const float Kpi = 0.2724272430209788755;
const float Kd = 0.0002863986337621703;
const float a = -0.634784293524907;
volatile float ek = 0;
volatile float ekm1 = 0;
volatile float upik = 0;
volatile float upikm1 = 0;
volatile float eyk = 0;
volatile float ykm1 = 0;
volatile float ud_k = 0;
const float fm = 1/0.006;

void setup() {
  // Llamada a funciones de configuración
  GPIO_setup();
  ADC_setup();
  PWM_setup(1000);
  PWM_update_duty_cycle(0);
  Serial.begin(115200);
}

void loop() {

  static bool btn1, btn1_ = true, btn2, btn2_ = true;
  btn1 = PIO_Get(PIO_BOTON,PIO_INPUT,B_START);
  btn2 = PIO_Get(PIO_BOTON,PIO_INPUT,B_STOP);
  if(btn1 && !btn1_)
  {
    run = 1;
  }
  if(btn2 && !btn2_)
  {
    run = 0;
  }

  btn2_ = btn2;
  btn1_ = btn1;
  
  if(buffer_full && !data_sent){
    Serial.print("start");
    Serial.print('\n');
    for(uint16_t i = 0 ; i < N_SAVED ; i++){
      Serial.print(s_ref[i],4);
      Serial.print(',');
      Serial.print(s_y[i],4);
      Serial.print(',');
      Serial.print(s_u[i],4);
      Serial.print('\n');
      delay(2);
    }
    Serial.print("end");
    Serial.print('\n');
    data_sent = 1;
    PIO_Clear(PIO_LED,LED_2);
  }

}
//si leen esto me deben una birra jaja salu2


void ADC_Handler(void){
  static int index = 0;

  if(!(ADC->ADC_ISR & ADC_ISR_EOC7)){return;} 
  adc_7 = ADC->ADC_CDR[7]; //Se lee el valor del registro de resultado de conversión del canal 7
  counter_N++; //Cada vez que se ejecuta la interrupción del ADC se aumenta el contador para obtener la frecuencia de muestreo a partir de la del PWM
  if(counter_N < T_SAMPLE){return;}

  counter_N = 0; // Se limpia el contador que genera el periodo de muestreo
  
  PWM_update_duty_cycle(u); // Se actualiza la acción de control al inicio del periodo de muestreo para

  if(!run){
    index = 0;
    buffer_full = 0;
    data_sent = 0;
    PIO_Clear(PIO_LED,LED_2);
    //limpiar variables de control
    u = 0;
    return;
  }
      
  PIO_Set(PIO_LED,LED_1); // Se enciende el LED de estado 1
  //actualiar ref

  //actualizar ek
  ek = ref - y;
  //------------ Cálculo de la acción de control --------------------------------
  // Acción PI para el PI-D
  upik = upikm1 + Kpi*ek + a*Kpi*ekm1;
  // Acción D (desde la salida) aprox. Backward: ud(k) = Kd*[y(k)-y(k-1)]/T
  eyk = y - ykm1; // Error de velocidad: y(k)-y(k-1).//
  ud_k = Kd*eyk*fm; // Cálculo de la acción derivativa
  // Acción PI-D
  u = upik - ud_k;

  ekm1 = ek;
  upikm1 = upik;
  ykm1 = y;
  

  //guardar variables en buffer
  // delayMicroseconds(50);//solo para debug, comentar o borrar despues de implementar el control
  // u = 0.25;             //solo para debug, comentar o borrar despues de implementar el control
  if(!buffer_full){
    s_ref[index] = ref;
    s_y[index] = y;
    s_u[index] = u;
    ++index;
    if(index >= N_SAVED){
      buffer_full = 1;
      PIO_Set(PIO_LED,LED_2);
    }

  }

  PIO_Clear(PIO_LED,LED_1); //El led se apaga al actualizar la acción de control

}






