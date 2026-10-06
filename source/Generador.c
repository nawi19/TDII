
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
void pint0_callback(pint_pin_int_t, uint32_t);
void pint1_callback(pint_pin_int_t, uint32_t);
void pint2_callback(pint_pin_int_t, uint32_t);
void LCD_Print_WF();
void LCD_Print_frec();
void Actualizar_frec();


gpio_pin_config_t out_config = {kGPIO_DigitalOutput, 1};
gpio_pin_config_t in_config = {kGPIO_DigitalInput};
dac_config_t dacConfigStruct;
i2c_master_config_t cfg;


#define SYSTEM_CORE_CLOCK CLOCK_GetFreq(kCLOCK_CoreSysClk) //clock del sistema
#define DAC_frec 30000000
#define DAC_contadorMAX 655300
#define DAC_contadorMIN 1 //250 original. 199 lo minimo q llegue 184 ya se rompe

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
 I2C_config();
 LCD_config();
 GPIO_config();
 DAC_config();
 while (1);

 }


 void DAC_config(){

  POWER_DisablePD(kPDRUNCFG_PD_DAC0); //Activo el DAC
  DAC_GetDefaultConfig(&dacConfigStruct); //Configuración del DAC
  DAC_Init(DAC0, &dacConfigStruct);
  DAC_SetCounterValue(DAC0, DAC_contador);
  DAC_EnableDoubleBuffering(DAC0, true);

  NVIC_SetPriority(DAC0_IRQn, 3); //Interrupción con la menor prioridad
  NVIC_EnableIRQ(DAC0_IRQn); //Habilito la interrupción del DAC

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
  LCD_SetCursor(I2C1, 0, 0);
  LCD_Print(I2C1, " Facundo Bernal ");
  LCD_SetCursor(I2C1, 0, 1);
  LCD_Print(I2C1, "TD2 UTN FRA 2025");

 }


 void GPIO_config(){

   GPIO_PortInit(GPIO, 0);
   GPIO_PortInit(GPIO, 1);

   GPIO_PinInit(GPIO, 0, 4, &in_config); // User
   GPIO_PinInit(GPIO, 0, 12, &in_config); //ISP

   GPIO_PinInit(GPIO, 0, 18, &in_config); //clk
   GPIO_PinInit(GPIO, 0, 19, &in_config); //sw

   GPIO_PinInit(GPIO, 1, 0, &out_config); //LED
   GPIO_PinInit(GPIO, 1, 1, &out_config); //LED
   GPIO_PinInit(GPIO, 1, 2, &out_config); //LED

   GPIO_PinWrite(GPIO, 1, 1, 0);

   SYSCON_AttachSignal(SYSCON,
		   kPINT_PinInt0,
		   kSYSCON_GpioPort0Pin17ToPintsel); //Para interrupción por GPIO

   SYSCON_AttachSignal(SYSCON,
		   kPINT_PinInt1,
		   kSYSCON_GpioPort0Pin18ToPintsel); //Para interrupción por GPIO


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

	 EnableIRQ(PIN_INT0_IRQn);
	 EnableIRQ(PIN_INT1_IRQn);

 }


  void DAC0_IRQHandler(void){ //Handler de interrupción de DAC

   DAC_SetBufferValue(DAC0, senal_actual[iDAC++]);//Escribe el valor en el DAC, luego adelanta una posición
   if (iDAC>=cant_actual) iDAC=0;//si iDAC es igual al total de elementos de las muestras, vuelve al inicio

  }

  void




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
  WF_frec=((WF_frec+50U)/100U)*100U;
  }
  if(DAC_contador<DAC_contadorMIN){
  DAC_contador=DAC_contadorMIN;
  WF_frec=(DAC_frec/(DAC_contador*lista[iForma].senal[iCalidad].cant));
  WF_frec=((WF_frec+50U)/100U)*100U;
  }
  senal_actual=lista[iForma].senal[iCalidad].id; //Alojadas localmente
  cant_actual=lista[iForma].senal[iCalidad].cant;
  DAC_SetCounterValue(DAC0, DAC_contador); //Coloco el contador
  LCD_Print_frec();

  }


  void LCD_Print_WF(){ //Actualiza la forma de onda en el LCD

  LCD_SetCursor(I2C1, 0, 0); //Imprimo nombre de la forma de onda
  LCD_Print(I2C1, " ");
  LCD_SetCursor(I2C1, 0, 0); //Imprimo nombre de la forma de onda
  LCD_Print(I2C1, lista[iForma].nombre);

  }


  void LCD_Print_frec(){

  LCD_SetCursor(I2C1, 0, 1); //Imprimo la frecuencia
  char aux[17]; //aux
  if (WF_frec>9999) snprintf(aux, sizeof(aux), "f=%d.%d kHz ", (WF_frec/1000),((WF_frec %
  1000)/100));
  else{snprintf(aux, sizeof(aux), "f=%d Hz ", WF_frec);}
  LCD_Print(I2C1,aux);

  }


  void SysTick_Handler(){}
