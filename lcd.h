#ifndef LCD_H
#define LCD_H

#include <xc.h>

// --- LCD Pin Definitions (PORTD) ---
#define LCD_RS  PORTDbits.RD4
#define LCD_EN  PORTDbits.RD5
#define LCD_D4  PORTDbits.RD0
#define LCD_D5  PORTDbits.RD1
#define LCD_D6  PORTDbits.RD2
#define LCD_D7  PORTDbits.RD3

#define LCD_RS_TRIS  TRISDbits.TRISD4
#define LCD_EN_TRIS  TRISDbits.TRISD5
#define LCD_DATA_TRIS  (TRISD &= 0xF0)  // RD0-RD3 as output

void LCD_Init(void);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_WriteChar(char c);
void LCD_WriteString(const char *str);

#endif
