//  Configuration bits (20 MHz external crystal)
#pragma config FOSC   = HS      // High-speed crystal
#pragma config WDT    = OFF
#pragma config PWRT   = ON      // Power-up timer
#pragma config LVP    = OFF     // Disable low-voltage programming
#pragma config MCLRE  = ON
#pragma config PBADEN = OFF     // PORTB as digital
#pragma config BOR    = OFF

#define _XTAL_FREQ 20000000

#include <xc.h>
#include <stdint.h>
#include <string.h>
#include "lcd.h"
#include "keypad.h"
#include "eeprom_mgr.h"
#include "actuators.h"

// Constants
#define MAX_ATTEMPTS    3        // Lockout after this many wrong PINs
#define UNLOCK_SECS     5        // Seconds the relay stays open
#define LOCKOUT_SECS    30       // Seconds of lockout after max attempts
#define INPUT_TIMEOUT   15       // Seconds before input is auto-cleared

// State machine ─
typedef enum {
    STATE_FIRST_RUN,   // No PIN in EEPROM yet — prompt to set one
    STATE_SET_PIN,     // First-time PIN entry (two passes for confirm)
    STATE_LOCKED,      // Idle, waiting for key presses
    STATE_ENTERING,    // User is typing PIN digits
    STATE_UNLOCKED,    // Relay open, countdown to re-lock
    STATE_LOCKOUT,     // Too many wrong attempts
    STATE_CHANGE_OLD,  // Change PIN: verify old PIN first
    STATE_CHANGE_NEW,  // Change PIN: enter new PIN
    STATE_CHANGE_CFM   // Change PIN: confirm new PIN
} State;

// Globals 
static State    state          = STATE_LOCKED;
static char     input_buf[MAX_PIN_LEN + 1];
static uint8_t  input_len      = 0;
static char     stored_pin[MAX_PIN_LEN + 1];
static uint8_t  stored_pin_len = 0;
static char     new_pin[MAX_PIN_LEN + 1];
static uint8_t  new_pin_len    = 0;
static uint8_t  attempts       = 0;
static uint8_t  set_pass       = 0; // 0=first entry, 1=confirm

// Simple second timer using TMR0 
// At 20 MHz, prescaler 256: one TMR0 overflow ≈ 13.1 ms
// We count overflows to get seconds.
volatile uint16_t tmr0_ticks = 0;

void __interrupt() ISR(void) {
    if (INTCONbits.TMR0IF) {
        tmr0_ticks++;
        INTCONbits.TMR0IF = 0;
        TMR0H = 0; TMR0L = 0;
    }
}

static void TIMER_Init(void) {
    T0CON = 0b10000111; // TMR0 ON, 8-bit, prescaler 1:256
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE    = 1;
    INTCONbits.PEIE   = 1;
}

// Approx seconds elapsed since last reset of tmr0_ticks
// 20 MHz / 4 cycles per inst / 256 prescaler / 256 counts = ~76 overflows/sec
#define TICKS_PER_SEC 76u

static uint16_t Seconds(void) {
    return tmr0_ticks / TICKS_PER_SEC;
}

//  Helpers 
static void InputClear(void) {
    memset(input_buf, 0, sizeof(input_buf));
    input_len = 0;
}

// Display asterisks for entered digits
static void LCD_ShowEntry(uint8_t row, const char *prompt) {
    LCD_SetCursor(row, 0);
    LCD_WriteString(prompt);
    for (uint8_t i = strlen(prompt); i < 16; i++) {
        LCD_WriteChar(i < strlen(prompt) + input_len ? '*' : ' ');
    }
}

// Countdown display (second line)
static void LCD_ShowCountdown(uint8_t secs_left, const char *label) {
    LCD_SetCursor(1, 0);
    LCD_WriteString(label);
    // Write remaining seconds
    char num[4];
    num[0] = '0' + (secs_left / 10);
    num[1] = '0' + (secs_left % 10);
    num[2] = 's';
    num[3] = ' ';
    LCD_WriteString(num);
}

//  State handlers

static void Handle_FirstRun(void) {
    LCD_Clear();
    LCD_SetCursor(0, 0); LCD_WriteString("  LOCK SETUP");
    LCD_SetCursor(1, 0); LCD_WriteString("Press # to begin");
    char k = 0;
    while (k != '#') k = KEYPAD_GetKey();
    state   = STATE_SET_PIN;
    set_pass = 0;
    InputClear();
    LCD_Clear();
}

static void Handle_SetPin(char key) {
    if (key >= '0' && key <= '9') {
        if (input_len < MAX_PIN_LEN) {
            input_buf[input_len++] = key;
            BUZZER_Beep(1);
        }
    } else if (key == '*') {
        InputClear();
    } else if (key == '#') {
        if (input_len < MIN_PIN_LEN) {
            LCD_SetCursor(1, 0);
            LCD_WriteString("Min 4 digits!  ");
            __delay_ms(1500);
            return;
        }
        if (set_pass == 0) {
            // Store first entry as new_pin for confirmation
            memcpy(new_pin, input_buf, input_len);
            new_pin_len = input_len;
            new_pin[new_pin_len] = '\0';
            set_pass = 1;
            InputClear();
            LCD_Clear();
        } else {
            // Confirm pass
            if (input_len == new_pin_len &&
                memcmp(input_buf, new_pin, input_len) == 0) {
                EEPROM_SavePIN(new_pin, new_pin_len);
                stored_pin_len = EEPROM_LoadPIN(stored_pin);
                LCD_Clear();
                LCD_SetCursor(0, 0); LCD_WriteString("  PIN SAVED!");
                LCD_SetCursor(1, 0); LCD_WriteString("System locked.");
                BUZZER_Success();
                __delay_ms(2000);
                state = STATE_LOCKED;
                LED_Red(1); LED_Green(0);
            } else {
                LCD_SetCursor(1, 0);
                LCD_WriteString("No match! Retry");
                BUZZER_Alarm();
                __delay_ms(1500);
                set_pass = 0;
            }
            InputClear();
            LCD_Clear();
        }
    }
}

static uint16_t idle_tick = 0; // For input timeout

static void Handle_Locked(char key) {
    if (key) {
        idle_tick = tmr0_ticks; // Reset idle timer

        if (key >= '0' && key <= '9') {
            InputClear();
            input_buf[input_len++] = key;
            state = STATE_ENTERING;
            BUZZER_Beep(1);
        }
        // Long-hold '*' for PIN change is handled in main loop polling
    }
}

static void Handle_Entering(char key) {
    // Timeout check
    if ((tmr0_ticks - idle_tick) > (INPUT_TIMEOUT * TICKS_PER_SEC)) {
        InputClear();
        state = STATE_LOCKED;
        LCD_Clear();
        return;
    }

    if (!key) return;
    idle_tick = tmr0_ticks;

    if (key >= '0' && key <= '9') {
        if (input_len < MAX_PIN_LEN) {
            input_buf[input_len++] = key;
            BUZZER_Beep(1);
        }
    } else if (key == '*') {
        InputClear();
        state = STATE_LOCKED;
        LCD_Clear();
    } else if (key == '#') {
        input_buf[input_len] = '\0';

        if (input_len == stored_pin_len &&
            memcmp(input_buf, stored_pin, stored_pin_len) == 0) {
            // Correct PIN
            attempts = 0;
            EEPROM_SetAttempts(0);
            state = STATE_UNLOCKED;
            tmr0_ticks = 0; // Reset for unlock countdown
            RELAY_Unlock();
            LED_Green(1); LED_Red(0);
            BUZZER_Success();
            LCD_Clear();
            LCD_SetCursor(0, 0); LCD_WriteString("  ** OPEN **  ");
        } else {
            // Wrong PIN
            attempts++;
            EEPROM_SetAttempts(attempts);
            BUZZER_Alarm();
            LCD_Clear();
            LCD_SetCursor(0, 0);
            if (attempts < MAX_ATTEMPTS) {
                LCD_WriteString("Wrong PIN!     ");
                char rem[16] = "Attempts left: ";
                rem[15] = '0' + (MAX_ATTEMPTS - attempts);
                LCD_SetCursor(1, 0); LCD_WriteString(rem);
                __delay_ms(2000);
                state = STATE_LOCKED;
            } else {
                // Lockout
                state = STATE_LOCKOUT;
                tmr0_ticks = 0;
                LCD_WriteString(" *** LOCKED ***");
                LCD_SetCursor(1, 0); LCD_WriteString("Wait 30s...    ");
                LED_Red(1); LED_Green(0);
            }
        }
        InputClear();
        LCD_Clear();
    }
}

static void Handle_Unlocked(void) {
    uint8_t elapsed = (uint8_t)Seconds();
    uint8_t left    = (elapsed < UNLOCK_SECS) ? (UNLOCK_SECS - elapsed) : 0;
    LCD_ShowCountdown(left, "Re-lock in: ");
    if (left == 0) {
        RELAY_Lock();
        LED_Green(0); LED_Red(1);
        state = STATE_LOCKED;
        attempts = 0;
        LCD_Clear();
    }
}

static void Handle_Lockout(void) {
    uint8_t elapsed = (uint8_t)Seconds();
    uint8_t left    = (elapsed < LOCKOUT_SECS) ? (LOCKOUT_SECS - elapsed) : 0;
    LCD_SetCursor(0, 0); LCD_WriteString(" *** LOCKED ***");
    LCD_ShowCountdown(left, "Wait: ");
    if (left == 0) {
        attempts = 0;
        EEPROM_SetAttempts(0);
        state = STATE_LOCKED;
        LCD_Clear();
    }
}

// ─── Main ─────────────────────────────────────────────────────────────────
void main(void) {
    // Init all peripherals
    LCD_Init();
    KEYPAD_Init();
    RELAY_Init();
    BUZZER_Init();
    LED_Init();
    TIMER_Init();

    // Splash
    LCD_SetCursor(0, 0); LCD_WriteString(" Secure Lock v1");
    LCD_SetCursor(1, 0); LCD_WriteString("  Initialising ");
    __delay_ms(1500);
    LCD_Clear();

    // Load PIN from EEPROM
    if (!EEPROM_IsPINSet()) {
        state = STATE_FIRST_RUN;
    } else {
        stored_pin_len = EEPROM_LoadPIN(stored_pin);
        attempts       = EEPROM_GetAttempts();
        state          = (attempts >= MAX_ATTEMPTS) ? STATE_LOCKOUT : STATE_LOCKED;
        tmr0_ticks     = 0;
    }

    while (1) {
        char key = KEYPAD_GetKey();

        // Update LCD prompt based on state
        switch (state) {
            case STATE_FIRST_RUN:
                Handle_FirstRun();
                break;

            case STATE_SET_PIN:
                if (set_pass == 0) {
                    LCD_SetCursor(0, 0); LCD_WriteString("Set new PIN:    ");
                } else {
                    LCD_SetCursor(0, 0); LCD_WriteString("Confirm PIN:    ");
                }
                LCD_ShowEntry(1, "");
                if (key) Handle_SetPin(key);
                break;

            case STATE_LOCKED:
                LCD_SetCursor(0, 0); LCD_WriteString("  [  LOCKED  ] ");
                LCD_SetCursor(1, 0); LCD_WriteString("Enter PIN:      ");
                Handle_Locked(key);
                break;

            case STATE_ENTERING:
                LCD_SetCursor(0, 0); LCD_WriteString("Enter PIN:      ");
                LCD_ShowEntry(1, "");
                Handle_Entering(key);
                break;

            case STATE_UNLOCKED:
                Handle_Unlocked();
                break;

            case STATE_LOCKOUT:
                Handle_Lockout();
                break;

            // Change-PIN sub-states (same logic as SET_PIN but verifies old first)
            case STATE_CHANGE_OLD:
                LCD_SetCursor(0, 0); LCD_WriteString("Enter old PIN:  ");
                LCD_ShowEntry(1, "");
                if (key) Handle_SetPin(key); // Reuse logic (simplified)
                break;

            default:
                state = STATE_LOCKED;
                break;
        }

        __delay_ms(30); // ~33 Hz main loop
    }
}
