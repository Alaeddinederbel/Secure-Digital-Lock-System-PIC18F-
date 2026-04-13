#include "eeprom_mgr.h"
#include <xc.h>
#include <stdint.h>

// PIC18F built-in EEPROM write
void EEPROM_WriteByte(uint8_t addr, uint8_t data) {
    EEADR  = addr;
    EEDATA = data;
    EECON1bits.EEPGD = 0; // Access data EEPROM
    EECON1bits.CFGS  = 0;
    EECON1bits.WREN  = 1; // Enable writes

    INTCONbits.GIE = 0;   // Disable interrupts during unlock sequence
    EECON2 = 0x55;
    EECON2 = 0xAA;
    EECON1bits.WR = 1;    // Start write
    INTCONbits.GIE = 1;   // Re-enable interrupts

    while (EECON1bits.WR); // Wait for write complete
    EECON1bits.WREN = 0;   // Disable writes
}

// PIC18F built-in EEPROM read
uint8_t EEPROM_ReadByte(uint8_t addr) {
    EEADR  = addr;
    EECON1bits.EEPGD = 0;
    EECON1bits.CFGS  = 0;
    EECON1bits.RD    = 1; // Start read
    return EEDATA;
}

void EEPROM_SavePIN(const char *pin, uint8_t len) {
    for (uint8_t i = 0; i < len; i++) {
        EEPROM_WriteByte(EEPROM_PIN_ADDR + i, (uint8_t)pin[i]);
    }
    EEPROM_WriteByte(EEPROM_PIN_LEN_ADDR, len);
    EEPROM_WriteByte(EEPROM_VALID_ADDR, PIN_VALID_MAGIC);
    EEPROM_WriteByte(EEPROM_ATTEMPTS_ADDR, 0); // Reset attempts
}

uint8_t EEPROM_LoadPIN(char *pin) {
    uint8_t len = EEPROM_ReadByte(EEPROM_PIN_LEN_ADDR);
    if (len < MIN_PIN_LEN || len > MAX_PIN_LEN) len = 4; // Fallback

    for (uint8_t i = 0; i < len; i++) {
        pin[i] = (char)EEPROM_ReadByte(EEPROM_PIN_ADDR + i);
    }
    pin[len] = '\0';
    return len;
}

uint8_t EEPROM_IsPINSet(void) {
    return (EEPROM_ReadByte(EEPROM_VALID_ADDR) == PIN_VALID_MAGIC);
}

void EEPROM_SetAttempts(uint8_t attempts) {
    EEPROM_WriteByte(EEPROM_ATTEMPTS_ADDR, attempts);
}

uint8_t EEPROM_GetAttempts(void) {
    return EEPROM_ReadByte(EEPROM_ATTEMPTS_ADDR);
}
