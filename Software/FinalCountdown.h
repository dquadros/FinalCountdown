/**
 * @file FinalCountdown.h
 * @author Daniel Quadros
 * @brief Global Definitions
 * @version 0.1
 * @date 2025-09-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

// Connections
#define I2C_ID i2c1
#define I2C_SCL_PIN 7
#define I2C_SDA_PIN 6

#define SPI_ID spi0
#define SPI_SCLK_PIN 2
#define SPI_MISO_PIN 4
#define SPI_MOSI_PIN 3
#define SPI_SS_PIN 5

#define EPAPER_RST_PIN  26
#define EPAPER_DC_PIN   27
#define EPAPER_BUSY_PIN 1

#define TECLA_PIN 0

// Date and time structure
typedef struct {
  uint8_t day;
  uint8_t month;
  uint16_t year;
  uint8_t hour;
  uint8_t minutes;
  uint8_t seconds;
} RTC_DATE;

// Configuration
typedef struct {
  RTC_DATE eventDate;
  uint8_t check;
} CONFIG;

// rtc.c routines
void rtc_init (void);
int rtc_read_eeprom(uint16_t addr, uint8_t *buffer, int bufsize);
int rtc_write_eeprom(uint16_t addr, uint8_t *buffer, int bufsize);
int rtc_read_date(RTC_DATE *pDate);
int rtc_write_date(RTC_DATE *pDate);

// power.c routines
int powman_off_for_ms(uint64_t duration_ms);
int powman_off_until_time(uint64_t abs_time_ms);
void powman_init(uint64_t abs_time_ms);

// epaper.c routines
uint8_t epd_power_off(void);
uint8_t epd_init(void);
void epd_refresh(void);
void epd_text(int lin, int col, char *text);
void epd_plot(int x, int y);
void epd_circle(int x0, int y0, int r);
void epd_line(int x1, int y1, int x2, int y2);
