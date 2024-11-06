/*
  Sistemas de control y automatizacion - 2024
*/

#include <Arduino.h>
#include "peripheral_config.h"

// Variables globalesm
int adc_7;
const int T_SAMPLE = 1;
volatile bool run = 0, buffer_full = 0, data_sent = 0;
volatile int counter_N = 0;
volatile float ref = 0, y = 0;
volatile float u = 0;
float s_ref[N_SAVED], s_y[N_SAVED], s_u[N_SAVED];

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
  //actualizar uk
  
  //e = ref - y;
  //------------ Cálculo de la acción de control --------------------------------
  // Acción P: up(k) = Kp*e(k)
  //up = Kp*e;
  
  // Acción PI, aprox. Forward: upi(k) = upi(k-1) + Kpi*e(k) + a*Kpi*e(k-1)
  //upi = upi_ + Kpi*e + a*Kpi*ek_;  

  // Acción PI para el PI-D
  //upi = upi_ + Kpid*e + a*Kpid*e_;
                
  // Acción D (desde la salida) aprox. Backward: ud(k) = Kdd*[y(k)-y(k-1)]/T
  //ey = y - y_;	    // Error de velocidad: y(k)-y(k-1).
  //ud_ = Kdd*ey;		// Cálculo de la acción derivativa

  // Acción PI-D
  //upid_k = upik - ud_k;
  //upid_k = upi_k - ud_k;

  //guardar variables en buffer


  delayMicroseconds(50);//solo para debug, comentar o borrar despues de implementar el control
  u = 0.25;             //solo para debug, comentar o borrar despues de implementar el control

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






