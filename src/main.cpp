#include <Arduino.h>
#include <WiFi.h>
#include <sunset.h>
#include <time.h>
#include <gfx.hpp>
#include <ili9341.hpp>
#include <tft_io.hpp>

#include "Inter.hpp"

using namespace arduino;
using namespace gfx;

#define LCD_BACKLIGHT_HIGH false
#define LCD_ROTATION 0
#define LCD_HOST    VSPI
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

#define WIFI_SSID "bar"
#define WIFI_PSK "wi9NNYara"
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC -18000
#define DST_OFFSET_SEC 3600

void setup() {
  Serial.begin(115200);
  // while (!Serial) { delay(100); }

  Serial.println(F("Init display..."));
  lcd.initialize();
  lcd.clear(lcd.bounds());
  lcd.fill(lcd.bounds(), lcd_color::burly_wood);

  Serial.println(F("Connecting to WiFi"));
  WiFi.begin(WIFI_SSID, WIFI_PSK);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println(F(""));
  Serial.println(F("Connected!"));

  Serial.println(F("Getting time from NTP..."));
  configTime(GMT_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER);
}

void drawClock() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println(F("Failed to obtain time"));
    return;
  }

  char timeString[9];
  strftime(timeString, 9, "%H:%m:%S", &timeinfo);
  Serial.println(timeString);
  Serial.println();

  open_text_info textInfo;
  textInfo.font = &Inter;
  textInfo.text = timeString;
  textInfo.scale = 32;
  srect16 textPos = textInfo.font->measure_text(ssize16::max(), spoint16::zero(), textInfo.text, textInfo.scale).bounds();
  textPos.center_inplace((srect16)lcd.bounds());

  draw::text(lcd, textPos, textInfo, lcd_color::white, lcd_color::burly_wood);
}

void loop() {
  drawClock();
  delay(1000);
}
