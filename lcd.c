#include "lcd.h"
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 20000000

// Send a nibble (4 bits) to LCD data pins
static void LCD_Nibble(uint8_t nibble) {
    LCD_D4 = (nibble >> 0) & 1;
    LCD_D5 = (nibble >> 1) & 1;
    LCD_D6 = (nibble >> 2) & 1;
    LCD_D7 = (nibble >> 3) & 1;
    LCD_EN = 1;
    __delay_us(1);
    LCD_EN = 0;
    __delay_us(50);
}

// Send full byte as two nibbles (high nibble first)
static void LCD_Send(uint8_t byte, uint8_t rs) {
    LCD_RS = rs;
    LCD_Nibble(byte >> 4);   // high nibble
    LCD_Nibble(byte & 0x0F); // low nibble
}

void LCD_Init(void) {
    // Set data and control pins as outputs
    LCD_DATA_TRIS;
    LCD_RS_TRIS = 0;
    LCD_EN_TRIS = 0;
    LCD_RS = 0;
    LCD_EN = 0;

    __delay_ms(50); // Power-on delay

    // Initialization sequence (from HD44780 datasheet)
    LCD_Nibble(0x03); __delay_ms(5);
    LCD_Nibble(0x03); __delay_ms(1);
    LCD_Nibble(0x03); __delay_us(150);
    LCD_Nibble(0x02); // Switch to 4-bit mode

    LCD_Send(0x28, 0); // Function set: 4-bit, 2 lines, 5x8 font
    LCD_Send(0x0C, 0); // Display ON, cursor OFF, blink OFF
    LCD_Send(0x06, 0); // Entry mode: increment, no shift
    LCD_Send(0x01, 0); // Clear display
    __delay_ms(2);
}

void LCD_Clear(void) {
    LCD_Send(0x01, 0);
    __delay_ms(2);
}

void LCD_SetCursor(uint8_t row, uint8_t col) {
    // Row 0 = 0x80, Row 1 = 0xC0
    uint8_t addr = (row == 0) ? 0x80 : 0xC0;
    LCD_Send(addr + col, 0);
}

void LCD_WriteChar(char c) {
    LCD_Send((uint8_t)c, 1);
}

void LCD_WriteString(const char *str) {
    while (*str) {
        LCD_WriteChar(*str++);
    }
}
