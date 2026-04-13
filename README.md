# Secure Digital Lock System (PIC18F4550)

Embedded access-control system with PIN authentication, 4×4 keypad input,
16×2 LCD feedback, relay-driven lock actuation, and lockout protection.

---

## System architecture

```
                        +------------------+
                        |   Power supply   |
                        |  12V → 7805 → 5V |
                        +--------+---------+
                                 |
                    +------------+------------+
                    |                         |
         +----------+----------+    +---------+---------+
         |   PIC18F4550  MCU   |    |  Internal EEPROM  |
         |                     +<-->+  PIN + attempts   |
         |  Auth state machine |    +-------------------+
         |  TMR0 lockout timer |
         +--+--------+-----+--+
            |        |     |
     input  |        |     |  outputs
            |        |     |
   +--------+--+  +--+--+  +-------+----------+
   | 4×4 Keypad|  | LCD |  | Relay | Buzzer   |
   | PORTB     |  | 16×2|  | RC0   | RC2 PWM  |
   | Row scan  |  | PORTD  | NPN   |          |
   | Col input |  | 4-bit  | driver|          |
   +-----------+  +-----+  +---+---+----------+
                               |
                         +-----+------+
                         | 12V Solenoid|
                         | / Mag-lock  |
                         +------------+

  Signal flow:
  Keypad → PORTB → Auth engine → EEPROM verify
                              ↓ correct PIN
                           Relay → Lock opens (5s)
                              ↓ wrong PIN (×3)
                           Lockout → Buzzer alarm (30s)
                              ↓ always
                           LCD → Status feedback
```

---

## Hardware required

| Component         | Part / value                          |
|-------------------|---------------------------------------|
| MCU               | PIC18F4550 (40-pin DIP)               |
| Crystal           | 20 MHz + 2× 22 pF caps               |
| Keypad            | 4×4 matrix (membrane)                 |
| LCD               | 16×2 HD44780 compatible               |
| LCD contrast pot  | 10 kΩ trim pot                        |
| Relay module      | 5 V SRD-05VDC (active-HIGH coil)      |
| NPN transistor    | 2N2222 or BC547                       |
| Flyback diode     | 1N4007 across relay coil              |
| Relay base R      | 1 kΩ (RC0 → transistor base)          |
| Buzzer            | Passive 5 V piezo                     |
| LED green         | 5 mm LED + 330 Ω                      |
| LED red           | 5 mm LED + 330 Ω                      |
| Regulator         | 7805 + 10 µF + 100 nF decoupling caps |
| Lock              | 12 V solenoid bolt or mag-lock        |
| PSU               | 12 V DC (powers regulator + lock)     |

---

## Wiring summary

### Keypad → PORTB
```
Row 0 (top)  → RB0
Row 1        → RB1
Row 2        → RB2
Row 3 (bot)  → RB3
Col 0 (left) → RB4
Col 1        → RB5
Col 2        → RB6
Col 3 (rght) → RB7
```
Rows are driven LOW for scanning. Columns use internal pull-ups (INTCON2.RBPU=0).

### LCD (4-bit mode) → PORTD
```
LCD D4 → RD0     LCD RS → RD4
LCD D5 → RD1     LCD EN → RD5
LCD D6 → RD2     LCD RW → GND
LCD D7 → RD3     LCD V0 → Wiper of 10kΩ pot
```

### Outputs
```
RC0 → 1kΩ → 2N2222 base     (relay driver)
2N2222 collector → Relay IN
2N2222 emitter  → GND
1N4007 across relay coil (cathode → +5V)

RC2 → Passive buzzer → GND

RA0 → 330Ω → Green LED → GND
RA1 → 330Ω → Red LED   → GND
```

---

## Building

### MPLAB X (recommended)
1. File → New Project → Standalone Project → PIC18F4550
2. Add all `.c` and `.h` files to the project
3. Set compiler: XC8, optimization level 1
4. Configuration bits: FOSC=HS, WDT=OFF, PWRT=ON, LVP=OFF, PBADEN=OFF
5. Build & Program

### Command line (XC8)
```bash
xc8-cc -mcpu=18F4550 -O1 -o lock.hex main.c lcd.c keypad.c eeprom_mgr.c actuators.c
# Program with PICkit 3/4:
ipecmd -TPPK4 -P18F4550 -Flock.hex -M
```

---

## First-run flow

1. Power on — splash screen for 1.5 s
2. EEPROM has no PIN → "LOCK SETUP" screen appears
3. Press `#` to begin
4. Enter a 4–6 digit PIN, press `#`
5. Confirm the PIN again, press `#`
6. PIN saved to internal EEPROM — system enters LOCKED state

---

## Normal operation

| Key        | Action                                      |
|------------|---------------------------------------------|
| `0`–`9`    | Enter digit                                 |
| `#`        | Confirm / submit PIN                        |
| `*`        | Clear current entry / cancel               |
| Correct PIN | Relay opens for 5 seconds, green LED on    |
| Wrong PIN  | Red flash, attempt counter increments       |
| 3× wrong   | 30-second lockout, alarm buzzer             |

---

## EEPROM memory map

| Address | Content                    |
|---------|----------------------------|
| 0x00–0x05 | PIN digits (raw, up to 6) |
| 0x06    | PIN length                 |
| 0x07    | Failed attempt counter     |
| 0x08    | 0xA5 magic (PIN is set)    |

> ⚠️ The PIN is stored unencrypted in EEPROM. For higher security,
> apply a simple XOR mask or CRC before writing, and add code-protect
> configuration bits to prevent external readout.

---

## Configuration bits (in main.c pragmas)
```c
#pragma config FOSC   = HS      // 20 MHz crystal
#pragma config WDT    = OFF
#pragma config PWRT   = ON
#pragma config LVP    = OFF
#pragma config MCLRE  = ON
#pragma config PBADEN = OFF
#pragma config BOR    = OFF
```

---

## Project structure

```
Secure-Digital-Lock-System-PIC18F-/
├── main.c          State machine + main loop
├── lcd.c / .h      HD44780 LCD driver (4-bit)
├── keypad.c / .h   4×4 matrix keypad with debounce
├── eeprom_mgr.c/.h Internal EEPROM PIN storage
├── actuators.c/.h  Relay, buzzer, LEDs
├── Makefile        CLI build recipe
└── README.md       This file
```

---

# Author
**Ala Eddine Derbel**, *Embedded Systems Engineer*
