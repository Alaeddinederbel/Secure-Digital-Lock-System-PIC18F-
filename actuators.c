#include "actuators.h"
#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 20000000

//  Relay 
// Relay on RC0; HIGH = coil energised = UNLOCKED (modify if your relay is
// active-LOW - just invert the logic in Unlock/Lock).

void RELAY_Init(void) {
    TRISCbits.TRISC0 = 0;
    PORTCbits.RC0    = 0; // Start locked
}

void RELAY_Unlock(void) {
    PORTCbits.RC0 = 1;
}

void RELAY_Lock(void) {
    PORTCbits.RC0 = 0;
}

// Buzzer (passive, driven by CCP1 PWM on RC2) 
// We toggle RC2 manually (simpler than full CCP setup for short tones).

void BUZZER_Init(void) {
    TRISCbits.TRISC2 = 0;
    PORTCbits.RC2    = 0;
}

// Generates a ~2 kHz tone for `ms` milliseconds (blocking)
static void BUZZER_Tone(uint16_t ms) {
    uint16_t cycles = (uint16_t)(ms * 2); // ~2 kHz: toggle every 250us
    for (uint16_t i = 0; i < cycles; i++) {
        PORTCbits.RC2 ^= 1;
        __delay_us(250);
    }
    PORTCbits.RC2 = 0;
}

void BUZZER_Beep(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        BUZZER_Tone(80);
        __delay_ms(100);
    }
}

void BUZZER_Alarm(void) {
    // Three long harsh beeps
    for (uint8_t i = 0; i < 3; i++) {
        BUZZER_Tone(400);
        __delay_ms(200);
    }
}

void BUZZER_Success(void) {
    // Two rising beeps
    BUZZER_Tone(100);
    __delay_ms(60);
    BUZZER_Tone(200);
}

//  LEDs 
void LED_Init(void) {
    TRISAbits.TRISA0 = 0; // Green
    TRISAbits.TRISA1 = 0; // Red
    PORTAbits.RA0    = 0;
    PORTAbits.RA1    = 1; // Red ON at startup (locked)
}

void LED_Green(uint8_t on) {
    PORTAbits.RA0 = on ? 1 : 0;
}

void LED_Red(uint8_t on) {
    PORTAbits.RA1 = on ? 1 : 0;
}
