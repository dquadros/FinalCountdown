/**
 * @file epaper.c
 * @author Daniel Quadros
 * @brief e-paper display control
 * @version 0.1
 * @date 2025-09-05
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "FinalCountdown.h"

#include "font_12x16.h"

// Controller Commands
#define CMD_SWRESET			 	      0x12
#define CMD_DRV_OUT_CTRL 	 	    0x01
#define CMD_DATA_ENTRY_MODE	    0x11
#define CMD_SET_RAM_XADDR	 	    0x44
#define CMD_SET_RAM_YADDR	 	    0x45
#define CMD_BORDER_WAVE		 	    0x3C
#define CMD_READ_TEMPERATURE 	  0x18
#define CMD_DISP_UPD_CTL 	 	    0x22
#define CMD_ACTIVE_DISP_UPD_SEQ	0x20
#define CMD_SET_RAM_XADDR_COUNT 0x4E
#define CMD_SET_RAM_YADDR_COUNT 0x4F
#define CMD_DEEP_SLEEP 			    0x10
#define CMD_WRITE_RAM 			    0x24
#define CMD_WRITE_REDRAM 		    0x26

#define TIME_UPDATE 180000


#define EPAPER_RST_HIGH() gpio_put(EPAPER_RST_PIN, true)
#define EPAPER_RST_LOW() gpio_put(EPAPER_RST_PIN, false)
#define EPAPER_CS_HIGH() gpio_put(SPI_SS_PIN, true)
#define EPAPER_CS_LOW() gpio_put(SPI_SS_PIN, false)
#define EPAPER_DC_HIGH() gpio_put(EPAPER_DC_PIN, true)
#define EPAPER_DC_LOW() gpio_put(EPAPER_DC_PIN, false)
#define EPAPER_BUSY() gpio_get(EPAPER_BUSY_PIN)

#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 200
#define LINE_BYTES (SCREEN_HEIGHT/8)
uint8_t screen[SCREEN_WIDTH*LINE_BYTES];

// Flag to signal when display is powered off
uint8_t hibernating = true;


// Initialize the pins connected to the display
static void pin_init() {
  // Digital pins
  gpio_init(EPAPER_RST_PIN);
  gpio_set_dir(EPAPER_RST_PIN, GPIO_OUT);
  EPAPER_RST_HIGH();
  gpio_init(EPAPER_DC_PIN);
  gpio_set_dir(EPAPER_DC_PIN, GPIO_OUT);  
  EPAPER_DC_LOW();
  gpio_init(SPI_SS_PIN);
  gpio_set_dir(SPI_SS_PIN, GPIO_OUT);  
  EPAPER_CS_HIGH();
  gpio_init(EPAPER_BUSY_PIN);
  gpio_set_dir(EPAPER_BUSY_PIN, GPIO_IN);  

  // SPI
  spi_init(SPI_ID, 10000000);
  spi_set_format (SPI_ID, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
  gpio_set_function(SPI_SCLK_PIN, GPIO_FUNC_SPI);
  gpio_set_function(SPI_MOSI_PIN, GPIO_FUNC_SPI);
}

// Wait display controller not busy
static uint8_t epd_wait_busy()
{
  uint32_t timeout = 0;
  while (EPAPER_BUSY()) {
    timeout++;
    if (timeout > 40000) {
      return true;
    }
    sleep_ms(1);
  }
  return false;
}

// Send a command to the display controller
static void epd_write_cmd(uint8_t cmd)
{
	EPAPER_DC_LOW();
	EPAPER_CS_LOW();
  spi_write_blocking(SPI_ID, &cmd, 1);
	EPAPER_CS_HIGH();
	EPAPER_DC_HIGH();
}

// Send a data byte to the display controller
static void epd_write_data(uint8_t data)
{
	EPAPER_CS_LOW();
  spi_write_blocking(SPI_ID, &data, 1);
	EPAPER_CS_HIGH();
}


// Reset the display
static void epd_reset() {
	EPAPER_RST_LOW();
	sleep_ms(50);
	EPAPER_RST_HIGH();
	sleep_ms(50);
	hibernating = false;
}

// Turn on the display
static uint8_t epd_power_on() {
	epd_write_cmd(CMD_DISP_UPD_CTL);
	epd_write_data(0xf8);
	epd_write_cmd(CMD_ACTIVE_DISP_UPD_SEQ);
	return epd_wait_busy();
}

// Turn off the display
uint8_t epd_power_off(void)
{
	epd_write_cmd(CMD_DISP_UPD_CTL);
	epd_write_data(0x83);
	epd_write_cmd(CMD_ACTIVE_DISP_UPD_SEQ);
	if (epd_wait_busy()) {
		return true;
	}
	epd_write_cmd(CMD_DEEP_SLEEP);
	epd_write_data(0x01);
	hibernating = true;
	return false;
}


// Set the screen write pointer
static void epd_setpos(uint16_t x, uint16_t y)
{
	uint8_t _x;
	uint16_t _y;

	_x = x / 8;
	_y = 199 - y;

	epd_write_cmd(CMD_SET_RAM_XADDR_COUNT);
	epd_write_data(_x);

	epd_write_cmd(CMD_SET_RAM_YADDR_COUNT);
	epd_write_data(_y & 0xff);
	epd_write_data((_y >> 8) & 0x01);
}

// Init display
// (values from WeAct Studio example)
uint8_t epd_init() {
  pin_init();

  memset(screen, 0xFF, sizeof(screen));

	if (hibernating) {
		epd_reset();
	}
	if (epd_wait_busy()) {
		return true;
	}

	epd_write_cmd(CMD_SWRESET);
	sleep_ms(10);
	if (epd_wait_busy()) {
		return true;
	}

	epd_write_cmd(CMD_DRV_OUT_CTRL);
	epd_write_data(0xC7);
	epd_write_data(0x00);
	epd_write_data(0x01);

	epd_write_cmd(CMD_DATA_ENTRY_MODE);
	epd_write_data(0x01);

	epd_write_cmd(CMD_SET_RAM_XADDR);
	epd_write_data(0x00);
	epd_write_data(0x18);

	epd_write_cmd(CMD_SET_RAM_YADDR);
	epd_write_data(0xC7);
	epd_write_data(0x00);
	epd_write_data(0x00);
	epd_write_data(0x00);

	epd_write_cmd(CMD_BORDER_WAVE);
	epd_write_data(0x05);

	epd_write_cmd(CMD_READ_TEMPERATURE);
	epd_write_data(0x80);

	epd_setpos(0,0);

	return epd_power_on();
}

// Send the screen content to the display controller
static void epd_send_screen() {
  EPAPER_CS_LOW();
  spi_write_blocking(SPI_ID, screen, sizeof(screen));
  EPAPER_CS_HIGH();
}

// Update image on the display
void epd_refresh() {
	// Turnon if necessary
	if (hibernating) {
		epd_init();
	}

	// Fill RED RAM
	epd_setpos(0, 0);
	epd_write_cmd(CMD_WRITE_REDRAM);
	epd_send_screen();

	// Fill BW RAM
	epd_setpos(0, 0);
	epd_write_cmd(CMD_WRITE_RAM);
	epd_send_screen();

	// Update the display
	epd_write_cmd(CMD_DISP_UPD_CTL);
	epd_write_data(0xF4);
	epd_write_cmd(CMD_ACTIVE_DISP_UPD_SEQ);

	epd_wait_busy();
}

// Write text to screen
// lin: 0 to 11
// col: 0 to 15
void epd_text(int lin, int col, char *text) {
  int y = 16*lin*LINE_BYTES;
  int x = col + (col >> 1);
  bool par = (col & 1) == 0;
  while (*text) {
    const uint8_t *p = &console_font_12x16[*text << 5];
    for (int lg = 0; lg < 16; lg++) {
			int pos = y + lg*LINE_BYTES + x; 
			//printf ("y=%d x=%d pos=%d par=%s\n", y, x, pos, par?"PAR":"IMPAR");
      if (par) {
        screen[pos] = *p++;
        screen[pos+1] &= 0x0F;
        screen[pos+1] |= *p++;
      } else {
        screen[pos] &= 0xF0;
        screen[pos] |= *p >> 4;
        screen[pos+1] = ((*p << 4) & 0xF0)|((*(p+1) >> 4) & 0x0F);
        p+=2;
      }
    }
    text++;
		x += par? 1 : 2;
    par = !par;
  }
}

// Draw a point
// x: 0 to 199
// y: 0 to 199
void epd_plot(int x, int y) {
	int pos = y*LINE_BYTES+(x/8);
	screen[pos] &= ~(1 << (7 - x&7));
}

// Draw a circle
void epd_circle(int x0, int y0, int r) {
	int ix, iy;
	int d, dh, dd;

	d = 3 - (r << 1);
	dh = 6;
	dd = 10 - (r << 2);
	for (ix =0, iy = r; ix <= iy; ix++) {
		epd_plot(x0+ix, y0+iy);
		epd_plot(x0+iy, y0+ix);
		epd_plot(x0+ix, y0-iy);
		epd_plot(x0+iy, y0-ix);
		epd_plot(x0-ix, y0+iy);
		epd_plot(x0-iy, y0+ix);
		epd_plot(x0-ix, y0-iy);
		epd_plot(x0-iy, y0-ix);
		if (d >= 0) {
			iy--;
			d += dd;
			dd += 8;
		} else {
			d += dh;
			dd +=4;
		}
		dh += 4;
	}
}

// Draw a line
void epd_line(int x1, int y1, int x2, int y2) {
	int x, y, dx, dy, d, incx, incy, stepy;

	if (x1 > x2) {
		x = x2; x2 = x1; x1 = x;
		y = y2; y2 = y1; y1 = y;
	}	else {
		x = x1;
		y = y1;
	}

	dx = x2-x1;
	dy = y2-y1;
	if (dy < 0) {
		dy = -dy;
		stepy = -1;
	} else {
		stepy = 1;
	}

	if (dx >= dy) {
		d = (dy << 1) - dx;
		incx = dy << 1;
		incy = incx - (dx << 1);
		while (true) {
			epd_plot(x,y);
			if (x == x2) {
				break;
			}
			x++;
			if (d >= 0) {
				y += stepy;
				d += incy;
			} else {
				d += incx;
			}
		}
	} else {
		d = (dx << 1) - dy;
		incy = dx << 1;
		incx = incy - (dy << 1);
		while (true) {
			epd_plot(x,y);
			if (y == y2) {
				break;
			}
			y += stepy;
			if (d >= 0) {
				x++;
				d += incx;
			} else {
				d += incy;
			}
		}
	}
}
