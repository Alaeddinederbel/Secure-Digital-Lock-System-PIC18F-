#ifndef ACTUATORS_H
#define ACTUATORS_H
#include <stdint.h>

// Relay (door lock) - RC0
void RELAY_Init(void);
void RELAY_Unlock(void);
void RELAY_Lock(void);

// Buzzer - RC2 (CCP1 PWM, passive buzzer)
void BUZZER_Init(void);
void BUZZER_Beep(uint8_t count);     // short beeps
void BUZZER_Alarm(void);             // lockout alarm pattern
void BUZZER_Success(void);           // unlock melody

// LEDs - RA0 (green), RA1 (red)
void LED_Init(void);
void LED_Green(uint8_t on);
void LED_Red(uint8_t on);

#endif
