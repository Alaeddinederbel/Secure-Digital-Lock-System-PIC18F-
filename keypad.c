#include "keypad.h"
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 20000000

/*
 * Wiring:
 *  Rows    -> PORTB output pins RB0..RB3  (driven LOW one at a time)
 *  Columns -> PORTB input pins  RB4..RB7  (internal pull-ups enabled via INTCON2)
 *
 * Keypad layout:
 *   1  2  3  A
 *   4  5  6  B
 *   7  8  9  C
 *   *  0  #  D
 *
 * In this project we use only 0-9, * (clear), # (enter).
 * A-D are ignored.
 */

static const char KEY_MAP[4][4] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

void KEYPAD_Init(void) {
    // RB0-RB3 = outputs (rows), RB4-RB7 = inputs (cols)
    TRISB = 0xF0;         // 1111 0000
    INTCON2bits.RBPU = 0; // Enable PORTB pull-ups (columns)
    PORTB = 0xFF;         // All rows HIGH initially
}

char KEYPAD_GetKey(void) {
    for (uint8_t row = 0; row < 4; row++) {
        // Drive the current row LOW, all others HIGH
        PORTB = ~(1 << row) & 0x0F;
        __delay_us(10); // Settling time

        uint8_t cols = (PORTB >> 4) & 0x0F;

        if (cols != 0x0F) { // At least one column pulled low
            __delay_ms(20); // Debounce

            cols = (PORTB >> 4) & 0x0F; // Re-read after debounce
            if (cols == 0x0F) continue;  // Bounce — skip

            for (uint8_t col = 0; col < 4; col++) {
                if (!(cols & (1 << col))) {
                    // Wait for key release
                    while (!((PORTB >> 4) & (1 << col)));
                    __delay_ms(10);

                    PORTB = 0x0F; // Restore rows
                    return KEY_MAP[row][col];
                }
            }
        }
    }
    PORTB = 0x0F; // Restore rows
    return 0;     // No key pressed
}
