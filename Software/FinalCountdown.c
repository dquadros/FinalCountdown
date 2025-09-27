/**
 * @file FinalCountdown.c
 * @author Daniel Quadros
 * @brief Main module fot the FinalCountdown project
 * @version 0.1
 * @date 2025-09-04
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/gpio.h"

#include "FinalCountdown.h"

const double PI = 3.14159265;

// Configuration
CONFIG config;

// Initializations
static int init() {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    gpio_init(TECLA_PIN);
    gpio_set_dir(TECLA_PIN, GPIO_IN);
    gpio_pull_up(TECLA_PIN);

    // LED is on while wake
    gpio_put(PICO_DEFAULT_LED_PIN, false);

    powman_init(1704067200000);
    rtc_init();
    epd_init();
}

// Calculate the configuration checksum
static uint8_t checkConfig() {
    uint8_t *pCfg = (uint8_t *) &config;
    uint8_t check = 0;
    for (int i = 0; i < sizeof(CONFIG); i++) {
        check ^= *pCfg++;
    }
    return check;
}

// Read current configuration
// Returns True if checksum ok
static int load_cfg() {
    rtc_read_eeprom(0, (uint8_t *)&config, sizeof(config));
    return checkConfig() == 0;
}

// Save current configuration
static void save_cfg() {
    config.check = 0;
    config.check = checkConfig();
    rtc_write_eeprom(0, (uint8_t *)&config, sizeof(config));
}

// Check if a BCD value is in range
static int check_val(char *p, int min_val, int max_val) {
    int val = (p[0] - '0')*10 + p[1] - '0';
    return (val >= min_val) && (val <= max_val);
}

// Read data and time from stdio
static void get_date_time(RTC_DATE *datetime) {
    char buf[10];
    int pos = 0;

    while (1) {
        int c = stdio_getchar();
        if ((c == '\r') && (pos == 10)){
            if (!check_val(buf, 1, 31) || !check_val(buf+2, 1, 12) ||
                !check_val(buf+6, 0, 23) || !check_val(buf+8, 0, 59)) {
                    stdio_putchar_raw(0x07); // BEEP
                }
            datetime->day = (buf[0] - '0')*10 + buf[1] - '0';
            datetime->month = (buf[2] - '0')*10 + buf[3] - '0';
            datetime->year = 2000 + (buf[4] - '0')*10 + buf[5] - '0';
            datetime->hour = (buf[6] - '0')*10 + buf[7] - '0';
            datetime->minutes = (buf[8] - '0')*10 + buf[9] - '0';
            datetime->seconds = 0;
            stdio_putchar_raw('\r');
            stdio_putchar_raw('\n');
            return;
        } else if ((c == 0x08) && (pos > 0)) {
            stdio_putchar_raw(0x08);
            stdio_putchar_raw(' ');
            stdio_putchar_raw(0x08);
            pos--;
        } else if ((c >= '0') && (c <= '9') && (pos < 10)) {
            stdio_putchar_raw(c);
            buf[pos++] = c;
        } else {
            stdio_putchar_raw(0x07); // BEEP
        }
    }
}

// Configure the device
static void configure() {
    RTC_DATE datetime;

    // Wait USB connection
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    // Do the configuration
    printf("\nConfiguraton\n\n");
    printf("Current date and time (DDMMAAHHMM):\n");
    get_date_time(&datetime);
    rtc_write_date(&datetime);
    printf("Event date and time (DDMMAAHHMM):\n");
    get_date_time(&config.eventDate);
    save_cfg();
    printf("\n");
}


// Calculates the epoch (seconds from 1/1/1970) for a date/time
time_t epoch (RTC_DATE *pData) {
    struct tm tmDate;

    tmDate.tm_hour = pData->hour;
    tmDate.tm_mday = pData->day;
    tmDate.tm_min = pData->minutes;
    tmDate.tm_mon = pData->month - 1;
    tmDate.tm_sec = pData->seconds;
    tmDate.tm_year = pData->year - 1900;
    tmDate.tm_isdst = 0;
    return mktime(&tmDate);
}


// Main Program
int main() {

    RTC_DATE date;
    char buf[17];
    double alfa, beta;

    init();

    // Check if configuration neeeded or requested
    if ((gpio_get(TECLA_PIN) == 0) || !rtc_read_date(&date) || !load_cfg()) {
        configure();
        rtc_read_date(&date);
    }

    // Update the display
    time_t now = epoch(&date);
    time_t event = epoch(&config.eventDate);
    if (now < event) {
        unsigned long remaining = ((ulong) event - (ulong) now)/60L;  // minutos que faltam
        if (remaining < 24L*60L) {
            if (date.day == config.eventDate.day) {
             epd_text(0, 4, "\x90 TODAY!");
            }
            sprintf(buf, "%ld hours", remaining/60L);
        } else {
            sprintf(buf, "%ld days", remaining/(24L*60L));
        }
        epd_text(1, (16-strlen(buf))/2, buf);
        epd_text(2, 3, "REMAINING");
    } else {
        epd_text(1, 2, "PAST EVENT");
    }
    sprintf(buf, "%02d/%02d/%04d", date.day, date.month, date.year);
    epd_text(11, 3, buf);
    const int CIRCLE_X = 100;
    const int CIRCLE_Y = 116;
    const int CIRCLE_R = 54;
    epd_circle(CIRCLE_X, CIRCLE_Y, CIRCLE_R);
    for (alfa = 0; alfa < 2*PI; alfa += PI/4) {
        epd_plot (round(CIRCLE_X+(CIRCLE_R-2)*sin(alfa)), round(CIRCLE_Y-(CIRCLE_R-2)*cos(alfa)));
        epd_plot (round(CIRCLE_X+(CIRCLE_R-3)*sin(alfa)-1), round(CIRCLE_Y-(CIRCLE_R-3)*cos(alfa)));
        epd_plot (round(CIRCLE_X+(CIRCLE_R-3)*sin(alfa)), round(CIRCLE_Y-(CIRCLE_R-3)*cos(alfa)));
        epd_plot (round(CIRCLE_X+(CIRCLE_R-4)*sin(alfa)+1), round(CIRCLE_Y-(CIRCLE_R-3)*cos(alfa)));
        epd_plot (round(CIRCLE_X+(CIRCLE_R-4)*sin(alfa)), round(CIRCLE_Y-(CIRCLE_R-4)*cos(alfa)));
    }
    alfa = (date.hour*60.0+date.minutes)/(12*60.0)*2*PI;
    epd_line (CIRCLE_X, CIRCLE_Y, CIRCLE_X+(CIRCLE_R-12)*sin(alfa), CIRCLE_Y-(CIRCLE_R-12)*cos(alfa));
    beta = date.minutes/60.0*2*PI;
    epd_line (CIRCLE_X, CIRCLE_Y, CIRCLE_X+(CIRCLE_R-8)*sin(beta), CIRCLE_Y-(CIRCLE_R-8)*cos(beta));
    epd_refresh();

    // Sleep until next quart hour
    long off_time = (((date.minutes/15)+1)*15*60L-(date.minutes*60+date.seconds))*1000L;
    epd_power_off();
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    int rc = powman_off_for_ms(off_time);
    hard_assert(rc == PICO_OK);

    hard_assert(false); // should never get here!
    return 0;
}