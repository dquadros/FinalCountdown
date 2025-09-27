/**
 * @file rtc.c
 * @author Daniel Quadros
 * @brief tiny RTC (RTC DS1307 and 27C32 EEPROM) Access
 * @version 0.1
 * @date 2025-09-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "string.h"
#include "malloc.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"

#include "FinalCountdown.h"

// I2C Configuration
#define BAUD_RATE 100000 // standard 100KHz

// EEProm
#define EEPROM_ADDR 0x50
#define PAGE_SIZE 32

// DS1307
#define DS1307_ADDR 0x68
#define DS1307_NREGS 8

// Initialization
void rtc_init() {
  // Set up I2C
  i2c_init (I2C_ID, BAUD_RATE);
  
  // Set up the I2C pins
  gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
}

// Read data from EEPROM
int rtc_read_eeprom(uint16_t addr, uint8_t *buffer, int bufsize) {
  uint8_t buffAddr[2];

  buffAddr[0] = addr >> 8;
  buffAddr[1] = addr & 0xFF;
  int ret = i2c_write_blocking (I2C_ID, EEPROM_ADDR, buffAddr, 2, true);
  if (ret == 2) {
    ret = i2c_read_blocking(I2C_ID, EEPROM_ADDR, buffer, bufsize, false);
  } else {
    return false; // erro
  }
  return ret == bufsize;
}

// Write data to EEPROM
int rtc_write_eeprom(uint16_t addr, uint8_t *buffer, int bufsize) {

  uint8_t *aux = (uint8_t *) malloc(bufsize+2);
  aux[0] = addr >> 8;
  aux[1] = addr & 0xFF;
  memcpy(aux+2, buffer, bufsize);

  int ret = i2c_write_blocking (I2C_ID, EEPROM_ADDR, aux, bufsize+2, false);

  if (ret == (bufsize+2)) {
    // Wait for write to complete
    // 24C32 will acknoledge address only when writting finished
    while (i2c_read_blocking(I2C_ID, EEPROM_ADDR, aux, 1, false) != 1) {
      sleep_ms(1);
    }
  }

  free(aux);

  return ret == (bufsize+2);
}

// Convert BCD value to binary
static int8_t from_bcd(uint8_t val) {
  return (val >> 4)*10 + (val & 0x0F);
}

// Convert binary value to BCD
static int8_t to_bcd(uint8_t val) {
  return ((val / 10) << 4) + (val % 10);
}

// Read current date and time
int rtc_read_date(RTC_DATE *pDate) {
  uint8_t DS1307_regs[DS1307_NREGS];
  uint8_t buffAddr[1];

  buffAddr[0] = 0;
  int ret = i2c_write_blocking (I2C_ID, DS1307_ADDR, buffAddr, 1, true);
  if (ret == 1) {
    ret = i2c_read_blocking(I2C_ID, DS1307_ADDR, DS1307_regs, DS1307_NREGS, false);
    if (ret == DS1307_NREGS) {
      if (DS1307_regs[0] & 0x80) {
        return false; // relogio parado
      }
      pDate->day = from_bcd(DS1307_regs[4]);
      pDate->month = from_bcd(DS1307_regs[5]);
      pDate->year = 2000 + from_bcd(DS1307_regs[6]);
      if (DS1307_regs[2] & 0x40) {
        uint8_t hora = from_bcd(DS1307_regs[2] &= 0x1F);  // 12h
        if (DS1307_regs[4] & 0x20) {
          pDate->hour = (hora == 12) ? hora : (hora+12); // PM
        } else {
          pDate->hour = (hora == 12) ? 0 : hora; // AM
        } 
      } else {
        pDate->hour = from_bcd(DS1307_regs[2] &= 0x3F);   // 24h
      }
      pDate->minutes = from_bcd(DS1307_regs[1]);
      pDate->seconds = from_bcd(DS1307_regs[0]);
      return true;
    }
  }
  return false; // error
}

// Write current date and time
int rtc_write_date(RTC_DATE *pDate) {
  uint8_t DS1307_regs[DS1307_NREGS+1];

  DS1307_regs[0] = 0;
  DS1307_regs[1] = to_bcd(pDate->seconds);
  DS1307_regs[2] = to_bcd(pDate->minutes);
  DS1307_regs[3] = to_bcd(pDate->hour);
  DS1307_regs[4] = 1;
  DS1307_regs[5] = to_bcd(pDate->day);
  DS1307_regs[6] = to_bcd(pDate->month);
  DS1307_regs[7] = to_bcd(pDate->year - 2000);
  DS1307_regs[8] = 0;
  return i2c_write_blocking (I2C_ID, DS1307_ADDR, DS1307_regs, DS1307_NREGS+1, false);
}
