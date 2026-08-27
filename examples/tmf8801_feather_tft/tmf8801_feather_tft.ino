/*!
 * @file tmf8801_feather_tft.ino
 *
 * Display TMF8801 distance measurements on the built-in screen of an
 * Adafruit Feather ESP32-S2 TFT. Connect the sensor with a STEMMA QT cable.
 *
 * This example requires the Adafruit GFX Library and the Adafruit ST7735 and
 * ST7789 Library.
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution.
 */

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_TMF8801.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <SPI.h>

const uint16_t DISPLAY_WIDTH = 240;
const uint16_t DISPLAY_HEIGHT = 135;

Adafruit_ST7789 tft(TFT_CS, TFT_DC, TFT_RST);
Adafruit_TMF8801 tmf8801;
GFXcanvas16 framebuffer(DISPLAY_WIDTH, DISPLAY_HEIGHT);

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println(F("Adafruit TMF8801 Feather TFT demo"));

  // This pin supplies power to both the built-in TFT and the STEMMA QT port.
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);

  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  delay(10);

  tft.init(135, 240);
  tft.setRotation(3);
  framebuffer.setTextWrap(false);
  drawStartingScreen();

  if (!tmf8801.begin()) {
    showError(F("TMF8801 not found"));
    Serial.println(F("Could not find a valid TMF8801 sensor. Check wiring."));
    while (true) {
      delay(10);
    }
  }

  tmf8801.setIterations(900);       // 900,000 integration iterations
  tmf8801.setRepetitionPeriod(100); // About 10 measurements per second

  if (!tmf8801.startMeasuring()) {
    showError(F("Could not start"));
    Serial.println(F("Could not start TMF8801 measurements."));
    while (true) {
      delay(10);
    }
  }

  Serial.println(F("TMF8801 found. Measuring continuously."));
  drawMeasurementFrame();
  showFramebuffer();
}

void loop() {
  if (!tmf8801.dataReady()) {
    return;
  }

  tmf8801_result_t result;
  if (!tmf8801.readResult(&result)) {
    Serial.println(F("Could not read the measurement."));
    return;
  }

  updateDisplay(result);
  printSerialResult(result);
}

void drawStartingScreen() {
  framebuffer.fillScreen(ST77XX_BLACK);
  framebuffer.setTextColor(ST77XX_CYAN);
  framebuffer.setFont(&FreeSansBold18pt7b);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(39, 62);
  framebuffer.println(F("TMF8801"));
  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setFont(&FreeSans9pt7b);
  framebuffer.setCursor(52, 90);
  framebuffer.println(F("Starting sensor..."));
  showFramebuffer();
}

void drawMeasurementFrame() {
  framebuffer.fillScreen(ST77XX_BLACK);
  framebuffer.fillRect(0, 0, framebuffer.width(), 28, ST77XX_BLUE);
  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setFont(&FreeSansBold12pt7b);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(7, 21);
  framebuffer.print(F("TMF8801"));
  framebuffer.setFont(&FreeSans9pt7b);
  framebuffer.setCursor(172, 20);
  framebuffer.print(F("RANGE"));

  framebuffer.setFont();
  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setCursor(8, 34);
  framebuffer.print(F("DISTANCE"));

  framebuffer.drawRoundRect(7, 103, 226, 14, 4, ST77XX_WHITE);
  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setCursor(7, 122);
  framebuffer.print(F("20 mm"));
  framebuffer.setCursor(202, 122);
  framebuffer.print(F("2.5 m"));
}

void updateDisplay(const tmf8801_result_t &result) {
  drawMeasurementFrame();

  if (result.reliability == 0) {
    framebuffer.setTextColor(ST77XX_RED);
    framebuffer.setFont(&FreeSansBold18pt7b);
    framebuffer.setTextSize(1);
    framebuffer.setCursor(20, 76);
    framebuffer.print(F("NO TARGET"));
  } else {
    char distanceText[6];
    snprintf(distanceText, sizeof(distanceText), "%u", result.distance_mm);

    framebuffer.setFont(&FreeSansBold24pt7b);
    framebuffer.setTextColor(ST77XX_CYAN);
    framebuffer.setTextSize(1);
    framebuffer.setCursor(8, 82);
    framebuffer.print(distanceText);

    int16_t boundsX, boundsY;
    uint16_t textWidth, textHeight;
    framebuffer.getTextBounds(distanceText, 8, 82, &boundsX, &boundsY,
                              &textWidth, &textHeight);
    framebuffer.setFont(&FreeSansBold12pt7b);
    framebuffer.setTextColor(ST77XX_WHITE);
    framebuffer.setCursor(16 + textWidth, 81);
    framebuffer.print(F(" mm"));

    long rangeWidth = map(constrain(result.distance_mm, 20, 2500), 20, 2500,
                          1, 222);
    framebuffer.fillRoundRect(9, 105, rangeWidth, 10, 3, ST77XX_CYAN);
  }

  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setFont();
  framebuffer.setTextSize(1);
  framebuffer.setCursor(8, 91);
  framebuffer.print(F("STATUS: "));
  framebuffer.print(measurementStatusText(result.status));

  showFramebuffer();
}

void printSerialResult(const tmf8801_result_t &result) {
  if (result.reliability == 0) {
    Serial.println(F("No object detected"));
    return;
  }

  Serial.print(F("Distance: "));
  Serial.print(result.distance_mm);
  Serial.print(F(" mm   Reliability: "));
  Serial.print(result.reliability);
  Serial.print(F("/"));
  Serial.print(TMF8801_RESULT_RELIABILITY_LEVELS);
  Serial.print(F(" ("));
  Serial.print(reliabilityText(result.reliability));
  Serial.print(F(")   Status: "));
  Serial.println(measurementStatusText(result.status));
}

const __FlashStringHelper *reliabilityText(uint8_t reliability) {
  if (reliability >= TMF8801_RESULT_RELIABILITY_HIGH_MIN) {
    return F("HIGH");
  }
  return F("LOW");
}

const __FlashStringHelper *
measurementStatusText(tmf8801_measurement_status_t status) {
  switch (status) {
  case TMF8801_MEASUREMENT_NOT_INTERRUPTED:
    return F("NORMAL");
  case TMF8801_MEASUREMENT_INTERRUPTED_BY_GPIO:
    return F("GPIO INTERRUPT");
  case TMF8801_MEASUREMENT_STATUS_RESERVED_1:
  case TMF8801_MEASUREMENT_STATUS_RESERVED_3:
    return F("RESERVED");
  default:
    return F("UNKNOWN");
  }
}

void showError(const __FlashStringHelper *message) {
  framebuffer.fillScreen(ST77XX_BLACK);
  framebuffer.setTextColor(ST77XX_RED);
  framebuffer.setFont(&FreeSansBold12pt7b);
  framebuffer.setTextSize(1);
  framebuffer.setCursor(24, 48);
  framebuffer.println(F("SENSOR ERROR"));
  framebuffer.setTextColor(ST77XX_WHITE);
  framebuffer.setFont(&FreeSans9pt7b);
  framebuffer.setCursor(18, 82);
  framebuffer.println(message);
  showFramebuffer();
}

void showFramebuffer() {
  tft.drawRGBBitmap(0, 0, framebuffer.getBuffer(), framebuffer.width(),
                    framebuffer.height());
}
