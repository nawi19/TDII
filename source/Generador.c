
//============================================CONTENIDO=DEL=MAIN=====================================
#include <ondas.h>
#include <stdio.h>
#include "stdbool.h"
#include "board.h"
#include "fsl_dac.h"
#include "fsl_gpio.h"
#include "fsl_common.h"
#include "fsl_debug_console.h"
#include "fsl_iocon.h"
#include "fsl_swm.h"
#include "fsl_i2c.h"
#include "fsl_device_registers.h"
#include "fsl_power.h"
#include "fsl_pint.h"
#include "fsl_inputmux.h"
#include "fsl_inputmux_connections.h"
#include "fsl_syscon.h"
#include "pin_mux.h"
#include "lcd_i2c.h"
#include "ondas.h"
#include "string.h"
#include "peripherals.h"
#include "LPC845.h"
#include "clock_config.h"


void DAC_config();
void I2C_config();
void LCD_config();
void GPIO_config();
void PintCallback(pint_pin_int_t pintr, uint32_t pmatch_status);
void SwitchCallback(pint_pin_int_t pintr, uint32_t pmatch_status);
void Frec_a_digitos(uint32_t f);
uint32_t Digitos_a_frec();
void LCD_Print_WF();
void LCD_Print_frec();
void Encoder_Init();
void Actualizar_frec();


gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 1};
gpio_pin_config_t in_config = {kGPIO_DigitalInput};
dac_config_t dacConfigStruct;
i2c_master_config_t cfg;


#define SYSTEM_CORE_CLOCK CLOCK_GetFreq(kCLOCK_CoreSysClk) //clock del sistema
#define DAC_frec 30000000
#define DAC_contadorMAX 65535
#define DAC_contadorMIN 195 //250 original. 199 lo minimo q llegue 184 ya se rompe
#define NUM_DIGITOS 4
#define FREC_MAX 9999

volatile int8_t  digitos[NUM_DIGITOS] = {5, 0, 0, 0};
volatile uint8_t digitoactual = 0;
volatile uint32_t msTicks = 0, lastTickEncoder = 0, lastTickSwitch = 0;
volatile bool frec_cambio = false;  // flag para avisar al main

uint32_t DAC_contador=1, WF_frec=5000, iDAC=0;;
volatile int iForma=0, iCalidad=0, nid_ext=1, cant_deseada;
uint16_t* senal_actual=senoidal_48;
uint32_t cant_actual=48;

int main(void){

 BOARD_InitBootPins();
 BOARD_InitBootClocks();
 BOARD_InitBootPeripherals();
 SystemCoreClockUpdate();
 BOARD_InitDebugConsole();

 BOARD_BootClockFRO30M();
 SystemCoreClockUpdate();
 SysTick_Config(SystemCoreClock/1000);
 NVIC_SetPriority(SysTick_IRQn, 1);
 I2C_config();
 LCD_config();
 GPIO_config();
 DAC_config();
 Encoder_Init();

 frec_cambio = true;

 while (1){

	 if (frec_cambio){
	         frec_cambio = false;

	         __disable_irq();
	         WF_frec = Digitos_a_frec();
	         __enable_irq();

	         if (WF_frec > FREC_MAX) WF_frec = FREC_MAX;
	         if (WF_frec < 1)        WF_frec = 1;

	         Actualizar_frec();

	         __disable_irq();
	         Frec_a_digitos(WF_frec);
	         __enable_irq();

	    	 PRINTF("valor: %d%d%d%d dig:%d WF:%u\r\n",
	    	 	                  digitos[0],digitos[1],digitos[2],digitos[3],
	    	 	                  digitoactual, (unsigned)WF_frec);

	     }

 }

}

void DAC_config(){
    POWER_DisablePD(kPDRUNCFG_PD_DAC0);
    DAC_GetDefaultConfig(&dacConfigStruct);
    DAC_Init(DAC0, &dacConfigStruct);
    DAC_EnableDoubleBuffering(DAC0, true);

    DAC_contador = DAC_frec/(WF_frec*lista[iForma].senal[iCalidad].cant);
    DAC_SetCounterValue(DAC0, DAC_contador);

    NVIC_SetPriority(DAC0_IRQn, 3);
    //EnableIRQ(DAC0_IRQn);

    DAC_contador=DAC_frec/(WF_frec*lista[iForma].senal[iCalidad].cant); //Calculo el contador necesario
    DAC_SetCounterValue(DAC0, DAC_contador); //Establezco contador del DAC

}


 void I2C_config(){ //Configuración I2C

  CLOCK_Select(kI2C1_Clk_From_MainClk); // Configuración pines I2C

  CLOCK_EnableClock(kCLOCK_Swm);
  SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SDA, kSWM_PortPin_P0_27);
  SWM_SetMovablePinSelect(SWM0, kSWM_I2C1_SCL, kSWM_PortPin_P0_26);
  CLOCK_DisableClock(kCLOCK_Swm);

  i2c_master_config_t cfg; // Inicialización I2C
  I2C_MasterGetDefaultConfig(&cfg);
  cfg.baudRate_Bps = 100000;
  I2C_MasterInit(I2C1, &cfg, SystemCoreClock);

 }


 void LCD_config(){ //Configuración LCD

  LCD_Init(I2C1);
  LCD_Print_WF();   // muestra el nombre de la forma de onda en la línea 1

 }


 void GPIO_config(){

   GPIO_PortInit(GPIO, 0);
   GPIO_PortInit(GPIO, 1);

   GPIO_PinInit(GPIO, 0, 4, &in_config); // User
   GPIO_PinInit(GPIO, 0, 12, &in_config); //ISP

   GPIO_PinInit(GPIO, 0, 16, &in_config); //dt
   GPIO_PinInit(GPIO, 0, 18, &in_config); //clk
   GPIO_PinInit(GPIO, 0, 19, &in_config); //sw

   GPIO_PinInit(GPIO, 1, 0, &out_config); //LED
   GPIO_PinInit(GPIO, 1, 1, &out_config); //LED
   GPIO_PinInit(GPIO, 1, 2, &out_config); //LED

   GPIO_PinWrite(GPIO, 1, 1, 0);

 }

 void Encoder_Init(void){

	 CLOCK_EnableClock(kCLOCK_GpioInt);
	 SYSCON->PINTSEL[0] = 18U; // canal PINT0 <- P0_18 (CLK)
	 SYSCON->PINTSEL[1] = 19U; // canal PINT1 <- P0_19 (SW)

	 PINT_Init(PINT);

	 PINT_PinInterruptConfig(PINT,
			 kPINT_PinInt0,
			 kPINT_PinIntEnableFallEdge,
			 PintCallback);

	 PINT_PinInterruptConfig(PINT,
			 kPINT_PinInt1,
			 kPINT_PinIntEnableFallEdge,
			 SwitchCallback);

	 PINT_EnableCallback(PINT); //activo el callback

	 NVIC_SetPriority(PIN_INT0_IRQn, 2);
	 NVIC_SetPriority(PIN_INT1_IRQn, 2);

	 EnableIRQ(PIN_INT0_IRQn);
	 EnableIRQ(PIN_INT1_IRQn);


 }

 uint32_t Digitos_a_frec(void){
     uint32_t f = 0;
     for(int i=0; i<NUM_DIGITOS; i++)
     f = f*10 + digitos[i];
     return f;
 }

 void Frec_a_digitos(uint32_t f){
     for(int i=NUM_DIGITOS-1; i>=0; i--){
         digitos[i] = f % 10;
         f /= 10;
     }
 }

 void PintCallback(pint_pin_int_t pintr, uint32_t pmatch_status)
 {
     if ((msTicks - lastTickEncoder) < 5U) return;
     lastTickEncoder = msTicks;

     uint8_t clk = GPIO_PinRead(GPIO, 0, 18);
     uint8_t dt  = GPIO_PinRead(GPIO, 0, 16);

     if (clk != dt){
         digitos[digitoactual]++;
         if (digitos[digitoactual] > 9) digitos[digitoactual] = 0;
     } else {
         digitos[digitoactual]--;
         if (digitos[digitoactual] < 0) digitos[digitoactual] = 9;
     }
     frec_cambio = true;
 }

 void SwitchCallback(pint_pin_int_t pintr, uint32_t pmatch_status)
 {
     if ((msTicks - lastTickSwitch) < 200U) return;
     lastTickSwitch = msTicks;

     digitoactual++;
     if (digitoactual >= NUM_DIGITOS) digitoactual = 0;
     frec_cambio = true;
 }


  void DAC0_IRQHandler(void){ //Handler de interrupción de DAC

   DAC_SetBufferValue(DAC0, senal_actual[iDAC++]);//Escribe el valor en el DAC, luego adelanta una posición
   if (iDAC>=cant_actual) iDAC=0;//si iDAC es igual al total de elementos de las muestras, vuelve al inicio

  }


 /* void pint0_callback(pint_pin_int_t pintr, uint32_t pmatch_status){ //Cambia forma de onda

   iForma++; //mueve de nid
   if(iForma>=cant_formas)iForma=0; //si llega al final
   Actualizar_frec();
   LCD_Print_WF();

  }


  void pint1_callback(pint_pin_int_t pintr, uint32_t pmatch_status){ //Baja la frecuencia

  if(WF_frec>10000) WF_frec=WF_frec-1000;
  else WF_frec=WF_frec-100;
  Actualizar_frec();

  }


  void pint2_callback(pint_pin_int_t pintr, uint32_t pmatch_status){ //Sube la frecuencia

  if(WF_frec>10000) WF_frec=WF_frec+1000;
  else WF_frec=WF_frec+100;
  Actualizar_frec();

  }*/


  void Actualizar_frec(){ //Actualiza la frecuencia

  if (WF_frec>12000) iCalidad=0; //6 muestras
  else if (WF_frec>8500) iCalidad=1; //12 muestras
  else if (WF_frec>6300) iCalidad=2; //18 muestras
  else if (WF_frec>5100) iCalidad=3; //24 muestras
  else if (WF_frec>4200) iCalidad=4; //30 muestras
  else if (WF_frec>3600) iCalidad=5; //36 muestras
  else if (WF_frec>3200) iCalidad=6; //42 muestras
  else iCalidad=7; //48 muestras

  if(iCalidad>=lista[iForma].pasos) iCalidad=lista[iForma].pasos-1;
  DAC_contador=DAC_frec/(WF_frec*lista[iForma].senal[iCalidad].cant);//Calculo el contador necesario

  if(DAC_contador>DAC_contadorMAX){ //Si se pasa de los limites del contador...

  DAC_contador=DAC_contadorMAX;
  WF_frec=(DAC_frec/(DAC_contador*lista[iForma].senal[iCalidad].cant));

  }

  if(DAC_contador<DAC_contadorMIN){

  DAC_contador=DAC_contadorMIN;
  WF_frec=(DAC_frec/(DAC_contador*lista[iForma].senal[iCalidad].cant));

  }

  senal_actual=lista[iForma].senal[iCalidad].id; //Alojadas localmente
  cant_actual=lista[iForma].senal[iCalidad].cant;
  DAC_SetCounterValue(DAC0, DAC_contador); //Coloco el contador
  LCD_Print_frec();

  }


  void LCD_Print_WF(){ //Actualiza la forma de onda en el LCD

  LCD_SetCursor(I2C1, 0, 0); //Imprimo nombre de la forma de onda
  LCD_Print(I2C1, lista[iForma].nombre);

  }


  void LCD_Print_frec(){

  LCD_SetCursor(I2C1, 0, 1); //Imprimo la frecuencia
  char aux[17]; //aux
  if (WF_frec>9999) snprintf(aux, sizeof(aux), "f=%d.%d kHz   ", (int)(WF_frec/1000), (int)((WF_frec%1000)/100));
  else{snprintf(aux, sizeof(aux), "f=%d Hz       ", (int)WF_frec);}
  LCD_Print(I2C1,aux);

  }


  void SysTick_Handler(){

	  msTicks++;

  }
