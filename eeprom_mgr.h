#ifndef EEPROM_MGR_H
#define EEPROM_MGR_H
#include <stdint.h>

#define EEPROM_PIN_ADDR       0x00  // Stores PIN digits (up to 6 bytes)
#define EEPROM_PIN_LEN_ADDR   0x06  // Stores PIN length (1 byte)
#define EEPROM_ATTEMPTS_ADDR  0x07  // Stores failed attempt counter (1 byte)
#define EEPROM_VALID_ADDR     0x08  // 0xA5 = PIN has been set

#define PIN_VALID_MAGIC  0xA5
#define MAX_PIN_LEN      6
#define MIN_PIN_LEN      4

void     EEPROM_WriteByte(uint8_t addr, uint8_t data);
uint8_t  EEPROM_ReadByte(uint8_t addr);
void     EEPROM_SavePIN(const char *pin, uint8_t len);
uint8_t  EEPROM_LoadPIN(char *pin);  // Returns length
uint8_t  EEPROM_IsPINSet(void);
void     EEPROM_SetAttempts(uint8_t attempts);
uint8_t  EEPROM_GetAttempts(void);

#endif
