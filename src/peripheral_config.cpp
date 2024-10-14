#include "peripheral_config.h"
#include <Arduino.h>

void GPIO_setup(void){
  PMC->PMC_PCER0 |= PMC_PCER0_PID13;
  // Configuración de pin de salida
  // Se configuran los pines PC24 (D6) y PC25 (D5) como salidas
  PIO_SetOutput(PIO_LED,LED_1|LED_2,0,0,0);


  // Configuración de pin de entrada
  // Se configura primero el debouncing en el pin PC23 (D7)
  
  PIO_SetDebounceFilter(PIO_BOTON,B_START,10); //Se usa una frecuencia de filtro de 50 Hz. Se puede aumentar o disminuir para mejorar el debounce
  PIO_SetInput(PIO_BOTON,B_START,PIO_DEBOUNCE);
  PIO_SetDebounceFilter(PIO_BOTON,B_STOP,10); //Se usa una frecuencia de filtro de 50 Hz. Se puede aumentar o disminuir para mejorar el debounce
  PIO_SetInput(PIO_BOTON,B_STOP,PIO_DEBOUNCE);

}


void ADC_setup(void){
  // Configuración del ADC
  // Primero se activa desde el control de energía (PMC)
  PMC->PMC_PCER1 |= PMC_PCER1_PID37;
  // Reset del ADC para limpiar cualquier valor espurio
  ADC->ADC_CR = ADC_CR_SWRST;

/*
  En el siguiente registro se configuran los parámetros más importantes del ADC
  PRESCAL: Prescaler del reloj. El reloj máximo del ADC no debe superar 22 MHz. Por defecto el prescaler mínimo es 2.
    ADCClock = MCK/((PRESCAL + 1)*2). Para MCK = 84 MHz con PRESCAL = 1 , ADCClock = 21 MHz
    t_conv = 20/ADCClock = 0.95238 ns -> 1 us mínimo para 1 Msps
  TRGEN y TRGSEL_ADC_TRIG4: Se selecciona como trigger una fuente de hardware, en este caso la línea de evento del PWM
  USEQ_REG_ORDER: Se selecciona el muestreo secuencial
  TRACKTIM: Tiempo de "tracking". Es el tiempo entre el inicio de conversión de cada canal. 
    Se selecciona un valor mayor al de conversión para asegurar la conversión correcta a 12 bits.
    Tracking = (TRACKTIM + 1)/ADCClock -> con TRACTIM = 30 se obtiene 1,43 us
  TRANSFER: Tiempo de "transferencia". Es el tiempo entre el trigger de conversión del ADC en un nuevo canal, y el inicio actual de la conversión (ver datasheet).
    Se selecciona un valor pequeño, para asegurar una correcta transferencia de canal a canal.
    Transfer = (TRANSFER*2+3)/ADCClock -> con TRANSFER = 3 se obtienen 0,43 us
  Con el tracking y la transferencia, el tiempo de conversión total por canal es de aprox. 2 us.
  Como son 2 canales, ref(t) y y(t), el tiempo de conversión total antes del cálculo de la uc es de 4 us.
  Con un periodo de 1 ms, se corresponde a un 0,4% del periodo.
  Las otras configuraciones se dejan por defecto
*/ 

  ADC->ADC_MR |= ADC_MR_PRESCAL(1) | ADC_MR_TRGEN | ADC_MR_TRGSEL_ADC_TRIG4 | ADC_MR_TRACKTIM(30) | ADC_MR_TRANSFER(3);// | ADC_MR_USEQ_REG_ORDER; No se usan los 2 canales

  // Se activan los canales a muestrear
  ADC->ADC_CHER = ADC_CHER_CH7;  
  //ADC->ADC_CHER = ADC_CHER_CH6 | ADC_CHER_CH7;
  // Se selecciona la secuencia de muestreo. En este caso se usan los canales 6 y 7 para concordar con A0 y A1 en la placa
  //ADC->ADC_SEQR1 = ADC_SEQR1_USCH7(6) | ADC_SEQR1_USCH8(7); // Debe muestrear 6, luego 7 y finalizar
  //ADC->ADC_SEQR2 = 0;
  // Se configura la interrupción de fin de conversión del canal 7, para ese entonces el canal 6 también debe haber finalizado
  ADC->ADC_IER |= ADC_IER_EOC7;
  NVIC_EnableIRQ(ADC_IRQn);

}

/**
*@param {uint_16} pwm_period_us periodo de PWM en microsegundos
*/
void PWM_setup(unsigned int pwm_period_us){
  // Primero se debe activar la macrocelda PWM desde el controlador de energía (PMC). En este caso el módulo PWM tiene el ID 36
  PMC->PMC_PCER1 |= PMC_PCER1_PID36;
  // Luego, se configura el periférico del canal 0 en el pin correspondiente B16
  PIOB->PIO_ABSR |= PIO_ABSR_P16;
  // Se le da control del pin al periférico
  PIOB->PIO_PDR |= PIO_PDR_P16;
  {
      // Luego, se configura el periférico del canal 0 en el pin correspondiente B12
  PIOB->PIO_ABSR |= PIO_ABSR_P12;
  // Se le da control del pin al periférico
  PIOB->PIO_PDR |= PIO_PDR_P12;

  }
  //------------- Configuración específica del timer
  // Primero configura el reloj. El módulo PWM puede generar 2 relojes subdividiendo el del del sistema, clkA y clkB.
  // En este caso, se usa solo clkA
  // Reloj del sistema MCK = 84 MHz. Para PWM con alineación izquierda F_PWM = MCK/(CRPD*PRESC*DIVA) donde CRPD es el "periodo" o cantidad de conteos
  // Por lo tanto, se configura primero PRESC y DIVA 
  PWM->PWM_CLK = PWM_CLK_PREA(PRESC)|PWM_CLK_DIVA(DIVA); // Con esta configuración, para F_PWM de 1 kHz, el contador CRPD llega a 2000, es decir, resolución de 0.05% en ciclo de trabajo
  // Una vez configurado el reloj, se selecciona el canal PWM, la alineación y la polaridad. En este caso, el canal 0, polaridad es por defecto alineación izquierda
  PWM->PWM_CH_NUM[0].PWM_CMR = PWM_CMR_CPRE_CLKA | PWM_CMR_CPOL;
  // Se carga el valor máximo del contador, CRPD, correspondiente al "periodo"
  PWM->PWM_CH_NUM[0].PWM_CPRD = 2*pwm_period_us;  // F_PWM = 84000000/(2000*1*42) = 1 kHz
  // Se configura un valor de ciclo útil inicial y activa el canal del PWM
  PWM->PWM_CH_NUM[0].PWM_CDTY= 0;//(float)INIT_DUTY_CYCLE/100.0f*CPRD;

  // La sig. config. se corresponde al evento para iniciar el muestreo del ADC
  // El evento no se activa con una bandera de desbordamiento, sino con comparación directa con el valor del contador. 
  // Para que ocurra cada inicio de periodo, se compara con el valor máximo del contador, en este caso CPRD
  PWM->PWM_CMP[0].PWM_CMPV = PWM_CMPV_CV(CPRD);
  // Se activa la comparación
  PWM->PWM_CMP[0].PWM_CMPM = PWM_CMPM_CEN;      // Enable comparison
  
  // Al final,  se selecciona el canal 0 en el generador de pulsos de la línea de eventos 0 (se puede usar otra línea de eventos)
  PWM->PWM_ELMR[0] = PWM_ELMR_CSEL0; 

  // Se activa el PWM
  PWM->PWM_ENA = PWM_ENA_CHID0;   
}



/**
 * \brief Modifica el registro de actualización de ciclo útil. El cambio tendrá efecto en el siguiente periodo.
 *
 * \param duty_cycle              Valor de ciclo útil, en unidad.
 */
void PWM_update_duty_cycle(float duty_cycle){
  // En este dispositivo, el cambio del ciclo útil se realiza sólamente al inicio de cada ciclo
  // Es decir, siempre existe un atraso de implementación digital de un periodo
  if (duty_cycle >= 1){
    duty_cycle = 1;
  }
  PWM->PWM_CH_NUM[0].PWM_CDTYUPD = duty_cycle*CPRD;
}