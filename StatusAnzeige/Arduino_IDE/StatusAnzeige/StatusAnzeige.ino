/*
  StatusAnzeige To Go
  Waveshare ESP32-S3-Touch-LCD-2 (SKU 29667)

  Funktionen:
  - ST7789T3 Display 240x320
  - PNG-Dateien direkt von microSD
  - 13 adressierbare LEDs (WS2812/kompatibel)
  - externer Taster
  - WLAN + Fallback-AP
  - BLE-Gerätename / Advertising
  - FTP-Zugriff auf die SD-Karte
  - HTTP-API zur Statussteuerung
  - SD-Firmwareupdate mit /firmware/update.flag
  - Regenbogen-Welle beim Start
  - Menü: dunkelblaues Pulsieren #0000AA
  - Bestätigung: kurzer weißer Blitz
  - Statusfarben aus /config/config.json
  - letzter Status wird auf SD gespeichert

  Hardware-Pins des Waveshare-Boards:
    SPI gemeinsam: SCLK 39, MOSI 38, MISO 40
    LCD CS 45, DC 42, BL 1
    SD CS 41
    Touch/IMU I2C: SDA 48, SCL 47 (hier bewusst nicht benutzt)
    Externe LEDs: GPIO17
    Externer Taster: GPIO18 gegen GND

  WICHTIG:
  - Kamera darf nicht gleichzeitig benutzt werden, wenn GPIO17/18 für LED/Taster
    verwendet werden.
  - FTP ist unverschlüsselt. Nur im vertrauenswürdigen LAN verwenden.
*/

// We use physical ESP32-S3 GPIO numbers directly.
// Disable Arduino-ESP32 board pin remapping before Arduino.h is included.
// This also prevents macro collisions with Arduino_GFX methods named
// pinMode(), digitalWrite() and digitalRead().
#ifndef BOARD_USES_HW_GPIO_NUMBERS
#define BOARD_USES_HW_GPIO_NUMBERS 1
#endif

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <FS.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <esp_system.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_NeoPixel.h>
#include <PNGdec.h>

// PNGdec/zlib defines a macro named "local" which conflicts with
// NimBLE (ESP32 Arduino Core 3.x). Remove it before BLE headers.
#ifdef local
#undef local
#endif

#include <BLEDevice.h>
#include <BLEServer.h>
#include <time.h>

#if __has_include(<PubSubClient.h>)
#include <PubSubClient.h>
#define STATUSANZEIGE_HAS_MQTT 1
#else
#define STATUSANZEIGE_HAS_MQTT 0
#endif

// SimpleFTPServer für ESP32 ausdrücklich auf SD statt FFat festlegen.
#define DEFAULT_STORAGE_TYPE_ESP32 STORAGE_SD
#include <SimpleFTPServer.h>

#define STATUSANZEIGE_VERSION "2.0.2"
#define STATUSANZEIGE_BUILD "2026-10-25"

// -----------------------------------------------------------------------------
// Board-Pins
// -----------------------------------------------------------------------------
static constexpr int PIN_SPI_SCLK = 39;
static constexpr int PIN_SPI_MOSI = 38;
static constexpr int PIN_SPI_MISO = 40;

static constexpr int PIN_LCD_CS   = 45;
static constexpr int PIN_LCD_DC   = 42;
static constexpr int PIN_LCD_BL   = 1;
static constexpr int PIN_LCD_RST  = -1;

static constexpr int PIN_SD_CS    = 41;

static constexpr int DEFAULT_LED_PIN    = 17;
static constexpr int DEFAULT_BUTTON_PIN = 18;

// Intern reserviert, nicht für externe Hardware benutzen:
static constexpr int PIN_TOUCH_IMU_SDA = 48;
static constexpr int PIN_TOUCH_IMU_SCL = 47;

// -----------------------------------------------------------------------------
// Display
// -----------------------------------------------------------------------------
Arduino_DataBus *gfxBus = new Arduino_ESP32SPI(
  PIN_LCD_DC,
  PIN_LCD_CS,
  PIN_SPI_SCLK,
  PIN_SPI_MOSI,
  PIN_SPI_MISO
);

Arduino_GFX *gfx = new Arduino_ST7789(
  gfxBus,
  PIN_LCD_RST,
  0,      // Portrait; kann nach config geladen geändert werden
  true,   // IPS
  240,
  320
);

// -----------------------------------------------------------------------------
// Globale Objekte
// -----------------------------------------------------------------------------
Adafruit_NeoPixel pixels;
PNG png;
WebServer web(80);
FtpServer ftp;
DNSServer dnsServer;
WiFiClient mqttNet;
#if STATUSANZEIGE_HAS_MQTT
PubSubClient mqttClient(mqttNet);
#endif

BLEServer *bleServer = nullptr;
BLEAdvertising *bleAdvertising = nullptr;
BLEService *bleControlService = nullptr;
BLECharacteristic *bleCommandCharacteristic = nullptr;
BLECharacteristic *bleStateCharacteristic = nullptr;

bool sdReady = false;
bool wifiConnected = false;
bool apActive = false;
uint32_t lastWifiReconnectAttempt = 0;
const uint32_t WIFI_RECONNECT_INTERVAL_MS = 5000;
bool ftpStarted = false;
bool webStarted = false;
bool bleStarted = false;
bool mqttStarted = false;
bool touchStarted = false;
bool safeMode = false;
uint32_t lastLedEffectTick = 0;
RTC_DATA_ATTR uint8_t crashBootCounter = 0;
bool lowBatteryDimActive = false;
File configRestoreFile;
bool configRestoreAuthorized = false;

// -----------------------------------------------------------------------------
// Konfiguration
// -----------------------------------------------------------------------------
struct AppConfig {
  String deviceName = "StatusAnzeige";
  String hostname = "statusanzeige";

  bool wifiEnabled = true;
  String wifiSsid = "DEIN_WLAN_NAME";
  String wifiPassword = "DEIN_WLAN_PASSWORT";
  uint32_t wifiTimeoutMs = 15000;

  bool fallbackApEnabled = true;
  String fallbackApSsid = "StatusAnzeige-Setup";
  String fallbackApPassword = "StatusAnzeige123";

  bool bleEnabled = true;
  String bluetoothName = "StatusAnzeige";

  bool ftpEnabled = true;
  String ftpUser = "status";
  String ftpPassword = "StatusAnzeige123!";

  bool apiEnabled = true;
  String apiPasscode = "123456";

  bool clockEnabled = true;
  String tzPosix = "CET-1CEST,M3.5.0/2,M10.5.0/3";
  String ntp1 = "pool.ntp.org";
  String ntp2 = "time.google.com";
  String ntp3 = "time.cloudflare.com";
  uint16_t footerHeight = 54;
  uint32_t clockRefreshMs = 250;

  bool statusBarEnabled = true;
  uint16_t statusBarHeight = 22;
  uint32_t statusBarRefreshMs = 1000;

  bool batteryEnabled = true;
  int batteryAdcPin = 4;
  float batteryDivider = 3.0f;
  float batteryMinV = 3.30f;
  float batteryMaxV = 4.20f;
  uint8_t batteryWarnPercent = 20;
  uint8_t batteryDimPercent = 10;
  uint8_t batteryCriticalPercent = 5;

  bool touchEnabled = true;
  int touchSda = 48;
  int touchScl = 47;
  uint8_t touchAddress = 0x15;

  bool mqttEnabled = false;
  String mqttHost = "";
  uint16_t mqttPort = 1883;
  String mqttUser = "";
  String mqttPassword = "";
  String mqttBaseTopic = "statusanzeige";
  bool mqttDiscovery = true;

  bool webUpdateEnabled = true;
  bool captivePortalEnabled = true;

  uint8_t displayRotation = 2;
  uint8_t displayBrightness = 80;

  uint16_t ledCount = 13;
  int ledPin = DEFAULT_LED_PIN;
  uint8_t ledBrightness = 35;

  uint32_t menuColor = 0x0000AA;
  uint8_t menuMinBrightness = 8;
  uint8_t menuMaxBrightness = 35;
  uint32_t menuPeriodMs = 1600;

  uint32_t confirmationColor = 0xFFFFFF;
  uint32_t confirmationMs = 120;

  uint32_t startupRainbowMs = 2200;
  uint32_t startupRainbowStepMs = 45;

  int buttonPin = DEFAULT_BUTTON_PIN;
  uint32_t debounceMs = 35;
  uint32_t longPressMs = 700;
  uint32_t menuTimeoutMs = 12000;

  String defaultStatus = "frei";
  bool rememberLastStatus = true;

  bool sdUpdateEnabled = true;
  String firmwarePath = "/firmware/firmware.bin";
  String updateFlagPath = "/firmware/update.flag";
  String doneFirmwarePath = "/firmware/firmware.done.bin";
};

AppConfig cfg;

struct StatusItem {
  String id;
  String label;
  String image;
  uint32_t ledColor;
  String ledEffect = "static";
  uint16_t effectPeriodMs = 1000;
};

static constexpr size_t MAX_STATUSES = 24;
StatusItem statuses[MAX_STATUSES];
size_t statusCount = 0;
int currentStatusIndex = -1;

// -----------------------------------------------------------------------------
// Menü-/Tasterzustand
// -----------------------------------------------------------------------------
bool menuActive = false;
int menuIndex = 0;
uint32_t menuStartedAt = 0;

bool rawButtonState = HIGH;
bool stableButtonState = HIGH;
uint32_t lastButtonChangeAt = 0;
uint32_t buttonPressedAt = 0;
bool longPressHandled = false;

// -----------------------------------------------------------------------------
// PNG-Zustand
// -----------------------------------------------------------------------------
int pngX = 0;
int pngY = 0;
uint16_t pngLineBuffer[320];

// -----------------------------------------------------------------------------
// Zeitsteuerungen
// -----------------------------------------------------------------------------
uint32_t lastServiceCheck = 0;
uint32_t lastClockRefresh = 0;
int lastClockKey = -1;
bool timeSynced = false;

uint32_t lastStatusBarRefresh = 0;
int lastWifiBars = -1;
int lastBatteryPercent = -1;
float lastBatteryVoltage = 0.0f;

// -----------------------------------------------------------------------------
// Prototypen
// -----------------------------------------------------------------------------
bool initDisplayAndSd();
bool loadConfig();
void applyDisplaySettings();
void initLeds();
void startupRainbowWave();
void setLedColor(uint32_t rgb);
void updateMenuPulse();
void confirmationFlash();
void initClockSync();
void drawClockFooter(bool force = false);
void initBatteryAdc();
float readBatteryVoltage();
int batteryPercentFromVoltage(float voltage);
void drawStatusBar(bool force = false);
void initTouch();
void updateTouch();
void updateStatusLedEffect();
void initMqtt();
void maintainMqtt();
void publishMqttState(bool discovery = false);
void startCaptiveDns();
String systemPage();
String statusesConfigPage();
void handleStatusesJson();
void handleStatusConfigSave();
void handleConfigBackup();
bool copySdFile(const char *src, const char *dst);
String restorePage();
void handleConfigRestoreUpload();
void handleConfigRestoreDone();
void updateLowBatteryProtection();
String updatePage();
void handleFirmwareUpload();
void handleFirmwareUploadDone();
uint32_t parseColor(const String &s, uint32_t fallback);
uint32_t scaleColor(uint32_t rgb, uint8_t percent);

bool drawPngFromSd(const String &path);
void showTextScreen(const String &title, const String &line1 = "", const String &line2 = "");

int findStatusIndex(const String &id);
void activateStatus(int index, bool saveLast = true);
void enterMenu();
void nextMenuItem();
void confirmMenu();
void cancelMenu();
void updateButton();

void connectNetwork();
void maintainWifiConnection();
void startFallbackAp();
void startBle();
void notifyBleState();
bool saveWifiConfigToSd(const String &ssid, const String &password);
void startFtp();
void startWeb();
bool checkApiPasscode();
void serviceFlags();

void saveLastStatus();
String loadLastStatus();

bool checkAndApplySdFirmwareUpdate();

String currentIpString();
String wifiSecurityName(uint8_t enc);
String wifiSetupPage();
void handleWifiScan();
void handleWifiSave();
String configPage();
void handleConfigGet();
void handleConfigSave();
String buildHomePage();
void sendInfoJson();

// -----------------------------------------------------------------------------
// Hilfen
// -----------------------------------------------------------------------------
uint32_t parseColor(const String &s, uint32_t fallback) {
  String t = s;
  t.trim();
  if (t.startsWith("#")) t.remove(0, 1);
  if (t.length() != 6) return fallback;
  char *endptr = nullptr;
  unsigned long v = strtoul(t.c_str(), &endptr, 16);
  if (endptr == t.c_str()) return fallback;
  return static_cast<uint32_t>(v & 0xFFFFFF);
}

uint32_t scaleColor(uint32_t rgb, uint8_t percent) {
  percent = min<uint8_t>(percent, 100);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  r = (uint16_t(r) * percent) / 100;
  g = (uint16_t(g) * percent) / 100;
  b = (uint16_t(b) * percent) / 100;
  return pixels.Color(r, g, b);
}

// -----------------------------------------------------------------------------
// Display + SD
// -----------------------------------------------------------------------------
bool initDisplayAndSd() {
  pinMode(PIN_LCD_CS, OUTPUT);
  pinMode(PIN_SD_CS, OUTPUT);
  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_SD_CS, HIGH);

  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, LOW);

  Serial.println("[LCD] Initialisiere Display...");
  Serial.println("[LCD] gfx->begin()...");
  if (!gfx->begin()) {
    Serial.println("[LCD] FEHLER: gfx->begin fehlgeschlagen");
    return false;
  }

  Serial.println("[LCD] gfx->begin OK");
  gfx->fillScreen(RGB565_BLACK);

  // Backlight immediately on for diagnosis.
  digitalWrite(PIN_LCD_BL, HIGH);
  Serial.println("[LCD] Backlight EIN");

  digitalWrite(PIN_LCD_CS, HIGH);

  // Reconfigure Arduino's global SPI object for the SD card.
  // LCD and SD use the same physical pins but separate CS lines.
  SPI.begin(PIN_SPI_SCLK, PIN_SPI_MISO, PIN_SPI_MOSI, -1);

  // SD zuerst mit moderaten 16 MHz, falls nötig Retry mit 4 MHz.
  Serial.println("[SD] Initialisiere SD-Karte...");
  sdReady = SD.begin(PIN_SD_CS, SPI, 16000000);
  if (!sdReady) {
    Serial.println("[SD] 16 MHz fehlgeschlagen, Retry mit 4 MHz...");
    SD.end();
    delay(100);
    sdReady = SD.begin(PIN_SD_CS, SPI, 4000000);
  }

  if (!sdReady) {
    Serial.println("[SD] FEHLER: Keine SD-Karte");
    showTextScreen("SD FEHLER", "Karte pruefen", "FAT32 verwenden");
    digitalWrite(PIN_LCD_BL, HIGH);
    return false;
  }

  uint64_t sizeMb = SD.cardSize() / (1024ULL * 1024ULL);
  Serial.printf("[SD] OK, %llu MB\n", sizeMb);

  digitalWrite(PIN_LCD_CS, HIGH);
  digitalWrite(PIN_SD_CS, HIGH);
  return true;
}

void applyDisplaySettings() {
  // Gehäusemontage: Display ist physisch um 180° gedreht.
  // rotation 2 ist deshalb der korrekte Standard.
  cfg.displayRotation &= 0x03;
  gfx->setRotation(cfg.displayRotation);

  // Arduino-ESP32 Core >= 3.0
  ledcAttach(PIN_LCD_BL, 5000, 8);
  uint8_t duty = map(constrain(cfg.displayBrightness, 0, 100), 0, 100, 0, 255);
  ledcWrite(PIN_LCD_BL, duty);
}

// -----------------------------------------------------------------------------
// Config
// -----------------------------------------------------------------------------
bool loadConfig() {
  if (!sdReady) return false;

  File f = SD.open("/config/config.json", FILE_READ);
  if (!f) {
    Serial.println("[CFG] config.json nicht gefunden - Defaults werden benutzt");
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    Serial.printf("[CFG] JSON Fehler: %s\n", err.c_str());
    return false;
  }

  cfg.deviceName = doc["device"]["name"] | cfg.deviceName;
  cfg.hostname = doc["device"]["hostname"] | cfg.hostname;

  cfg.wifiEnabled = doc["wifi"]["enabled"] | cfg.wifiEnabled;
  cfg.wifiSsid = doc["wifi"]["ssid"] | cfg.wifiSsid;
  cfg.wifiPassword = doc["wifi"]["password"] | cfg.wifiPassword;
  cfg.wifiTimeoutMs = doc["wifi"]["connect_timeout_ms"] | cfg.wifiTimeoutMs;
  cfg.fallbackApEnabled = doc["wifi"]["fallback_ap"]["enabled"] | cfg.fallbackApEnabled;
  cfg.fallbackApSsid = doc["wifi"]["fallback_ap"]["ssid"] | cfg.fallbackApSsid;
  cfg.fallbackApPassword = doc["wifi"]["fallback_ap"]["password"] | cfg.fallbackApPassword;

  cfg.bleEnabled = doc["bluetooth"]["enabled"] | cfg.bleEnabled;
  cfg.bluetoothName = doc["bluetooth"]["device_name"] | cfg.bluetoothName;

  cfg.ftpEnabled = doc["ftp"]["enabled"] | cfg.ftpEnabled;
  cfg.ftpUser = doc["ftp"]["user"] | cfg.ftpUser;
  cfg.ftpPassword = doc["ftp"]["password"] | cfg.ftpPassword;

  cfg.apiEnabled = doc["api"]["enabled"] | cfg.apiEnabled;
  cfg.apiPasscode = doc["api"]["passcode"] | cfg.apiPasscode;

  cfg.clockEnabled = doc["clock"]["enabled"] | cfg.clockEnabled;
  cfg.tzPosix = doc["clock"]["timezone_posix"] | cfg.tzPosix;
  cfg.ntp1 = doc["clock"]["ntp1"] | cfg.ntp1;
  cfg.ntp2 = doc["clock"]["ntp2"] | cfg.ntp2;
  cfg.ntp3 = doc["clock"]["ntp3"] | cfg.ntp3;
  cfg.footerHeight = doc["clock"]["footer_height"] | cfg.footerHeight;
  cfg.clockRefreshMs = doc["clock"]["refresh_ms"] | cfg.clockRefreshMs;

  cfg.statusBarEnabled = doc["status_bar"]["enabled"] | cfg.statusBarEnabled;
  cfg.statusBarHeight = doc["status_bar"]["height"] | cfg.statusBarHeight;
  cfg.statusBarRefreshMs = doc["status_bar"]["refresh_ms"] | cfg.statusBarRefreshMs;

  cfg.batteryEnabled = doc["battery"]["enabled"] | cfg.batteryEnabled;
  // BAT_ADC is fixed by the Waveshare PCB wiring.
  // Ignore stale/wrong values from older config files.
  cfg.batteryAdcPin = 4;
  cfg.batteryDivider = doc["battery"]["divider"] | cfg.batteryDivider;
  cfg.batteryMinV = doc["battery"]["min_voltage"] | cfg.batteryMinV;
  cfg.batteryMaxV = doc["battery"]["max_voltage"] | cfg.batteryMaxV;
  cfg.batteryWarnPercent = doc["battery"]["warn_percent"] | cfg.batteryWarnPercent;
  cfg.batteryDimPercent = doc["battery"]["dim_percent"] | cfg.batteryDimPercent;
  cfg.batteryCriticalPercent = doc["battery"]["critical_percent"] | cfg.batteryCriticalPercent;

cfg.touchEnabled = doc["touch"]["enabled"] | cfg.touchEnabled;
cfg.touchSda = doc["touch"]["sda"] | cfg.touchSda;
cfg.touchScl = doc["touch"]["scl"] | cfg.touchScl;

cfg.mqttEnabled = doc["mqtt"]["enabled"] | cfg.mqttEnabled;
cfg.mqttHost = doc["mqtt"]["host"] | cfg.mqttHost;
cfg.mqttPort = doc["mqtt"]["port"] | cfg.mqttPort;
cfg.mqttUser = doc["mqtt"]["user"] | cfg.mqttUser;
cfg.mqttPassword = doc["mqtt"]["password"] | cfg.mqttPassword;
cfg.mqttBaseTopic = doc["mqtt"]["base_topic"] | cfg.mqttBaseTopic;
cfg.mqttDiscovery = doc["mqtt"]["home_assistant_discovery"] | cfg.mqttDiscovery;

cfg.webUpdateEnabled = doc["web_update"]["enabled"] | cfg.webUpdateEnabled;
cfg.captivePortalEnabled = doc["wifi"]["fallback_ap"]["captive_portal"] | cfg.captivePortalEnabled;

  cfg.displayRotation = doc["display"]["rotation"] | cfg.displayRotation;
  cfg.displayBrightness = doc["display"]["brightness_percent"] | cfg.displayBrightness;

  cfg.ledCount = doc["leds"]["count"] | cfg.ledCount;
  cfg.ledPin = doc["leds"]["data_pin"] | cfg.ledPin;
  cfg.ledBrightness = doc["leds"]["brightness_percent"] | cfg.ledBrightness;

  cfg.startupRainbowMs = doc["leds"]["startup"]["duration_ms"] | cfg.startupRainbowMs;
  cfg.startupRainbowStepMs = doc["leds"]["startup"]["step_ms"] | cfg.startupRainbowStepMs;

  cfg.menuColor = parseColor(
    String((const char*)(doc["leds"]["menu"]["color"] | "#0000AA")),
    cfg.menuColor
  );
  cfg.menuMinBrightness = doc["leds"]["menu"]["min_brightness_percent"] | cfg.menuMinBrightness;
  cfg.menuMaxBrightness = doc["leds"]["menu"]["max_brightness_percent"] | cfg.menuMaxBrightness;
  cfg.menuPeriodMs = doc["leds"]["menu"]["period_ms"] | cfg.menuPeriodMs;

  cfg.confirmationColor = parseColor(
    String((const char*)(doc["leds"]["confirmation"]["color"] | "#FFFFFF")),
    cfg.confirmationColor
  );
  cfg.confirmationMs = doc["leds"]["confirmation"]["duration_ms"] | cfg.confirmationMs;

  cfg.buttonPin = doc["button"]["pin"] | cfg.buttonPin;
  cfg.debounceMs = doc["button"]["debounce_ms"] | cfg.debounceMs;
  cfg.longPressMs = doc["button"]["long_press_ms"] | cfg.longPressMs;
  cfg.menuTimeoutMs = doc["button"]["menu_timeout_ms"] | cfg.menuTimeoutMs;

  cfg.defaultStatus = doc["startup"]["default_status"] | cfg.defaultStatus;
  cfg.rememberLastStatus = doc["startup"]["remember_last_status"] | cfg.rememberLastStatus;

  cfg.sdUpdateEnabled = doc["sd_update"]["enabled"] | cfg.sdUpdateEnabled;
  cfg.firmwarePath = doc["sd_update"]["firmware_path"] | cfg.firmwarePath;
  cfg.updateFlagPath = doc["sd_update"]["update_flag_path"] | cfg.updateFlagPath;
  cfg.doneFirmwarePath = doc["sd_update"]["done_firmware_path"] | cfg.doneFirmwarePath;

  statusCount = 0;
  JsonArray arr = doc["statuses"].as<JsonArray>();
  for (JsonObject s : arr) {
    if (statusCount >= MAX_STATUSES) break;

    String id = s["id"] | "";
    String image = s["image"] | "";
    if (id.isEmpty() || image.isEmpty()) continue;

    statuses[statusCount].id = id;
    statuses[statusCount].label = String((const char*)(s["label"] | id.c_str()));
    statuses[statusCount].image = image;
    statuses[statusCount].ledColor = parseColor(
      String((const char*)(s["led_color"] | "#FFFFFF")),
      0xFFFFFF
    );
    statuses[statusCount].ledEffect = String((const char*)(s["led_effect"] | "static"));
    statuses[statusCount].effectPeriodMs = s["effect_period_ms"] | 1000;
    statusCount++;
  }

  Serial.printf("[CFG] %u Status-Eintraege geladen\n", (unsigned)statusCount);
  return true;
}

// -----------------------------------------------------------------------------
// LED
// -----------------------------------------------------------------------------
void initLeds() {
  cfg.ledCount = constrain(cfg.ledCount, 1, 64);
  pixels.updateLength(cfg.ledCount);
  pixels.setPin(cfg.ledPin);
  pixels.begin();
  pixels.setBrightness(map(constrain(cfg.ledBrightness, 0, 100), 0, 100, 0, 255));
  pixels.clear();
  pixels.show();
}

void startupRainbowWave() {
  uint32_t start = millis();
  uint32_t last = 0;
  uint16_t baseHue = 0;

  while (millis() - start < cfg.startupRainbowMs) {
    if (millis() - last >= cfg.startupRainbowStepMs) {
      last = millis();
      for (uint16_t i = 0; i < cfg.ledCount; i++) {
        uint16_t hue = baseHue + (65535UL * i / cfg.ledCount);
        pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue, 255, 255)));
      }
      pixels.show();
      baseHue += 1800;
    }
    delay(1);
  }
  pixels.clear();
  pixels.show();
}

void setLedColor(uint32_t rgb) {
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;
  for (uint16_t i = 0; i < cfg.ledCount; i++) {
    pixels.setPixelColor(i, pixels.Color(r, g, b));
  }
  pixels.show();
}

void updateMenuPulse() {
  if (!menuActive) return;

  float phase = (millis() % cfg.menuPeriodMs) / float(cfg.menuPeriodMs);
  float wave = (sinf(phase * 2.0f * PI) + 1.0f) * 0.5f;
  uint8_t pct = cfg.menuMinBrightness +
                uint8_t(wave * (cfg.menuMaxBrightness - cfg.menuMinBrightness));

  uint32_t c = scaleColor(cfg.menuColor, pct);
  for (uint16_t i = 0; i < cfg.ledCount; i++) pixels.setPixelColor(i, c);
  pixels.show();
}

void confirmationFlash() {
  setLedColor(cfg.confirmationColor);
  delay(cfg.confirmationMs);
}

void initClockSync() {
  if (!cfg.clockEnabled || !wifiConnected) return;

  configTzTime(
    cfg.tzPosix.c_str(),
    cfg.ntp1.c_str(),
    cfg.ntp2.c_str(),
    cfg.ntp3.c_str()
  );

  struct tm tmNow;
  if (getLocalTime(&tmNow, 5000)) {
    timeSynced = true;
    Serial.printf("[CLOCK] Zeit synchronisiert: %02d:%02d\n", tmNow.tm_hour, tmNow.tm_min);
  } else {
    Serial.println("[CLOCK] NTP noch nicht verfuegbar");
  }
}

void drawClockFooter(bool force) {
  if (!cfg.clockEnabled) return;
  if (millis() - lastClockRefresh < cfg.clockRefreshMs && !force) return;
  lastClockRefresh = millis();

  struct tm tmNow;
  bool ok = getLocalTime(&tmNow, 50);

  String timeStr = "--:--";
  String dateStr = "Keine Zeit";

  if (ok) {
    timeSynced = true;
    const char* wdays[] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
    const char* months[] = {"Jan", "Feb", "Mrz", "Apr", "Mai", "Jun", "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

    // Uhrzeit mit dauerhaft sichtbarem Doppelpunkt.
    char bufTime[8];
    snprintf(
      bufTime,
      sizeof(bufTime),
      "%02d:%02d",
      tmNow.tm_hour,
      tmNow.tm_min
    );
    timeStr = String(bufTime);

    char bufDate[24];
    snprintf(
      bufDate,
      sizeof(bufDate),
      "%s %02d.%s",
      wdays[tmNow.tm_wday],
      tmNow.tm_mday,
      months[tmNow.tm_mon]
    );
    dateStr = String(bufDate);

    // Nur bei Minutenwechsel neu zeichnen.
    int key =
      tmNow.tm_yday * 1440 +
      tmNow.tm_hour * 60 +
      tmNow.tm_min;

    if (!force && key == lastClockKey) return;
    lastClockKey = key;
  }

  uint16_t footerH = min<uint16_t>(cfg.footerHeight, gfx->height());
  uint16_t y = gfx->height() - footerH;

  gfx->fillRect(0, y, gfx->width(), footerH, RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);

  // Time
  gfx->setTextSize(3);
  int timeW = timeStr.length() * 6 * 3;
  int timeX = max(0, (int(gfx->width()) - timeW) / 2);
  gfx->setCursor(timeX, y + 6);
  gfx->print(timeStr);

  // Date
  gfx->setTextSize(2);
  int dateW = dateStr.length() * 6 * 2;
  int dateX = max(0, (int(gfx->width()) - dateW) / 2);
  gfx->setCursor(dateX, y + 34);
  gfx->print(dateStr);
}

void initBatteryAdc() {
  if (!cfg.batteryEnabled) return;

  // Waveshare ESP32-S3-Touch-LCD-2:
  // BAT_ADC is GPIO4 = ADC1_CH3.
  cfg.batteryAdcPin = 4;

  analogReadResolution(12);
  analogSetPinAttenuation(cfg.batteryAdcPin, ADC_11db);

  // One dummy conversion initializes the Arduino-ESP32 ADC path.
  (void)analogRead(cfg.batteryAdcPin);

  Serial.printf("[BAT] BAT_ADC GPIO%d initialisiert\n", cfg.batteryAdcPin);
}

float readBatteryVoltage() {
  if (!cfg.batteryEnabled) return 0.0f;

  // Fixed hardware mapping on Waveshare ESP32-S3-Touch-LCD-2.
  const int adcPin = 4;

  // Average several ADC readings to reduce jitter.
  uint32_t mvSum = 0;
  const int samples = 12;

  for (int i = 0; i < samples; i++) {
    mvSum += analogReadMilliVolts(adcPin);
    delayMicroseconds(250);
  }

  float adcMv = mvSum / float(samples);
  return (adcMv / 1000.0f) * cfg.batteryDivider;
}

int batteryPercentFromVoltage(float v) {
  // Practical 1S LiPo approximation.
  // Deliberately conservative at the low end.
  if (v <= 3.30f) return 0;
  if (v >= 4.20f) return 100;

  struct Point { float v; int p; };
  static const Point curve[] = {
    {3.30f, 0},
    {3.50f, 5},
    {3.60f, 10},
    {3.70f, 20},
    {3.75f, 30},
    {3.80f, 40},
    {3.85f, 50},
    {3.90f, 60},
    {3.95f, 70},
    {4.00f, 80},
    {4.10f, 90},
    {4.20f, 100}
  };

  for (size_t i = 1; i < sizeof(curve) / sizeof(curve[0]); i++) {
    if (v <= curve[i].v) {
      float span = curve[i].v - curve[i - 1].v;
      float f = (v - curve[i - 1].v) / span;
      return curve[i - 1].p + int(roundf(f * (curve[i].p - curve[i - 1].p)));
    }
  }
  return 100;
}

void drawStatusBar(bool force) {
  if (!cfg.statusBarEnabled) return;
  if (!force && millis() - lastStatusBarRefresh < cfg.statusBarRefreshMs) return;
  lastStatusBarRefresh = millis();

  // WiFi strength -> 0..4 bars.
  int wifiBars = 0;
  int rssi = -127;
  if (WiFi.status() == WL_CONNECTED) {
    rssi = WiFi.RSSI();
    if (rssi >= -55) wifiBars = 4;
    else if (rssi >= -67) wifiBars = 3;
    else if (rssi >= -75) wifiBars = 2;
    else if (rssi >= -85) wifiBars = 1;
  }

  float batteryV = readBatteryVoltage();
  int batteryPct = cfg.batteryEnabled ? batteryPercentFromVoltage(batteryV) : -1;

  if (!force && wifiBars == lastWifiBars && batteryPct == lastBatteryPercent) {
    return;
  }

  lastWifiBars = wifiBars;
  lastBatteryPercent = batteryPct;
  lastBatteryVoltage = batteryV;

  const int h = min((int)cfg.statusBarHeight, (int)gfx->height());
  gfx->fillRect(0, 0, gfx->width(), h, RGB565_BLACK);

  // ---- WiFi icon: left side ----
  const int wx = 8;
  const int baseY = h - 4;
  const int barW = 3;
  const int gap = 2;

  for (int i = 0; i < 4; i++) {
    int bh = 3 + i * 3;
    uint16_t c = (i < wifiBars) ? RGB565_WHITE : 0x4208; // dark gray
    gfx->fillRect(wx + i * (barW + gap), baseY - bh, barW, bh, c);
  }

  // Small RSSI text.
  gfx->setTextSize(1);
  gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
  gfx->setCursor(31, 7);
  if (WiFi.status() == WL_CONNECTED) {
    gfx->print(String(rssi));
  } else {
    gfx->print("--");
  }

  // ---- Battery icon: right side ----
  const int bx = gfx->width() - 57;
  const int by = 4;
  const int bw = 27;
  const int bh = 14;

  gfx->drawRect(bx, by, bw, bh, RGB565_WHITE);
  gfx->fillRect(bx + bw, by + 4, 3, 6, RGB565_WHITE);

  if (batteryPct >= 0) {
    int fillW = map(constrain(batteryPct, 0, 100), 0, 100, 0, bw - 4);

    uint16_t fillColor = RGB565_WHITE;
    if (batteryPct <= 15) fillColor = 0xF800;      // red
    else if (batteryPct <= 30) fillColor = 0xFFE0; // yellow

    if (fillW > 0) {
      gfx->fillRect(bx + 2, by + 2, fillW, bh - 4, fillColor);
    }

    gfx->setTextSize(1);
    gfx->setTextColor(RGB565_WHITE, RGB565_BLACK);
    gfx->setCursor(gfx->width() - 24, 7);
    gfx->print(String(batteryPct));
    gfx->print("%");
  } else {
    gfx->setCursor(gfx->width() - 21, 7);
    gfx->print("--");
  }
}

// -----------------------------------------------------------------------------
// PNG von SD
// -----------------------------------------------------------------------------
void *pngOpen(const char *filename, int32_t *size) {
  File *f = new File(SD.open(filename, FILE_READ));
  if (!(*f)) {
    delete f;
    return nullptr;
  }
  *size = (int32_t)f->size();
  return f;
}

void pngClose(void *handle) {
  File *f = reinterpret_cast<File *>(handle);
  if (f) {
    f->close();
    delete f;
  }
}

int32_t pngRead(PNGFILE *pFile, uint8_t *buffer, int32_t length) {
  File *f = reinterpret_cast<File *>(pFile->fHandle);
  if (!f) return 0;
  return f->read(buffer, length);
}

int32_t pngSeek(PNGFILE *pFile, int32_t position) {
  File *f = reinterpret_cast<File *>(pFile->fHandle);
  if (!f) return -1;
  if (!f->seek(position)) return -1;
  return position;
}

int pngDraw(PNGDRAW *pDraw) {
  if (pDraw->iWidth > 320) return 0;

  png.getLineAsRGB565(
    pDraw,
    pngLineBuffer,
    PNG_RGB565_BIG_ENDIAN,
    0x00000000
  );

  // PNGdec liefert hier RGB565 in Big-Endian-Reihenfolge.
  // Arduino_GFX muss deshalb die Big-Endian-Zeichenfunktion verwenden.
  gfx->draw16bitBeRGBBitmap(
    pngX,
    pngY + pDraw->y,
    pngLineBuffer,
    pDraw->iWidth,
    1
  );
  return 1;
}

bool drawPngFromSd(const String &path) {
  if (!sdReady || !SD.exists(path)) {
    Serial.printf("[PNG] Datei fehlt: %s\n", path.c_str());
    showTextScreen("BILD FEHLT", path);
    return false;
  }

  digitalWrite(PIN_SD_CS, HIGH);
  digitalWrite(PIN_LCD_CS, HIGH);

  int rc = png.open(
    path.c_str(),
    pngOpen,
    pngClose,
    pngRead,
    pngSeek,
    pngDraw
  );

  if (rc != PNG_SUCCESS) {
    Serial.printf("[PNG] open Fehler %d: %s\n", rc, path.c_str());
    showTextScreen("PNG FEHLER", path);
    return false;
  }

  int w = png.getWidth();
  int h = png.getHeight();
  pngX = max(0, (gfx->width() - w) / 2);
  pngY = max(0, (gfx->height() - h) / 2);

  gfx->fillScreen(RGB565_BLACK);
  rc = png.decode(nullptr, 0);
  png.close();

  if (rc != PNG_SUCCESS) {
    Serial.printf("[PNG] decode Fehler %d\n", rc);
    return false;
  }
  return true;
}

void showTextScreen(const String &title, const String &line1, const String &line2) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 40);
  gfx->println(title);
  gfx->setTextSize(1);
  if (!line1.isEmpty()) {
    gfx->setCursor(10, 90);
    gfx->println(line1);
  }
  if (!line2.isEmpty()) {
    gfx->setCursor(10, 115);
    gfx->println(line2);
  }
}

// -----------------------------------------------------------------------------
// Statuslogik
// -----------------------------------------------------------------------------
int findStatusIndex(const String &id) {
  for (size_t i = 0; i < statusCount; i++) {
    if (statuses[i].id.equalsIgnoreCase(id)) return (int)i;
  }
  return -1;
}

void saveLastStatus() {
  if (!sdReady || !cfg.rememberLastStatus || currentStatusIndex < 0) return;
  File f = SD.open("/data/last_status.txt", FILE_WRITE);
  if (!f) return;
  f.print(statuses[currentStatusIndex].id);
  f.close();
}

String loadLastStatus() {
  if (!sdReady || !cfg.rememberLastStatus) return "";
  File f = SD.open("/data/last_status.txt", FILE_READ);
  if (!f) return "";
  String s = f.readString();
  f.close();
  s.trim();
  return s;
}

void activateStatus(int index, bool saveLast) {
  if (index < 0 || index >= (int)statusCount) return;

  currentStatusIndex = index;
  menuActive = false;

  Serial.printf("[STATUS] %s\n", statuses[index].id.c_str());
  drawPngFromSd(statuses[index].image);
  drawStatusBar(true);
  drawClockFooter(true);
  setLedColor(statuses[index].ledColor);

  if (saveLast) saveLastStatus();
  if (bleStarted) notifyBleState();
  publishMqttState(false);
}

void enterMenu() {
  if (statusCount == 0) return;
  menuActive = true;
  menuStartedAt = millis();
  menuIndex = currentStatusIndex >= 0 ? currentStatusIndex : 0;

  drawPngFromSd(statuses[menuIndex].image);
  drawStatusBar(true);
  drawClockFooter(true);
  updateMenuPulse();
  Serial.printf("[MENU] %s\n", statuses[menuIndex].id.c_str());
}

void nextMenuItem() {
  if (!menuActive || statusCount == 0) return;
  menuIndex = (menuIndex + 1) % statusCount;
  menuStartedAt = millis();

  drawPngFromSd(statuses[menuIndex].image);
  drawStatusBar(true);
  drawClockFooter(true);
  updateMenuPulse();
  Serial.printf("[MENU] %s\n", statuses[menuIndex].id.c_str());
}

void confirmMenu() {
  if (!menuActive) return;
  confirmationFlash();
  activateStatus(menuIndex, true);
}

void cancelMenu() {
  if (!menuActive) return;
  menuActive = false;
  if (currentStatusIndex >= 0) activateStatus(currentStatusIndex, false);
}

void updateButton() {
  bool raw = digitalRead(cfg.buttonPin);

  if (raw != rawButtonState) {
    rawButtonState = raw;
    lastButtonChangeAt = millis();
  }

  if (millis() - lastButtonChangeAt < cfg.debounceMs) return;

  if (stableButtonState != rawButtonState) {
    stableButtonState = rawButtonState;

    if (stableButtonState == LOW) {
      buttonPressedAt = millis();
      longPressHandled = false;
    } else {
      if (!longPressHandled) {
        // kurzer Druck
        if (!menuActive) enterMenu();
        else nextMenuItem();
      }
    }
  }

  if (stableButtonState == LOW && !longPressHandled) {
    if (millis() - buttonPressedAt >= cfg.longPressMs) {
      longPressHandled = true;
      if (menuActive) confirmMenu();
      else enterMenu();
    }
  }

  if (menuActive && millis() - menuStartedAt >= cfg.menuTimeoutMs) {
    cancelMenu();
  }
}


// -----------------------------------------------------------------------------
// Touch, Status-LED-Effekte und MQTT
// -----------------------------------------------------------------------------
void initTouch() {
  if (!cfg.touchEnabled) return;
  Wire.begin(cfg.touchSda, cfg.touchScl);
  Wire.beginTransmission(cfg.touchAddress);
  touchStarted = (Wire.endTransmission() == 0);
  Serial.printf("[TOUCH] CST816D: %s\n", touchStarted ? "OK" : "nicht gefunden");
}

bool readTouchPoint(int &x, int &y) {
  if (!touchStarted) return false;
  Wire.beginTransmission(cfg.touchAddress);
  Wire.write(0x02);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)cfg.touchAddress, 5) != 5) return false;
  uint8_t fingers = Wire.read();
  uint8_t xh = Wire.read();
  uint8_t xl = Wire.read();
  uint8_t yh = Wire.read();
  uint8_t yl = Wire.read();
  if ((fingers & 0x0F) == 0) return false;
  x = ((xh & 0x0F) << 8) | xl;
  y = ((yh & 0x0F) << 8) | yl;
  if (cfg.displayRotation == 2) { x = 239 - x; y = 319 - y; }
  return true;
}

void updateTouch() {
  static bool wasDown = false;
  static int startX = 0, startY = 0, lastX = 0, lastY = 0;
  static uint32_t downAt = 0;
  int x = 0, y = 0;
  bool down = readTouchPoint(x, y);
  if (down) { lastX = x; lastY = y; }
  if (down && !wasDown) { startX = x; startY = y; downAt = millis(); }
  if (!down && wasDown) {
    int dx = lastX - startX;
    if (abs(dx) >= 45 && statusCount > 0) {
      if (!menuActive) enterMenu();
      if (dx < 0) nextMenuItem();
      else {
        menuIndex = (menuIndex - 1 + (int)statusCount) % statusCount;
        drawPngFromSd(statuses[menuIndex].image);
        drawStatusBar(true);
        drawClockFooter(true);
      }
    } else if (millis() - downAt > 650) {
      if (menuActive) confirmMenu(); else enterMenu();
    } else {
      if (!menuActive) enterMenu(); else nextMenuItem();
    }
  }
  wasDown = down;
}

void updateStatusLedEffect() {
  if (menuActive || currentStatusIndex < 0) return;
  StatusItem &st = statuses[currentStatusIndex];
  if (st.ledEffect == "static" || st.ledEffect.isEmpty()) return;
  uint32_t period = max((uint32_t)st.effectPeriodMs, (uint32_t)200);
  if (millis() - lastLedEffectTick < 50) return;
  lastLedEffectTick = millis();
  if (st.ledEffect == "blink") {
    bool on = ((millis() / max((uint32_t)(period / 2), (uint32_t)100)) % 2) == 0;
    setLedColor(on ? st.ledColor : 0x000000);
  } else if (st.ledEffect == "pulse") {
    float phase = (millis() % period) / float(period);
    uint8_t pct = 20 + uint8_t(((sinf(phase * 2.0f * PI) + 1.0f) * 0.5f) * 80.0f);
    setLedColor(scaleColor(st.ledColor, pct));
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  msg.trim();
  if (String(topic) == cfg.mqttBaseTopic + "/set") {
    int idx = findStatusIndex(msg);
    if (idx >= 0) activateStatus(idx, true);
  }
}

void publishMqttState(bool discovery) {
#if STATUSANZEIGE_HAS_MQTT
  if (!mqttStarted || !mqttClient.connected()) return;
  String state = currentStatusIndex >= 0 ? statuses[currentStatusIndex].id : "unknown";
  mqttClient.publish((cfg.mqttBaseTopic + "/state").c_str(), state.c_str(), true);
  mqttClient.publish((cfg.mqttBaseTopic + "/availability").c_str(), "online", true);
  char tmp[24];
  snprintf(tmp, sizeof(tmp), "%d", lastBatteryPercent);
  mqttClient.publish((cfg.mqttBaseTopic + "/battery").c_str(), tmp, true);
  snprintf(tmp, sizeof(tmp), "%d", WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : -127);
  mqttClient.publish((cfg.mqttBaseTopic + "/rssi").c_str(), tmp, true);
  if (discovery && cfg.mqttDiscovery) {
    String uid = cfg.hostname;
    String sel = "{\"name\":\"Status\",\"unique_id\":\"" + uid + "_status\",\"state_topic\":\"" + cfg.mqttBaseTopic + "/state\",\"command_topic\":\"" + cfg.mqttBaseTopic + "/set\",\"availability_topic\":\"" + cfg.mqttBaseTopic + "/availability\"}";
    mqttClient.publish(("homeassistant/select/" + uid + "/status/config").c_str(), sel.c_str(), true);
    String bat = "{\"name\":\"Akku\",\"unique_id\":\"" + uid + "_battery\",\"state_topic\":\"" + cfg.mqttBaseTopic + "/battery\",\"unit_of_measurement\":\"%\",\"device_class\":\"battery\"}";
    mqttClient.publish(("homeassistant/sensor/" + uid + "/battery/config").c_str(), bat.c_str(), true);
  }
#endif
}

void initMqtt() {
#if STATUSANZEIGE_HAS_MQTT
  if (!cfg.mqttEnabled || cfg.mqttHost.isEmpty()) return;
  mqttClient.setServer(cfg.mqttHost.c_str(), cfg.mqttPort);
  mqttClient.setCallback(mqttCallback);
  mqttStarted = true;
#else
  if (cfg.mqttEnabled) Serial.println("[MQTT] PubSubClient fehlt; MQTT bleibt aus");
#endif
}

void maintainMqtt() {
#if STATUSANZEIGE_HAS_MQTT
  if (!mqttStarted || WiFi.status() != WL_CONNECTED) return;
  if (!mqttClient.connected()) {
    String cid = cfg.hostname + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool ok;
    if (cfg.mqttUser.isEmpty()) {
      ok = mqttClient.connect(cid.c_str(), (cfg.mqttBaseTopic + "/availability").c_str(), 0, true, "offline");
    } else {
      ok = mqttClient.connect(cid.c_str(), cfg.mqttUser.c_str(), cfg.mqttPassword.c_str(), (cfg.mqttBaseTopic + "/availability").c_str(), 0, true, "offline");
    }
    if (ok) {
      mqttClient.subscribe((cfg.mqttBaseTopic + "/set").c_str());
      publishMqttState(true);
    }
  }
  mqttClient.loop();
#endif
}

void startCaptiveDns() {
  if (apActive && cfg.captivePortalEnabled) dnsServer.start(53, "*", WiFi.softAPIP());
}

// -----------------------------------------------------------------------------
// Netzwerk
// -----------------------------------------------------------------------------
void startFallbackAp() {
  if (!cfg.fallbackApEnabled) return;

  WiFi.mode(WIFI_AP_STA);
  bool ok = WiFi.softAP(
    cfg.fallbackApSsid.c_str(),
    cfg.fallbackApPassword.c_str()
  );

  if (ok) {
    apActive = true;
    Serial.printf("[WIFI] Fallback-AP: %s\n", cfg.fallbackApSsid.c_str());
    Serial.printf("[WIFI] AP-IP: %s\n", WiFi.softAPIP().toString().c_str());
    startCaptiveDns();
  }
}

void connectNetwork() {
  if (!cfg.wifiEnabled) {
    Serial.println("[WIFI] deaktiviert");
    if (cfg.fallbackApEnabled) startFallbackAp();
    return;
  }

  bool placeholder =
    cfg.wifiSsid.startsWith("DEIN_") ||
    cfg.wifiPassword.startsWith("DEIN_") ||
    cfg.wifiSsid.isEmpty();

  if (placeholder) {
    Serial.println("[WIFI] Zugangsdaten noch Platzhalter");
    startFallbackAp();
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);

  // Maximale WLAN-Performance:
  // - kein WiFi Power Save
  // - maximale Sendeleistung des ESP32-S3
  WiFi.setSleep(false);

  bool txPowerOk = WiFi.setTxPower(WIFI_POWER_19_5dBm);
  Serial.printf(
    "[WIFI] Power Save: AUS | TX Power 19.5 dBm: %s\n",
    txPowerOk ? "OK" : "FEHLER"
  );

  WiFi.setHostname(cfg.hostname.c_str());
  WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());

  Serial.printf("[WIFI] Verbinde mit %s", cfg.wifiSsid.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - start < cfg.wifiTimeoutMs) {
    Serial.print(".");
    delay(250);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("[WIFI] OK: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("[WIFI] RSSI: %d dBm\n", WiFi.RSSI());
    Serial.printf("[WIFI] Kanal: %d\n", WiFi.channel());

    if (MDNS.begin(cfg.hostname.c_str())) {
      MDNS.addService("http", "tcp", 80);
      MDNS.addService("ftp", "tcp", 21);
      Serial.printf("[mDNS] http://%s.local/\n", cfg.hostname.c_str());
    }
  } else {
    Serial.println("[WIFI] Verbindung fehlgeschlagen");
    startFallbackAp();
  }
}


void maintainWifiConnection() {
  if (!cfg.wifiEnabled) return;

  wl_status_t state = WiFi.status();

  if (state == WL_CONNECTED) {
    if (!wifiConnected) {
      wifiConnected = true;
      Serial.printf(
        "[WIFI] Wieder verbunden: %s | RSSI %d dBm\n",
        WiFi.localIP().toString().c_str(),
        WiFi.RSSI()
      );
    }
    return;
  }

  if (wifiConnected) {
    wifiConnected = false;
    Serial.println("[WIFI] Verbindung verloren");
  }

  if (millis() - lastWifiReconnectAttempt < WIFI_RECONNECT_INTERVAL_MS) {
    return;
  }

  lastWifiReconnectAttempt = millis();

  Serial.println("[WIFI] Reconnect...");
  WiFi.reconnect();
}


static const char *BLE_SERVICE_UUID = "7b4e1000-8b6f-4d69-9d52-5f6a7a8c1000";
static const char *BLE_COMMAND_UUID = "7b4e1001-8b6f-4d69-9d52-5f6a7a8c1000";
static const char *BLE_STATE_UUID   = "7b4e1002-8b6f-4d69-9d52-5f6a7a8c1000";

bool saveWifiConfigToSd(const String &ssid, const String &password) {
  if (!sdReady) return false;

  File in = SD.open("/config/config.json", FILE_READ);
  if (!in) return false;

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, in);
  in.close();
  if (err) return false;

  doc["wifi"]["ssid"] = ssid;
  doc["wifi"]["password"] = password;

  // Arduino-ESP32 3.3.x: fs::File has no truncate().
  // Delete the old config and create a fresh file.
  SD.remove("/config/config.json");

  File out = SD.open("/config/config.json", FILE_WRITE);
  if (!out) return false;

  size_t written = serializeJsonPretty(doc, out);
  out.close();

  if (written == 0) return false;

  cfg.wifiSsid = ssid;
  cfg.wifiPassword = password;
  return true;
}

void notifyBleState() {
  if (!bleStarted || !bleStateCharacteristic) return;

  JsonDocument doc;
  doc["status"] = currentStatusIndex >= 0 ? statuses[currentStatusIndex].id : "none";
  doc["led_brightness"] = cfg.ledBrightness;
  doc["display_brightness"] = cfg.displayBrightness;
  doc["wifi_connected"] = wifiConnected;
  doc["ip"] = currentIpString();
  doc["ssid"] = cfg.wifiSsid;
  doc["wifi_rssi"] = wifiConnected ? WiFi.RSSI() : -127;
  doc["battery_percent"] = lastBatteryPercent;
  doc["battery_voltage"] = lastBatteryVoltage;

  String out;
  serializeJson(doc, out);

  bleStateCharacteristic->setValue(out.c_str());
  bleStateCharacteristic->notify();
}

class StatusAnzeigeBleCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    String cmd = String(characteristic->getValue().c_str());
    cmd.trim();

    Serial.printf("[BLE] CMD: %s\n", cmd.c_str());

    if (cmd.startsWith("status:")) {
      String id = cmd.substring(7);
      id.trim();
      int idx = findStatusIndex(id);
      if (idx >= 0) {
        confirmationFlash();
        activateStatus(idx, true);
        notifyBleState();
      }
      return;
    }

    if (cmd.startsWith("led:")) {
      int v = cmd.substring(4).toInt();
      cfg.ledBrightness = constrain(v, 0, 100);
      pixels.setBrightness(map(cfg.ledBrightness, 0, 100, 0, 255));
      if (currentStatusIndex >= 0) {
        setLedColor(statuses[currentStatusIndex].ledColor);
      }
      notifyBleState();
      return;
    }

    if (cmd.startsWith("display:")) {
      int v = cmd.substring(8).toInt();
      cfg.displayBrightness = constrain(v, 0, 100);
      applyDisplaySettings();
      notifyBleState();
      return;
    }

    if (cmd.startsWith("wifi:")) {
      String payload = cmd.substring(5);
      int sep = payload.indexOf('|');
      if (sep > 0) {
        String ssid = payload.substring(0, sep);
        String pass = payload.substring(sep + 1);
        ssid.trim();

        if (!ssid.isEmpty() && saveWifiConfigToSd(ssid, pass)) {
          if (bleStateCharacteristic) {
            bleStateCharacteristic->setValue("{\"ok\":true,\"wifi_saved\":true,\"reboot_required\":true}");
            bleStateCharacteristic->notify();
          }
        } else {
          if (bleStateCharacteristic) {
            bleStateCharacteristic->setValue("{\"ok\":false,\"error\":\"wifi_save_failed\"}");
            bleStateCharacteristic->notify();
          }
        }
      }
      return;
    }

    if (cmd == "reboot") {
      if (bleStateCharacteristic) {
        bleStateCharacteristic->setValue("{\"ok\":true,\"reboot\":true}");
        bleStateCharacteristic->notify();
      }
      delay(300);
      ESP.restart();
      return;
    }

    // Convenience: a bare status id also works.
    int idx = findStatusIndex(cmd);
    if (idx >= 0) {
      confirmationFlash();
      activateStatus(idx, true);
      notifyBleState();
    }
  }

  void onRead(BLECharacteristic *characteristic) override {
    notifyBleState();
  }
};

void startBle() {
  if (!cfg.bleEnabled) return;

  BLEDevice::init(cfg.bluetoothName.c_str());
  bleServer = BLEDevice::createServer();

  bleControlService = bleServer->createService(BLE_SERVICE_UUID);

  bleCommandCharacteristic = bleControlService->createCharacteristic(
    BLE_COMMAND_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );

  bleStateCharacteristic = bleControlService->createCharacteristic(
    BLE_STATE_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );

  bleCommandCharacteristic->setCallbacks(new StatusAnzeigeBleCallbacks());
  bleStateCharacteristic->setCallbacks(new StatusAnzeigeBleCallbacks());

  bleControlService->start();

  bleAdvertising = BLEDevice::getAdvertising();
  bleAdvertising->addServiceUUID(BLE_SERVICE_UUID);
  bleAdvertising->setScanResponse(true);
  bleAdvertising->start();

  bleStarted = true;

  Serial.printf("[BLE] Name: %s\n", cfg.bluetoothName.c_str());
  Serial.printf("[BLE] Service: %s\n", BLE_SERVICE_UUID);
  notifyBleState();
}

void startFtp() {
  if (!cfg.ftpEnabled || !sdReady) return;
  if (!wifiConnected && !apActive) return;

  ftp.begin(
    cfg.ftpUser.c_str(),
    cfg.ftpPassword.c_str(),
    "StatusAnzeige SD"
  );
  ftpStarted = true;

  Serial.printf("[FTP] Benutzer: %s\n", cfg.ftpUser.c_str());
  Serial.printf("[FTP] Server: ftp://%s/\n", currentIpString().c_str());
}

String currentIpString() {
  if (wifiConnected) return WiFi.localIP().toString();
  if (apActive) return WiFi.softAPIP().toString();
  return "0.0.0.0";
}


// -----------------------------------------------------------------------------
// Web-WLAN-Setup
// -----------------------------------------------------------------------------
String wifiSecurityName(uint8_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN: return "Offen";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
#ifdef WIFI_AUTH_WPA2_ENTERPRISE
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2 Enterprise";
#endif
#ifdef WIFI_AUTH_WPA3_PSK
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
#endif
#ifdef WIFI_AUTH_WPA2_WPA3_PSK
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
#endif
    default: return "Gesichert";
  }
}

String wifiSetupPage() {
  String html;
  html.reserve(9000);

  html += F(
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>StatusAnzeige WLAN</title>"
    "<style>"
    "body{margin:0;background:#080b10;color:#eef2f7;font-family:Arial,sans-serif}"
    ".wrap{max-width:720px;margin:auto;padding:22px}"
    ".card{background:#121822;border:1px solid #273142;border-radius:14px;padding:18px;margin-bottom:16px}"
    "h1,h2{margin-top:0}"
    "button,input{font:inherit}"
    "button{background:#1f6feb;color:#fff;border:0;border-radius:9px;padding:11px 14px;cursor:pointer}"
    "button.secondary{background:#283241}"
    "input{width:100%;box-sizing:border-box;background:#090d13;color:#fff;border:1px solid #39465a;"
    "border-radius:9px;padding:11px;margin:7px 0 13px}"
    ".net{display:flex;justify-content:space-between;gap:10px;align-items:center;"
    "padding:11px;border-bottom:1px solid #273142;cursor:pointer}"
    ".net:hover{background:#182130}"
    ".ssid{font-weight:700;overflow:hidden;text-overflow:ellipsis}"
    ".meta{font-size:12px;color:#9aa8ba;white-space:nowrap}"
    ".ok{color:#62d98b}.bad{color:#ff6b6b}.muted{color:#9aa8ba}"
    "a{color:#77aaff}"
    "</style></head><body><div class='wrap'>"
    "<h1>StatusAnzeige WLAN</h1>"
  );

  html += "<div class='card'><b>Aktuelle SSID:</b> " + cfg.wifiSsid;
  if (wifiConnected) {
    html += "<br><b>Verbunden:</b> <span class='ok'>Ja</span>";
    html += "<br><b>IP:</b> " + WiFi.localIP().toString();
    html += "<br><b>Signal:</b> " + String(WiFi.RSSI()) + " dBm";
  } else {
    html += "<br><b>Verbunden:</b> <span class='bad'>Nein</span>";
  }
  if (apActive) {
    html += "<br><b>Setup-AP:</b> " + cfg.fallbackApSsid;
    html += "<br><b>AP-IP:</b> " + WiFi.softAPIP().toString();
  }
  html += "</div>";

  html += F(
    "<div class='card'>"
    "<div style='display:flex;justify-content:space-between;align-items:center;gap:10px'>"
    "<h2 style='margin:0'>Verfügbare WLANs</h2>"
    "<button class='secondary' onclick='scanWifi()'>Neu scannen</button>"
    "</div>"
    "<div id='scanStatus' class='muted' style='margin:12px 0'>Noch nicht gescannt.</div>"
    "<div id='networks'></div>"
    "</div>"

    "<div class='card'>"
    "<h2>WLAN ändern</h2>"
    "<form id='wifiForm'>"
    "<label>SSID</label>"
    "<input id='ssid' name='ssid' autocomplete='off' required>"
    "<label>WLAN-Passwort</label>"
    "<input id='password' name='password' type='password' autocomplete='new-password'>"
    "<label>API-Passcode</label>"
    "<input id='ps' name='ps' type='password' inputmode='numeric' required>"
    "<button type='submit'>Speichern & Neustarten</button>"
    "</form>"
    "<div id='saveStatus' style='margin-top:12px'></div>"
    "</div>"

    "<div class='card'><a href='/'>Zurück zur StatusAnzeige</a></div>"

    "<script>"
    "function esc(s){return String(s).replace(/[&<>\"']/g,m=>({'&':'&amp;','<':'&lt;','>':'&gt;','\"':'&quot;',\"'\":'&#39;'}[m]));}"
    "async function scanWifi(){"
      "const st=document.getElementById('scanStatus');"
      "const box=document.getElementById('networks');"
      "st.textContent='Suche WLANs...';box.innerHTML='';"
      "try{"
        "const r=await fetch('/api/wifi/scan');"
        "const j=await r.json();"
        "if(!j.ok)throw new Error(j.error||'Scan fehlgeschlagen');"
        "st.textContent=j.networks.length+' WLAN(s) gefunden';"
        "j.networks.forEach(n=>{"
          "const d=document.createElement('div');d.className='net';"
          "d.innerHTML='<div><div class=\"ssid\">'+esc(n.ssid||'(versteckt)')+'</div>"
          "<div class=\"meta\">Kanal '+n.channel+' · '+esc(n.security)+'</div></div>"
          "<div class=\"meta\">'+n.rssi+' dBm</div>';"
          "if(n.ssid){d.onclick=()=>{document.getElementById('ssid').value=n.ssid;"
          "document.getElementById('password').focus();};}"
          "box.appendChild(d);"
        "});"
      "}catch(e){st.textContent='Fehler: '+e.message;}"
    "}"

    "document.getElementById('wifiForm').addEventListener('submit',async e=>{"
      "e.preventDefault();"
      "const out=document.getElementById('saveStatus');"
      "const fd=new URLSearchParams();"
      "fd.set('ssid',document.getElementById('ssid').value);"
      "fd.set('password',document.getElementById('password').value);"
      "const ps=document.getElementById('ps').value;"
      "out.textContent='Speichere...';"
      "try{"
        "const r=await fetch('/api/wifi/save?ps='+encodeURIComponent(ps),"
        "{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:fd});"
        "const j=await r.json();"
        "if(!j.ok)throw new Error(j.error||'Speichern fehlgeschlagen');"
        "out.innerHTML='<span class=\"ok\">Gespeichert. Gerät startet neu...</span>';"
      "}catch(e){out.innerHTML='<span class=\"bad\">Fehler: '+esc(e.message)+'</span>';}"
    "});"
    "scanWifi();"
    "</script></div></body></html>"
  );

  return html;
}

void handleWifiScan() {
  int16_t n = WiFi.scanNetworks(false, true, false, 180);

  JsonDocument doc;
  doc["ok"] = (n >= 0);
  JsonArray arr = doc["networks"].to<JsonArray>();

  if (n >= 0) {
    for (int i = 0; i < n; i++) {
      String ssid = WiFi.SSID(i);
      bool duplicate = false;
      for (JsonObject item : arr) {
        if (String((const char*)item["ssid"]) == ssid) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;

      JsonObject item = arr.add<JsonObject>();
      item["ssid"] = ssid;
      item["rssi"] = WiFi.RSSI(i);
      item["channel"] = WiFi.channel(i);
      item["security"] = wifiSecurityName(WiFi.encryptionType(i));
    }
  } else {
    doc["error"] = "wifi_scan_failed";
  }

  WiFi.scanDelete();

  String out;
  serializeJson(doc, out);
  web.send(n >= 0 ? 200 : 500, "application/json; charset=utf-8", out);
}

void handleWifiSave() {
  if (!checkApiPasscode()) return;

  if (!web.hasArg("ssid")) {
    web.send(400, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"SSID fehlt\"}");
    return;
  }

  String ssid = web.arg("ssid");
  String password = web.hasArg("password") ? web.arg("password") : "";
  ssid.trim();

  if (ssid.isEmpty() || ssid.length() > 32) {
    web.send(400, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"SSID ungueltig\"}");
    return;
  }

  if (!password.isEmpty() && password.length() < 8) {
    web.send(400, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"Passwort muss mindestens 8 Zeichen haben oder leer sein\"}");
    return;
  }

  if (!saveWifiConfigToSd(ssid, password)) {
    web.send(500, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"config.json konnte nicht gespeichert werden\"}");
    return;
  }

  web.send(200, "application/json; charset=utf-8",
           "{\"ok\":true,\"reboot\":true}");
  Serial.printf("[WIFI] Neue SSID ueber Web gespeichert: %s\n", ssid.c_str());
  delay(700);
  ESP.restart();
}



String configPage() {
  String html;
  html.reserve(14000);

  html += F(
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>StatusAnzeige Konfiguration</title>"
    "<style>"
    "body{margin:0;background:#080b10;color:#eef2f7;font-family:Arial,sans-serif}"
    ".wrap{max-width:820px;margin:auto;padding:22px}"
    ".card{background:#121822;border:1px solid #273142;border-radius:14px;padding:18px;margin-bottom:16px}"
    ".grid{display:grid;grid-template-columns:1fr 1fr;gap:14px}"
    "@media(max-width:680px){.grid{grid-template-columns:1fr}}"
    "label{display:block;font-size:13px;color:#aab5c5;margin-bottom:5px}"
    "input,select{width:100%;box-sizing:border-box;background:#090d13;color:#fff;border:1px solid #39465a;"
    "border-radius:9px;padding:10px;margin-bottom:12px}"
    "input[type=checkbox]{width:auto;transform:scale(1.2);margin-right:8px}"
    "button{background:#1f6feb;color:#fff;border:0;border-radius:9px;padding:11px 14px;cursor:pointer}"
    ".secondary{background:#283241}"
    ".ok{color:#62d98b}.bad{color:#ff6b6b}.muted{color:#9aa8ba}"
    "a{color:#77aaff}"
    "</style></head><body><div class='wrap'>"
    "<h1>StatusAnzeige Konfiguration</h1>"

    "<div class='card'><h2>API / Sicherheit</h2>"
    "<div class='grid'>"
    "<div><label>Aktueller API-Passcode</label><input id='auth_ps' type='password' inputmode='numeric' required placeholder='aktueller Passcode'></div>"
    "<div><label>Neuer API-Passcode</label><input id='new_api_passcode' type='password' inputmode='numeric' placeholder='leer = unverändert'></div>"
    "<div><label>Neuen API-Passcode bestätigen</label><input id='new_api_passcode2' type='password' inputmode='numeric' placeholder='noch einmal eingeben'></div>"
    "</div>"
    "<div class='muted'>Der aktuelle Passcode autorisiert die Änderung. Der neue Passcode wird in config.json gespeichert.</div></div>"

    "<div class='card'><h2>Anzeige & LEDs</h2><div class='grid'>"
    "<div><label>LED-Helligkeit (%)</label><input id='led_brightness' type='number' min='0' max='100'></div>"
    "<div><label>Display-Helligkeit (%)</label><input id='display_brightness' type='number' min='0' max='100'></div>"
    "<div><label>Display-Rotation</label><select id='rotation'>"
    "<option value='0'>0°</option><option value='1'>90°</option><option value='2'>180°</option><option value='3'>270°</option>"
    "</select></div>"
    "<div><label>Statusleiste Höhe (px)</label><input id='statusbar_height' type='number' min='16' max='50'></div>"
    "</div>"
    "<label><input id='statusbar_enabled' type='checkbox'>Statusleiste anzeigen</label>"
    "<label><input id='battery_enabled' type='checkbox'>Akkuanzeige aktiv</label>"
    "</div>"

    "<div class='card'><h2>WLAN</h2><div class='grid'>"
    "<div><label>SSID</label><input id='wifi_ssid'></div>"
    "<div><label>Neues WLAN-Passwort</label><input id='wifi_password' type='password' placeholder='leer = unverändert'></div>"
    "</div></div>"

    "<div class='card'><h2>FTP</h2><div class='grid'>"
    "<div><label>FTP Benutzer</label><input id='ftp_user'></div>"
    "<div><label>Neues FTP-Passwort</label><input id='ftp_password' type='password' placeholder='leer = unverändert'></div>"
    "</div><label><input id='ftp_enabled' type='checkbox'>FTP aktiv</label></div>"

    "<div class='card'><h2>MQTT / Home Assistant</h2><div class='grid'>"
    "<div><label>MQTT Host</label><input id='mqtt_host'></div>"
    "<div><label>Port</label><input id='mqtt_port' type='number' value='1883'></div>"
    "<div><label>Benutzer</label><input id='mqtt_user'></div>"
    "<div><label>Neues MQTT-Passwort</label><input id='mqtt_password' type='password' placeholder='leer = unverändert'></div>"
    "<div><label>Base Topic</label><input id='mqtt_base_topic'></div>"
    "</div><label><input id='mqtt_enabled' type='checkbox'>MQTT aktiv</label>"
    "<label><input id='mqtt_discovery' type='checkbox'>Home Assistant Discovery</label></div>"

    "<div class='card'><h2>Backup & weitere Seiten</h2>"
    "<a href='/config/backup'>config.json herunterladen</a> &nbsp; | &nbsp; <a href='/config/restore-page'>Wiederherstellen</a> &nbsp; | &nbsp; "
    "<a href='/statuses'>Statusfarben / Effekte</a> &nbsp; | &nbsp; <a href='/system'>Systemdiagnose</a> &nbsp; | &nbsp; <a href='/update'>Firmwareupdate</a>"
    "</div>"

    "<div class='card'><h2>Gerät</h2><div class='grid'>"
    "<div><label>Hostname</label><input id='hostname'></div>"
    "</div></div>"

    "<div class='card'>"
    "<button onclick='saveCfg()'>Konfiguration speichern</button> "
    "<button class='secondary' onclick='location.href=\"/\"'>Zurück</button>"
    "<div id='result' style='margin-top:12px'></div>"
    "</div>"

    "<script>"
    "function el(id){return document.getElementById(id)};"
    "async function loadCfg(){"
      "try{"
        "const r=await fetch('/api/config');const j=await r.json();"
        "if(!j.ok)throw new Error(j.error||'Laden fehlgeschlagen');"
        "el('led_brightness').value=j.led_brightness;"
        "el('display_brightness').value=j.display_brightness;"
        "el('rotation').value=j.rotation;"
        "el('statusbar_enabled').checked=j.statusbar_enabled;"
        "el('statusbar_height').value=j.statusbar_height;"
        "el('battery_enabled').checked=j.battery_enabled;"
        "el('wifi_ssid').value=j.wifi_ssid||'';"
        "el('ftp_enabled').checked=j.ftp_enabled;"
        "el('ftp_user').value=j.ftp_user||'';"
        "el('mqtt_enabled').checked=j.mqtt_enabled;"
        "el('mqtt_host').value=j.mqtt_host||'';"
        "el('mqtt_port').value=j.mqtt_port||1883;"
        "el('mqtt_user').value=j.mqtt_user||'';"
        "el('mqtt_base_topic').value=j.mqtt_base_topic||'statusanzeige';"
        "el('mqtt_discovery').checked=j.mqtt_discovery;"
        "el('hostname').value=j.hostname||'';"
      "}catch(e){el('result').innerHTML='<span class=\"bad\">'+e.message+'</span>';}"
    "}"
    "async function saveCfg(){"
      "const ps=el('auth_ps').value;"
      "if(!ps){el('result').innerHTML='<span class=\"bad\">Aktueller API-Passcode fehlt.</span>';return;}"
      "const np=el('new_api_passcode').value;"
      "const np2=el('new_api_passcode2').value;"
      "if(np!==np2){el('result').innerHTML='<span class=\"bad\">Die neuen API-Passcodes stimmen nicht überein.</span>';return;}"
      "const p=new URLSearchParams();"
      "p.set('led_brightness',el('led_brightness').value);"
      "p.set('display_brightness',el('display_brightness').value);"
      "p.set('rotation',el('rotation').value);"
      "p.set('statusbar_enabled',el('statusbar_enabled').checked?'1':'0');"
      "p.set('statusbar_height',el('statusbar_height').value);"
      "p.set('battery_enabled',el('battery_enabled').checked?'1':'0');"
      "p.set('wifi_ssid',el('wifi_ssid').value);"
      "p.set('wifi_password',el('wifi_password').value);"
      "p.set('ftp_enabled',el('ftp_enabled').checked?'1':'0');"
      "p.set('ftp_user',el('ftp_user').value);"
      "p.set('ftp_password',el('ftp_password').value);"
      "p.set('mqtt_enabled',el('mqtt_enabled').checked?'1':'0');"
      "p.set('mqtt_host',el('mqtt_host').value);"
      "p.set('mqtt_port',el('mqtt_port').value);"
      "p.set('mqtt_user',el('mqtt_user').value);"
      "p.set('mqtt_password',el('mqtt_password').value);"
      "p.set('mqtt_base_topic',el('mqtt_base_topic').value);"
      "p.set('mqtt_discovery',el('mqtt_discovery').checked?'1':'0');"
      "p.set('new_api_passcode',np);"
      "p.set('hostname',el('hostname').value);"
      "el('result').textContent='Speichere...';"
      "try{"
        "const r=await fetch('/api/config/save?ps='+encodeURIComponent(ps),{"
          "method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p"
        "});"
        "const j=await r.json();"
        "if(!j.ok)throw new Error(j.error||'Speichern fehlgeschlagen');"
        "if(np)sessionStorage.setItem('statusanzeige_ps',np);else sessionStorage.setItem('statusanzeige_ps',ps);"
        "el('result').innerHTML='<span class=\"ok\">Gespeichert. Neustart wird ausgeführt...</span>';"
      "}catch(e){el('result').innerHTML='<span class=\"bad\">'+e.message+'</span>';}"
    "}"
    "el('auth_ps').value=sessionStorage.getItem('statusanzeige_ps')||'';"
    "loadCfg();"
    "</script></div></body></html>"
  );

  return html;
}

void handleConfigGet() {
  JsonDocument doc;
  doc["ok"] = true;
  doc["led_brightness"] = cfg.ledBrightness;
  doc["display_brightness"] = cfg.displayBrightness;
  doc["rotation"] = cfg.displayRotation;
  doc["statusbar_enabled"] = cfg.statusBarEnabled;
  doc["statusbar_height"] = cfg.statusBarHeight;
  doc["battery_enabled"] = cfg.batteryEnabled;
  doc["wifi_ssid"] = cfg.wifiSsid;
  doc["ftp_enabled"] = cfg.ftpEnabled;
  doc["ftp_user"] = cfg.ftpUser;
  doc["mqtt_enabled"] = cfg.mqttEnabled;
  doc["mqtt_host"] = cfg.mqttHost;
  doc["mqtt_port"] = cfg.mqttPort;
  doc["mqtt_user"] = cfg.mqttUser;
  doc["mqtt_base_topic"] = cfg.mqttBaseTopic;
  doc["mqtt_discovery"] = cfg.mqttDiscovery;
  doc["hostname"] = cfg.hostname;

  String out;
  serializeJson(doc, out);
  web.send(200, "application/json; charset=utf-8", out);
}

void handleConfigSave() {
  if (!checkApiPasscode()) return;

  File f = SD.open("/config/config.json", FILE_READ);
  if (!f) {
    web.send(500, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"config.json nicht lesbar\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    web.send(500, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"config.json ungueltig\"}");
    return;
  }

  if (web.hasArg("led_brightness")) {
    doc["leds"]["brightness_percent"] = constrain(web.arg("led_brightness").toInt(), 0, 100);
  }

  if (web.hasArg("display_brightness")) {
    doc["display"]["brightness_percent"] = constrain(web.arg("display_brightness").toInt(), 0, 100);
  }

  if (web.hasArg("rotation")) {
    doc["display"]["rotation"] = constrain(web.arg("rotation").toInt(), 0, 3);
  }

  if (web.hasArg("statusbar_enabled")) {
    doc["status_bar"]["enabled"] = web.arg("statusbar_enabled") == "1";
  }

  if (web.hasArg("statusbar_height")) {
    doc["status_bar"]["height"] = constrain(web.arg("statusbar_height").toInt(), 16, 50);
  }

  if (web.hasArg("battery_enabled")) {
    doc["battery"]["enabled"] = web.arg("battery_enabled") == "1";
  }

  if (web.hasArg("wifi_ssid")) {
    String v = web.arg("wifi_ssid");
    v.trim();
    if (!v.isEmpty()) doc["wifi"]["ssid"] = v;
  }

  if (web.hasArg("wifi_password")) {
    String v = web.arg("wifi_password");
    if (!v.isEmpty()) doc["wifi"]["password"] = v;
  }

  if (web.hasArg("ftp_enabled")) {
    doc["ftp"]["enabled"] = web.arg("ftp_enabled") == "1";
  }

  if (web.hasArg("ftp_user")) {
    String v = web.arg("ftp_user");
    v.trim();
    if (!v.isEmpty()) doc["ftp"]["user"] = v;
  }

  if (web.hasArg("ftp_password")) {
    String v = web.arg("ftp_password");
    if (!v.isEmpty()) doc["ftp"]["password"] = v;
  }

  if (web.hasArg("mqtt_enabled")) doc["mqtt"]["enabled"] = web.arg("mqtt_enabled") == "1";
  if (web.hasArg("mqtt_host")) doc["mqtt"]["host"] = web.arg("mqtt_host");
  if (web.hasArg("mqtt_port")) doc["mqtt"]["port"] = constrain(web.arg("mqtt_port").toInt(), 1, 65535);
  if (web.hasArg("mqtt_user")) doc["mqtt"]["user"] = web.arg("mqtt_user");
  if (web.hasArg("mqtt_password")) { String v=web.arg("mqtt_password"); if(!v.isEmpty()) doc["mqtt"]["password"] = v; }
  if (web.hasArg("mqtt_base_topic")) { String v=web.arg("mqtt_base_topic"); v.trim(); if(!v.isEmpty()) doc["mqtt"]["base_topic"] = v; }
  if (web.hasArg("mqtt_discovery")) doc["mqtt"]["home_assistant_discovery"] = web.arg("mqtt_discovery") == "1";

  if (web.hasArg("new_api_passcode")) {
    String v = web.arg("new_api_passcode");
    v.trim();
    if (!v.isEmpty()) doc["api"]["passcode"] = v;
  }

  if (web.hasArg("hostname")) {
    String v = web.arg("hostname");
    v.trim();
    if (!v.isEmpty()) doc["device"]["hostname"] = v;
  }

  copySdFile("/config/config.json", "/config/config.bak");
  SD.remove("/config/config.json");
  File out = SD.open("/config/config.json", FILE_WRITE);
  if (!out) {
    web.send(500, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"config.json nicht schreibbar\"}");
    return;
  }

  if (serializeJsonPretty(doc, out) == 0) {
    out.close();
    web.send(500, "application/json; charset=utf-8",
             "{\"ok\":false,\"error\":\"Speichern fehlgeschlagen\"}");
    return;
  }

  out.close();

  web.send(200, "application/json; charset=utf-8",
           "{\"ok\":true,\"reboot\":true}");

  delay(700);
  ESP.restart();
}


void updateLowBatteryProtection() {
  if (!cfg.batteryEnabled || lastBatteryPercent < 0) return;
  bool shouldDim = lastBatteryPercent <= cfg.batteryDimPercent;
  if (shouldDim == lowBatteryDimActive) return;
  lowBatteryDimActive = shouldDim;
  if (shouldDim) {
    pixels.setBrightness(map(min((int)cfg.ledBrightness, 12), 0, 100, 0, 255));
    uint8_t duty = map(min((int)cfg.displayBrightness, 25), 0, 100, 0, 255);
    ledcWrite(PIN_LCD_BL, duty);
    Serial.printf("[BAT] Energiesparen aktiv (%d%%)\n", lastBatteryPercent);
  } else {
    pixels.setBrightness(map(constrain(cfg.ledBrightness,0,100),0,100,0,255));
    uint8_t duty = map(constrain(cfg.displayBrightness,0,100),0,100,0,255);
    ledcWrite(PIN_LCD_BL, duty);
    Serial.println("[BAT] Energiesparen beendet");
  }
}

String systemPage() {
  String h = "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#080b10;color:#eee;font-family:Arial;padding:20px}a{color:#7ab7ff}.c{background:#121822;padding:16px;border-radius:12px;margin:12px 0}</style></head><body>";
  h += "<h1>Systemdiagnose</h1><div class='c'>";
  h += "Firmware: " + String(STATUSANZEIGE_VERSION) + " (" + STATUSANZEIGE_BUILD + ")<br>";
  h += "Uptime: " + String(millis()/1000) + " s<br>";
  h += "IP: " + currentIpString() + "<br>";
  h += "RSSI: " + String(WiFi.status()==WL_CONNECTED ? WiFi.RSSI() : -127) + " dBm<br>";
  h += "Kanal: " + String(WiFi.channel()) + "<br>";
  h += "Akku: " + String(lastBatteryPercent) + " % / " + String(lastBatteryVoltage,2) + " V<br>";
  h += "Heap frei: " + String(ESP.getFreeHeap()) + "<br>PSRAM frei: " + String(ESP.getFreePsram()) + "<br>";
  h += "SD: " + String(sdReady?"OK":"FEHLER") + " | FTP: " + String(ftpStarted?"AN":"AUS") + " | BLE: " + String(bleStarted?"AN":"AUS") + " | MQTT: " + String(mqttStarted?"AN":"AUS") + "<br>";
  h += "Safe Mode: " + String(safeMode?"JA":"NEIN") + "</div><a href='/'>Zurueck</a></body></html>";
  return h;
}

String statusesConfigPage() {
  return String(F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'><style>body{background:#080b10;color:#eee;font-family:Arial;padding:18px}table{width:100%;border-collapse:collapse}td,th{padding:7px;border-bottom:1px solid #333}input,select,button{padding:7px;background:#111826;color:#fff;border:1px solid #3b4655;border-radius:6px}a{color:#7ab7ff}</style></head><body><h1>Status-Konfiguration</h1><div id='x'>Lade...</div><script>async function L(){let j=await (await fetch('/api/statuses')).json();let h='<table><tr><th>Status</th><th>Farbe</th><th>Effekt</th><th>Periode</th><th></th></tr>';j.statuses.forEach((s,i)=>{h+=`<tr><td>${s.label}</td><td><input id=c${i} type=color value=${s.color}></td><td><select id=e${i}><option>static</option><option>pulse</option><option>blink</option></select></td><td><input id=p${i} type=number value=${s.period} min=200 max=10000></td><td><button onclick=S(${i},'${s.id}')>Speichern</button></td></tr>`});h+='</table><p>API-Passcode: <input id=ps type=password></p><a href=/config>Zurueck</a>';x.innerHTML=h;j.statuses.forEach((s,i)=>document.getElementById('e'+i).value=s.effect)}async function S(i,id){let p=new URLSearchParams({id:id,color:document.getElementById('c'+i).value,effect:document.getElementById('e'+i).value,period:document.getElementById('p'+i).value});let r=await fetch('/api/statuses/save?ps='+encodeURIComponent(ps.value),{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p});alert((await r.json()).ok?'Gespeichert - Neustart':'Fehler')}L()</script></body></html>"));
}

void handleStatusesJson() {
  JsonDocument d; d["ok"] = true;
  JsonArray a = d["statuses"].to<JsonArray>();
  for (size_t i=0;i<statusCount;i++) {
    JsonObject o=a.add<JsonObject>(); o["id"]=statuses[i].id; o["label"]=statuses[i].label;
    char c[8]; snprintf(c,sizeof(c),"#%06lX",(unsigned long)statuses[i].ledColor); o["color"]=c;
    o["effect"]=statuses[i].ledEffect; o["period"]=statuses[i].effectPeriodMs;
  }
  String out; serializeJson(d,out); web.send(200,"application/json; charset=utf-8",out);
}

bool copySdFile(const char *src, const char *dst) {
  File in=SD.open(src,FILE_READ); if(!in)return false;
  SD.remove(dst); File out=SD.open(dst,FILE_WRITE); if(!out){in.close();return false;}
  uint8_t buf[512]; while(in.available()){size_t n=in.read(buf,sizeof(buf)); if(out.write(buf,n)!=n){in.close();out.close();return false;}}
  in.close(); out.close(); return true;
}

void handleStatusConfigSave() {
  if(!checkApiPasscode()) return;
  File f=SD.open("/config/config.json",FILE_READ); if(!f){web.send(500,"application/json","{\"ok\":false}");return;}
  JsonDocument d; if(deserializeJson(d,f)){f.close();web.send(500,"application/json","{\"ok\":false}");return;} f.close();
  String id=web.arg("id");
  for(JsonObject o:d["statuses"].as<JsonArray>()) if(String((const char*)(o["id"]|""))==id) {
    if(web.hasArg("color")) o["led_color"]=web.arg("color");
    if(web.hasArg("effect")) o["led_effect"]=web.arg("effect");
    if(web.hasArg("period")) o["effect_period_ms"]=constrain(web.arg("period").toInt(),200,10000);
  }
  copySdFile("/config/config.json","/config/config.bak"); SD.remove("/config/config.json");
  File w=SD.open("/config/config.json",FILE_WRITE); serializeJsonPretty(d,w); w.close();
  web.send(200,"application/json","{\"ok\":true}"); delay(400); ESP.restart();
}

void handleConfigBackup() {
  File f=SD.open("/config/config.json",FILE_READ); if(!f){web.send(404,"text/plain","config fehlt");return;}
  web.sendHeader("Content-Disposition","attachment; filename=config.json"); web.streamFile(f,"application/json"); f.close();
}

String restorePage() {
  return String(F("<!doctype html><html><body style='background:#080b10;color:#eee;font-family:Arial;padding:20px'><h1>Config wiederherstellen</h1><form id=f><input id=j type=file accept=.json required><p>API-Passcode: <input id=p type=password required></p><button>config.json hochladen</button></form><p><button onclick=R()>config.bak wiederherstellen</button></p><pre id=o></pre><script>f.onsubmit=async e=>{e.preventDefault();let d=new FormData();d.append('config',j.files[0]);let r=await fetch('/config/restore?ps='+encodeURIComponent(p.value),{method:'POST',body:d});o.textContent=await r.text()};async function R(){let r=await fetch('/config/restore-backup?ps='+encodeURIComponent(p.value),{method:'POST'});o.textContent=await r.text()}</script><a href='/config'>Zurueck</a></body></html>"));
}

void handleConfigRestoreUpload() {
  HTTPUpload &u = web.upload();
  if (u.status == UPLOAD_FILE_START) {
    configRestoreAuthorized = web.arg("ps") == cfg.apiPasscode;
    if (!configRestoreAuthorized) return;
    SD.remove("/config/config.upload.json");
    configRestoreFile = SD.open("/config/config.upload.json", FILE_WRITE);
  } else if (u.status == UPLOAD_FILE_WRITE) {
    if (configRestoreAuthorized && configRestoreFile) configRestoreFile.write(u.buf, u.currentSize);
  } else if (u.status == UPLOAD_FILE_END) {
    if (configRestoreFile) configRestoreFile.close();
  }
}

void handleConfigRestoreDone() {
  if (!configRestoreAuthorized) { web.send(401,"text/plain","Passcode falsch"); return; }
  File f=SD.open("/config/config.upload.json",FILE_READ); JsonDocument d;
  if(!f || deserializeJson(d,f)){ if(f)f.close(); SD.remove("/config/config.upload.json"); web.send(400,"text/plain","JSON ungueltig"); return; }
  f.close();
  copySdFile("/config/config.json","/config/config.bak");
  SD.remove("/config/config.json"); SD.rename("/config/config.upload.json","/config/config.json");
  web.send(200,"text/plain","Config wiederhergestellt - Neustart"); delay(500); ESP.restart();
}

String updatePage() {
  return String(F("<!doctype html><html><body style='background:#080b10;color:#eee;font-family:Arial;padding:20px'><h1>Firmware Update</h1><p>Nur .bin aus Arduino/PlatformIO verwenden.</p><form id=f><input id=b type=file accept=.bin required><p>API-Passcode: <input id=p type=password required></p><button>Update starten</button></form><pre id=o></pre><script>f.onsubmit=async e=>{e.preventDefault();let d=new FormData();d.append('firmware',b.files[0]);let r=await fetch('/update?ps='+encodeURIComponent(p.value),{method:'POST',body:d});o.textContent=await r.text()}</script><p><a href='/'>Zurueck</a></p></body></html>"));
}

void handleFirmwareUpload() {
  if (!cfg.webUpdateEnabled || !checkApiPasscode()) return;
  HTTPUpload &u=web.upload();
  if(u.status==UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
  else if(u.status==UPLOAD_FILE_WRITE) Update.write(u.buf,u.currentSize);
  else if(u.status==UPLOAD_FILE_END) Update.end(true);
}

void handleFirmwareUploadDone() {
  if (!cfg.webUpdateEnabled) { web.send(403,"text/plain","Webupdate deaktiviert"); return; }
  if(Update.hasError()) web.send(500,"text/plain","Update fehlgeschlagen");
  else { web.send(200,"text/plain","Update OK - Neustart"); delay(500); ESP.restart(); }
}

// -----------------------------------------------------------------------------
// HTTP
// -----------------------------------------------------------------------------
String buildHomePage() {
  String html;
  html.reserve(11000);

  html += F(
    "<!doctype html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>StatusAnzeige</title>"
    "<style>"
    "body{background:#080b10;color:#eee;font-family:Arial;margin:24px}"
    ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}"
    ".statusBtn,.actionBtn{padding:15px;background:#151b25;border:1px solid #202a38;"
    "border-radius:10px;color:white;text-align:center;cursor:pointer;font:inherit}"
    ".statusBtn:hover,.actionBtn:hover{background:#1b2432}"
    ".statusBtn.active{outline:2px solid #4e9cff;background:#192b43}"
    ".nav{display:flex;flex-wrap:wrap;gap:10px;margin:18px 0}"
    ".nav a{padding:10px 12px;background:#151b25;border-radius:9px;color:white;text-decoration:none}"
    ".card{background:#101620;border:1px solid #202a38;border-radius:12px;padding:14px;margin:15px 0}"
    ".small{color:#9aa4b2;font-size:13px}code{color:#8cf}"
    "input{background:#090d13;color:#fff;border:1px solid #39465a;border-radius:8px;padding:9px}"
    ".ok{color:#62d98b}.bad{color:#ff6b6b}.muted{color:#9aa8ba}"
    "</style></head><body>"
    "<h2>StatusAnzeige</h2>"

    "<div class='card'>"
    "<label>API-Passcode: <input id='apiPs' type='password' inputmode='numeric' size='12'></label> "
    "<button class='actionBtn' style='padding:9px 12px' onclick='savePs()'>merken</button>"
    "<div class='small'>Wird nur für diese Browser-Sitzung gespeichert.</div>"
    "</div>"

    "<div id='statusGrid' class='grid'>"
  );

  for (size_t i = 0; i < statusCount; i++) {
    html += "<button class='statusBtn";
    if ((int)i == currentStatusIndex) html += " active";
    html += "' id='st_" + statuses[i].id + "' onclick=\"setStatus('" + statuses[i].id + "')\">";
    html += statuses[i].label;
    html += "</button>";
  }

  html += F(
    "</div>"
    "<div class='card'>"
    "<div id='uiMsg' class='muted'>Bereit.</div>"
    "</div>"

    "<div class='nav'>"
    "<a href='/wifi'>WLAN</a>"
    "<a href='/config'>Konfiguration</a>"
    "<a href='/statuses'>Statusfarben</a>"
    "<a href='/system'>System</a>"
    "<a href='/update'>Update</a>"
    "</div>"

    "<div class='card'><b>Systemaktionen</b><div class='nav'>"
    "<button class='actionBtn' onclick=\"apiAction('/api/refresh','Bild neu geladen')\">Bild neu laden</button>"
    "<button class='actionBtn' onclick=\"apiAction('/api/reload','Config neu geladen')\">Config neu laden</button>"
    "<button class='actionBtn' onclick='rebootDevice()'>Neustart</button>"
    "</div></div>"

    "<div class='small'>"
  );

  html += "IP: " + currentIpString() + "<br>";
  html += "FTP: <code>ftp://" + currentIpString() + "/</code><br>";
  html += "mDNS: <code>http://" + cfg.hostname + ".local/</code><br>";
  html += "<a style='color:#7ab7ff' href='/api/info' target='_blank'>API-Info</a>";
  html += F(
    "</div>"

    "<script>"
    "const psEl=document.getElementById('apiPs');"
    "const msg=document.getElementById('uiMsg');"
    "function getPs(){return psEl.value.trim();}"
    "function savePs(){sessionStorage.setItem('statusanzeige_ps',getPs());showMsg('API-Passcode für diese Sitzung gemerkt.',true);}"
    "function showMsg(t,ok){msg.textContent=t;msg.className=ok?'ok':'bad';}"
    "async function callApi(path){"
      "const ps=getPs();"
      "if(!ps){showMsg('Bitte API-Passcode eingeben.',false);throw new Error('Passcode fehlt');}"
      "sessionStorage.setItem('statusanzeige_ps',ps);"
      "const sep=path.includes('?')?'&':'?';"
      "const r=await fetch(path+sep+'ps='+encodeURIComponent(ps),{cache:'no-store'});"
      "let j={};try{j=await r.json();}catch(e){}"
      "if(!r.ok||j.ok===false)throw new Error(j.error||('HTTP '+r.status));"
      "return j;"
    "}"
    "async function setStatus(id){"
      "try{"
        "const j=await callApi('/api/status?id='+encodeURIComponent(id));"
        "document.querySelectorAll('.statusBtn').forEach(b=>b.classList.remove('active'));"
        "const b=document.getElementById('st_'+id);if(b)b.classList.add('active');"
        "showMsg('Status gesetzt: '+(j.status||id),true);"
      "}catch(e){showMsg('Fehler: '+e.message,false);}"
    "}"
    "async function apiAction(path,okText){try{await callApi(path);showMsg(okText,true);}catch(e){showMsg('Fehler: '+e.message,false);}}"
    "async function rebootDevice(){"
      "try{await callApi('/api/reboot');showMsg('Neustart ausgelöst. Die Seite bleibt geöffnet und verbindet sich gleich wieder.',true);setTimeout(waitForDevice,2500);}"
      "catch(e){showMsg('Fehler: '+e.message,false);}"
    "}"
    "async function waitForDevice(){"
      "for(let i=0;i<20;i++){"
        "try{const r=await fetch('/api/info',{cache:'no-store'});if(r.ok){showMsg('Gerät ist wieder erreichbar.',true);return;}}catch(e){}"
        "await new Promise(r=>setTimeout(r,1000));"
      "}"
      "showMsg('Gerät startet noch oder ist unter dieser Adresse nicht erreichbar.',false);"
    "}"
    "psEl.value=sessionStorage.getItem('statusanzeige_ps')||'';"
    "</script></body></html>"
  );

  return html;
}

void sendInfoJson() {
  JsonDocument doc;
  doc["device"] = cfg.deviceName;
  doc["hostname"] = cfg.hostname;
  doc["ip"] = currentIpString();
  doc["wifi_connected"] = wifiConnected;
  doc["fallback_ap"] = apActive;
  doc["ap_ssid"] = apActive ? cfg.fallbackApSsid : "";
  doc["ap_ip"] = apActive ? WiFi.softAPIP().toString() : "";
  doc["ap_clients"] = apActive ? WiFi.softAPgetStationNum() : 0;
  doc["ftp"] = ftpStarted;
  doc["ble"] = bleStarted;
  doc["api_passcode_required"] = cfg.apiEnabled;
  doc["clock_enabled"] = cfg.clockEnabled;
  doc["clock_synced"] = timeSynced;
  doc["battery_percent"] = lastBatteryPercent;
  doc["battery_voltage"] = lastBatteryVoltage;
  doc["wifi_rssi"] = wifiConnected ? WiFi.RSSI() : -127;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["psram_free"] = ESP.getFreePsram();
  doc["sd_total_mb"] = sdReady ? SD.cardSize() / (1024ULL * 1024ULL) : 0;
  doc["status"] =
    currentStatusIndex >= 0 ? statuses[currentStatusIndex].id : "none";
  doc["menu"] = menuActive;

  String out;
  serializeJsonPretty(doc, out);
  web.send(200, "application/json; charset=utf-8", out);
}

bool checkApiPasscode() {
  if (!cfg.apiEnabled) return true;

  if (!web.hasArg("ps")) {
    web.send(
      401,
      "application/json; charset=utf-8",
      "{\"ok\":false,\"error\":\"Passcode fehlt\"}"
    );
    return false;
  }

  if (web.arg("ps") != cfg.apiPasscode) {
    web.send(
      403,
      "application/json; charset=utf-8",
      "{\"ok\":false,\"error\":\"Passcode falsch\"}"
    );
    return false;
  }

  return true;
}

void startWeb() {
  if (!wifiConnected && !apActive) return;

  web.on("/config", HTTP_GET, []() {
    web.send(200, "text/html; charset=utf-8", configPage());
  });

  web.on("/api/config", HTTP_GET, []() {
    handleConfigGet();
  });

  web.on("/api/config/save", HTTP_POST, []() {
    handleConfigSave();
  });

web.on("/system", HTTP_GET, [](){ web.send(200,"text/html; charset=utf-8",systemPage()); });
web.on("/statuses", HTTP_GET, [](){ web.send(200,"text/html; charset=utf-8",statusesConfigPage()); });
web.on("/api/statuses", HTTP_GET, [](){ handleStatusesJson(); });
web.on("/api/statuses/save", HTTP_POST, [](){ handleStatusConfigSave(); });
web.on("/config/backup", HTTP_GET, [](){ handleConfigBackup(); });
web.on("/config/restore-page", HTTP_GET, [](){ web.send(200,"text/html; charset=utf-8",restorePage()); });
web.on("/config/restore", HTTP_POST, [](){ handleConfigRestoreDone(); }, [](){ handleConfigRestoreUpload(); });
web.on("/config/restore-backup", HTTP_POST, [](){
  if(!checkApiPasscode()) return;
  if(!SD.exists("/config/config.bak")){web.send(404,"text/plain","Kein config.bak vorhanden");return;}
  copySdFile("/config/config.bak","/config/config.json");
  web.send(200,"text/plain","Backup wiederhergestellt - Neustart");
  delay(500); ESP.restart();
});
web.on("/update", HTTP_GET, [](){ web.send(200,"text/html; charset=utf-8",updatePage()); });
web.on("/update", HTTP_POST, [](){ handleFirmwareUploadDone(); }, [](){ handleFirmwareUpload(); });

  web.on("/wifi", HTTP_GET, []() {
    web.send(200, "text/html; charset=utf-8", wifiSetupPage());
  });

  web.on("/api/wifi/scan", HTTP_GET, []() {
    handleWifiScan();
  });

  web.on("/api/wifi/save", HTTP_POST, []() {
    handleWifiSave();
  });

  web.on("/", HTTP_GET, []() {
    web.send(200, "text/html; charset=utf-8", buildHomePage());
  });

  web.on("/api/info", HTTP_GET, []() {
    sendInfoJson();
  });

  web.on("/api/status", HTTP_GET, []() {
    if (!checkApiPasscode()) return;

    if (!web.hasArg("id")) {
      web.send(400, "application/json", "{\"ok\":false,\"error\":\"id fehlt\"}");
      return;
    }

    int idx = findStatusIndex(web.arg("id"));
    if (idx < 0) {
      web.send(404, "application/json", "{\"ok\":false,\"error\":\"Status unbekannt\"}");
      return;
    }

    confirmationFlash();
    activateStatus(idx, true);

    String response = "{\"ok\":true,\"status\":\"" + statuses[idx].id + "\"}";
    web.send(200, "application/json", response);
  });

  web.on("/api/refresh", HTTP_GET, []() {
    if (!checkApiPasscode()) return;

    if (currentStatusIndex >= 0) {
      drawPngFromSd(statuses[currentStatusIndex].image);
      drawStatusBar(true);
      drawClockFooter(true);
      setLedColor(statuses[currentStatusIndex].ledColor);
    }
    web.send(200, "application/json", "{\"ok\":true}");
  });

  web.on("/api/reload", HTTP_GET, []() {
    if (!checkApiPasscode()) return;

    bool ok = loadConfig();
    applyDisplaySettings();
    if (currentStatusIndex >= 0) {
      int idx = findStatusIndex(statuses[currentStatusIndex].id);
      if (idx < 0) idx = findStatusIndex(cfg.defaultStatus);
      if (idx >= 0) activateStatus(idx, false);
    }
    web.send(ok ? 200 : 500, "application/json",
             ok ? "{\"ok\":true}" : "{\"ok\":false}");
  });

  web.on("/api/reboot", HTTP_GET, []() {
    if (!checkApiPasscode()) return;

    web.send(200, "application/json", "{\"ok\":true,\"reboot\":true}");
    delay(250);
    ESP.restart();
  });

  web.onNotFound([](){
    if (apActive && cfg.captivePortalEnabled) {
      web.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/wifi", true);
      web.send(302, "text/plain", "");
    } else web.send(404, "text/plain", "Not found");
  });
  web.begin();
  webStarted = true;
  Serial.printf("[HTTP] http://%s/\n", currentIpString().c_str());
}

// -----------------------------------------------------------------------------
// FTP/SD-Steuerdateien
// -----------------------------------------------------------------------------
void serviceFlags() {
  if (!sdReady) return;
  if (millis() - lastServiceCheck < 1500) return;
  lastServiceCheck = millis();

  if (SD.exists("/config/reload.flag")) {
    SD.remove("/config/reload.flag");
    Serial.println("[FLAG] reload.flag");
    loadConfig();
    applyDisplaySettings();

    String lastId =
      currentStatusIndex >= 0 ? statuses[currentStatusIndex].id : cfg.defaultStatus;
    int idx = findStatusIndex(lastId);
    if (idx < 0) idx = findStatusIndex(cfg.defaultStatus);
    if (idx >= 0) activateStatus(idx, false);
  }

  if (SD.exists("/config/reboot.flag")) {
    SD.remove("/config/reboot.flag");
    Serial.println("[FLAG] reboot.flag");
    delay(200);
    ESP.restart();
  }
}

// -----------------------------------------------------------------------------
// Firmware-Update von SD
// -----------------------------------------------------------------------------
bool checkAndApplySdFirmwareUpdate() {
  if (!cfg.sdUpdateEnabled || !sdReady) return false;
  if (!SD.exists(cfg.firmwarePath)) return false;
  if (!SD.exists(cfg.updateFlagPath)) {
    Serial.println("[UPDATE] firmware.bin gefunden, aber update.flag fehlt");
    return false;
  }

  File bin = SD.open(cfg.firmwarePath, FILE_READ);
  if (!bin || bin.isDirectory()) {
    Serial.println("[UPDATE] firmware.bin kann nicht gelesen werden");
    return false;
  }

  size_t size = bin.size();
  if (size < 100000) {
    Serial.println("[UPDATE] firmware.bin unplausibel klein - Abbruch");
    bin.close();
    return false;
  }

  showTextScreen("FIRMWARE UPDATE", "Bitte nicht ausschalten", "Update laeuft...");
  setLedColor(0x0000FF);

  Serial.printf("[UPDATE] Starte, %u Bytes\n", (unsigned)size);

  if (!Update.begin(size, U_FLASH)) {
    Update.printError(Serial);
    bin.close();
    return false;
  }

  size_t written = Update.writeStream(bin);
  bin.close();

  if (written != size) {
    Serial.printf("[UPDATE] geschrieben %u / %u\n",
                  (unsigned)written, (unsigned)size);
  }

  if (!Update.end()) {
    Update.printError(Serial);
    showTextScreen("UPDATE FEHLER", "Siehe Serial");
    setLedColor(0xFF0000);
    return false;
  }

  if (!Update.isFinished()) {
    Serial.println("[UPDATE] Update nicht vollstaendig");
    showTextScreen("UPDATE FEHLER", "Nicht vollstaendig");
    setLedColor(0xFF0000);
    return false;
  }

  SD.remove(cfg.updateFlagPath);
  if (SD.exists(cfg.doneFirmwarePath)) SD.remove(cfg.doneFirmwarePath);
  SD.rename(cfg.firmwarePath, cfg.doneFirmwarePath);

  showTextScreen("UPDATE OK", "Neustart...");
  setLedColor(0x00FF00);
  delay(1200);
  ESP.restart();
  return true;
}

// -----------------------------------------------------------------------------
// Setup / Loop
// -----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("========================================");
  Serial.println(" StatusAnzeige To Go");
  Serial.println(" Waveshare ESP32-S3-Touch-LCD-2");
  Serial.println("========================================");

  esp_reset_reason_t rr = esp_reset_reason();
  if (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT) crashBootCounter++;
  else if (rr == ESP_RST_POWERON) crashBootCounter = 0;
  safeMode = crashBootCounter >= 3;
  if (safeMode) Serial.println("[SAFE] Safe Mode aktiv");

  if (!initDisplayAndSd()) {
    // Ohne SD bleibt das Gerät als Diagnose aktiv.
    pinMode(DEFAULT_BUTTON_PIN, INPUT_PULLUP);
    return;
  }

  loadConfig();
  applyDisplaySettings();

  pinMode(cfg.buttonPin, INPUT_PULLUP);
  rawButtonState = stableButtonState = digitalRead(cfg.buttonPin);

  initLeds();

  // Keine Regenbogenanimation beim Start.
  // Die LEDs werden nach dem Laden des letzten Status direkt
  // auf dessen feste Statusfarbe gesetzt.
  checkAndApplySdFirmwareUpdate();

  String startId = loadLastStatus();
  if (startId.isEmpty()) startId = cfg.defaultStatus;

  int idx = findStatusIndex(startId);
  if (idx < 0) idx = findStatusIndex(cfg.defaultStatus);
  if (idx < 0 && statusCount > 0) idx = 0;

  if (idx >= 0) activateStatus(idx, false);
  else showTextScreen("KEINE STATUS", "config.json pruefen");

  connectNetwork();
  initBatteryAdc();
  initClockSync();
  initTouch();
  if (!safeMode) {
    startBle();
    startFtp();
    initMqtt();
  }
  startWeb();

  Serial.println("[SYSTEM] Bereit");
}

void loop() {
  if (!sdReady) {
    delay(100);
    return;
  }

  updateButton();
  updateTouch();
  maintainWifiConnection();
  maintainMqtt();

  if (menuActive) {
    // Im Menü ausschließlich dunkelblau pulsieren.
    updateMenuPulse();
  }

  updateStatusLedEffect();
  drawStatusBar(false);
  updateLowBatteryProtection();
  drawClockFooter(false);

  if (webStarted) web.handleClient();
  if (apActive && cfg.captivePortalEnabled) dnsServer.processNextRequest();
  if (ftpStarted) ftp.handleFTP();

  serviceFlags();
  if (crashBootCounter && millis() > 30000) crashBootCounter = 0;

  delay(2);
}
