/*


Libreria LCD_i2c.h


*/


#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "fsl_i2c.h"  // Necesario para I2C_Type

// Dirección I2C del LCD (puede actualizarse tras escaneo)
extern uint8_t lcd_addr;

// Macros de control (ajustar según mapeo del PCF8574)
#define LCD_BACKLIGHT 0x08
#define LCD_ENABLE    0x04
#define LCD_RW        0x02
#define LCD_RS        0x01

/**
 * @brief Inicializa el LCD en modo 4 bits.
 *
 * Envía la secuencia de arranque según el datasheet del HD44780.
 * Debe llamarse una vez tras encender el LCD.
 *
 * @param base Puntero al periférico I2C (ej. I2C1)
 */
void LCD_Init(I2C_Type *base);

/**
 * @brief Envía un comando al LCD.
 *
 * RS=0, RW=0. Se usa para instrucciones como clear, cursor, etc.
 *
 * @param base Puntero al periférico I2C
 * @param cmd  Código de comando
 */
void LCD_Command(I2C_Type *base, uint8_t cmd);

/**
 * @brief Envía un carácter al LCD como dato.
 *
 * RS=1, RW=0. Se usa para mostrar texto.
 *
 * @param base Puntero al periférico I2C
 * @param data Carácter ASCII
 */
void LCD_Data(I2C_Type *base, uint8_t data);

/**
 * @brief Posiciona el cursor en una coordenada específica.
 *
 * @param base Puntero al periférico I2C
 * @param col  Columna (0–15)
 * @param row  Fila (0 o 1)
 */
void LCD_SetCursor(I2C_Type *base, uint8_t col, uint8_t row);

/**
 * @brief Escribe un string en el LCD desde la posición actual del cursor.
 *
 * No realiza salto de línea automático. Si querés escribir en una línea específica,
 * llamá antes a LCD_SetCursor().
 *
 * @param base Puntero al periférico I2C
 * @param str  Cadena de texto terminada en '\0'
 */
void LCD_Print(I2C_Type *base, const char *str);

/**
 * @brief Limpia completamente el contenido del LCD.
 *
 * Envía el comando 0x01 al controlador HD44780, que borra la pantalla
 * y posiciona el cursor en la esquina superior izquierda.
 *
 * @param base Puntero al periférico I2C
 */
void LCD_Clear(I2C_Type *base);

#endif // LCD_I2C_H
