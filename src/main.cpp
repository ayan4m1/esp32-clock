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
#define LCD_HOST    HSPI
#define PIN_NUM_MISO 25
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  19
#define PIN_NUM_CS   22
#define PIN_NUM_DC   21
#define PIN_NUM_RST  18
#define PIN_NUM_BCKL 5

using bus_type = tft_spi_ex<LCD_HOST,PIN_NUM_CS,PIN_NUM_MOSI,PIN_NUM_MISO,PIN_NUM_CLK,SPI_MODE0>; 
using lcd_type = ili9341<PIN_NUM_DC,PIN_NUM_RST,PIN_NUM_BCKL,bus_type,LCD_ROTATION,LCD_BACKLIGHT_HIGH,200,200>;
using lcd_color = color<typename lcd_type::pixel_type>;

lcd_type lcd;

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

bool colorDirection = true;
uint16_t colorStep = 0;
hsv_pixel<24U> bgColor(true, 0, 1, 1);

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

void setup() {
  Serial.begin(115200);

  lcd.initialize();
  lcd.clear(lcd.bounds());
  // lcd.fill(lcd.bounds(), COLOR_BG);

  WiFi.begin(WIFI_SSID, WIFI_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);

  // xTaskCreatePinnedToCore(handleWiFiConnection, "wifi", 20000, NULL, 0, NULL, 1);
  // xTaskCreatePinnedToCore(handleNtpSync, "ntp", 20000, NULL, 0, NULL, 1);
}

void drawText(const char* text, const uint8_t size) {
  open_text_info textInfo;
  textInfo.font = &Telegrama;
  textInfo.text = text;
  textInfo.scale = Telegrama.scale(32);
  srect16 textPos = textInfo.font->measure_text(ssize16::max(), spoint16::zero(), textInfo.text, textInfo.scale).bounds();
  textPos.center_inplace((srect16)lcd.bounds());

  draw::filled_rectangle(lcd, textPos.inflate(ssize16(1, 1)), bgColor);
  rgb_pixel<16U> bgRgb;
  gfx::convert<hsv_pixel<24U>, rgb_pixel<16U>>(bgColor, &bgRgb);
  draw::text(lcd, textPos, textInfo, COLOR_FG, bgRgb);
}

void drawIcons() {
  srect16 iconBg = srect16(0, 0, 64, 32);
  draw::filled_rectangle(lcd, iconBg, bgColor);
}

void drawClock() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println(F("Failed to obtain time"));
    return;
  }

  char timeString[9];
  strftime(timeString, 9, "%H:%M:%S", &timeinfo);
  Serial.println(timeString);
  Serial.println();

  drawText(timeString, 32);
}

void loop() {
  if (colorDirection) {
    colorStep++;
  } else {
    colorStep--;
  }
  if (colorStep == TIME_STEPS || colorStep == 0) {
    colorDirection = !colorDirection;
  }
  bgColor.channelr<channel_name::H>(colorStep / TIME_STEPS);

  // drawIcons();
  rgb_pixel<16U> bgRgb;
  gfx::convert<hsv_pixel<24U>, rgb_pixel<16U>>(bgColor, &bgRgb);
  draw::filled_rectangle(lcd, lcd.bounds(), bgRgb);
  drawClock();
  delay(1000);
}
