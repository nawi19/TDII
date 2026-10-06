/*

Contenido de LCD_i2C.C

*/

#include "lcd_i2c.h"
#include "fsl_debug_console.h"

uint8_t lcd_addr = 0x27;  // Dirección por defecto

static status_t LCD_I2C_WriteByte(I2C_Type *base, uint8_t data)
{
    status_t ret = I2C_MasterStart(base, lcd_addr, kI2C_Write);
    if (ret != kStatus_Success) return ret;
    ret = I2C_MasterWriteBlocking(base, &data, 1, kI2C_TransferDefaultFlag);
    I2C_MasterStop(base);
    return ret;
}

static void LCD_SendNibble(I2C_Type *base, uint8_t nibble, uint8_t mode)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT | mode;
    LCD_I2C_WriteByte(base, data | LCD_ENABLE);
    SDK_DelayAtLeastUs(50, SystemCoreClock);
    LCD_I2C_WriteByte(base, data & ~LCD_ENABLE);
    SDK_DelayAtLeastUs(50, SystemCoreClock);
}

void LCD_SendByte(I2C_Type *base, uint8_t value, uint8_t mode)
{
    LCD_SendNibble(base, value & 0xF0, mode);
    LCD_SendNibble(base, (value << 4) & 0xF0, mode);
}

void LCD_Command(I2C_Type *base, uint8_t cmd)
{
    LCD_SendByte(base, cmd, 0x00); // RS=0
    SDK_DelayAtLeastUs(2000, SystemCoreClock);
}

void LCD_Data(I2C_Type *base, uint8_t data)
{
    LCD_SendByte(base, data, LCD_RS); // RS=1
    SDK_DelayAtLeastUs(2000, SystemCoreClock);
}

void LCD_Init(I2C_Type *base)
{
    SDK_DelayAtLeastUs(50000, SystemCoreClock); // >40ms tras power-on

    // Forzar modo 8 bits
    LCD_SendNibble(base, 0x30, 0x00);
    SDK_DelayAtLeastUs(5000, SystemCoreClock);
    LCD_SendNibble(base, 0x30, 0x00);
    SDK_DelayAtLeastUs(5000, SystemCoreClock);
    LCD_SendNibble(base, 0x30, 0x00);
    SDK_DelayAtLeastUs(1000, SystemCoreClock);

    // Pasar a modo 4 bits
    LCD_SendNibble(base, 0x20, 0x00);
    SDK_DelayAtLeastUs(1000, SystemCoreClock);

    // Configuración
    LCD_Command(base, 0x28); // 4 bits, 2 líneas
    LCD_Command(base, 0x0C); // Display ON
    LCD_Command(base, 0x06); // Entry mode
    LCD_Command(base, 0x01); // Clear display
    SDK_DelayAtLeastUs(2000, SystemCoreClock);
}

void LCD_SetCursor(I2C_Type *base, uint8_t col, uint8_t row)
{
    static const uint8_t offsets[] = {0x00, 0x40};
    LCD_Command(base, 0x80 | (col + offsets[row]));
}

void LCD_Print(I2C_Type *base, const char *str)
{
    while (*str) {
        LCD_Data(base, (uint8_t)*str++);
    }
}

void LCD_Clear(I2C_Type *base)
{
    LCD_Command(base, 0x01);  // Comando: Clear display
    //delay(5);
    SDK_DelayAtLeastUs(2000, SystemCoreClock);  // Retardo mínimo requerido
}
