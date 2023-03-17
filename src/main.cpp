#include <Arduino.h>
#include <WiFi.h>
#include <sunset.h>
#include <time.h>

#include <gfx.hpp>
#include <ili9341.hpp>
#include <tft_io.hpp>

#include "Telegrama.hpp"

using namespace arduino;
using namespace gfx;

#define LCD_BACKLIGHT_HIGH false
#define LCD_ROTATION 2
#define LCD_HOST HSPI
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK 19
#define PIN_NUM_CS 22
#define PIN_NUM_DC 21
#define PIN_NUM_RST 18
#define PIN_NUM_BCKL 5

using bus_type = tft_spi_ex<LCD_HOST, PIN_NUM_CS, PIN_NUM_MOSI, PIN_NUM_MISO,
                            PIN_NUM_CLK, SPI_MODE0>;
using lcd_type = ili9341<PIN_NUM_DC, PIN_NUM_RST, PIN_NUM_BCKL, bus_type,
                         LCD_ROTATION, LCD_BACKLIGHT_HIGH, 200, 200>;
using lcd_color = color<typename lcd_type::pixel_type>;

lcd_type lcd;

using bmp_type = bitmap<decltype(lcd)::pixel_type>;
using bmp_color = color<typename bmp_type::pixel_type>;

volatile bool wifiConnected;
volatile bool ntpSynced;

#define WIFI_SSID "bar"
#define WIFI_PSK "wi9NNYara"

#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -18000
#define DST_OFFSET_SEC 3600

#define COLOR_FG lcd_color::white
#define COLOR_BG lcd_color::dark_slate_blue

#define TIME_STEPS 512.0

volatile bool colorDirection = true;
volatile uint16_t colorStep = 0;
hsv_pixel<24U> bgHsv(true, 0, 1, 1);

constexpr static const size16 bmpSize(120, 60);
uint8_t bmpBuf[bmp_type::sizeof_buffer(bmpSize)];

void handleWiFiConnection(void* params) {
  WiFi.begin(WIFI_SSID, WIFI_PSK);

  while (true) {
    wifiConnected = WiFi.status() == WL_CONNECTED;

    if (!wifiConnected) {
      WiFi.reconnect();
      while (WiFi.status() != WL_CONNECTED) {
        delay(100);
      }
      wifiConnected = true;
    }

    delay(10000);
  }
}

void handleNtpSync(void* params) {
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
  ntpSynced = true;
}

void handleColorStep(void* params) {
  while (true) {
    if (colorDirection) {
      colorStep++;
    } else {
      colorStep--;
    }
    if (colorStep == TIME_STEPS || colorStep == 0) {
      colorDirection = !colorDirection;
    }
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);

  lcd.initialize();
  lcd.clear(lcd.bounds());

  WiFi.begin(WIFI_SSID, WIFI_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);

  xTaskCreatePinnedToCore(handleColorStep, "color", 10000, NULL, 1, NULL, 1);
  // xTaskCreatePinnedToCore(handleWiFiConnection, "wifi", 20000, NULL, 0, NULL, 1);
  // xTaskCreatePinnedToCore(handleNtpSync, "ntp", 20000, NULL, 0, NULL, 1);
}

void drawText(const char* text, const uint8_t size, rgb_pixel<16U> fg, rgb_pixel<16U> bg) {
  open_text_info textInfo;
  textInfo.font = &Telegrama;
  textInfo.text = text;
  textInfo.scale = Telegrama.scale(32);
  textInfo.transparent_background = false;
  srect16 textPos = textInfo.font
                        ->measure_text(ssize16::max(), spoint16::zero(),
                                       textInfo.text, textInfo.scale)
                        .bounds();

  bmp_type bmp(size16(textPos.width(), textPos.height()), bmpBuf);

  bmp.clear(bmp.bounds());
  draw::text(bmp, textPos, textInfo, fg, bg);
  draw::bitmap(lcd, bmp.bounds().center(lcd.bounds()), bmp, bmp.bounds());
}

void drawIcons() {
  srect16 iconBg = srect16(0, 0, 64, 32);
  draw::filled_rectangle(lcd, iconBg, bgHsv);
}

void drawClock(rgb_pixel<16U> bg) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println(F("Failed to obtain time"));
    return;
  }

  char timeString[9];
  strftime(timeString, 9, "%H:%M:%S", &timeinfo);
  Serial.println(timeString);
  Serial.println();

  drawText(timeString, 32, COLOR_FG, bg);
}

void loop() {
  bgHsv.channelr<channel_name::H>(colorStep / TIME_STEPS);
  rgb_pixel<16U> bgRgb;
  gfx::convert<hsv_pixel<24U>, rgb_pixel<16U>>(bgHsv, &bgRgb);

  // drawIcons();
  draw::filled_rectangle(lcd, lcd.bounds(), bgRgb);
  drawClock(bgRgb);
}
