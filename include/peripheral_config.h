#ifndef PERIPHERAL_CONFIG
#define PERIPHERAL_CONFIG

#define INIT_DUTY_CYCLE 0   // Ciclo de trabajo inicial
#define PRESC 0
#define DIVA 42
#define CPRD 2000
#define N_SAVED 3000


#define delta_Ref	0.002
#define REF50 0.5f
#define REF75 0.75f
#define REF80 0.8f
#define REF100 1.f
#define PIO_LED PIOC
#define LED_1 PIO_PC25
#define LED_2 PIO_PC24 
#define PIO_BOTON PIOC
#define B_START PIO_PC23
#define B_STOP PIO_PC22

void GPIO_setup(void);
void ADC_setup(void);
void PWM_setup(unsigned int);
void PWM_update_duty_cycle(float duty_cycle);

#endif