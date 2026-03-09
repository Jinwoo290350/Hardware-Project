/*
 * =============================================
 * ELDERLY SAFETY SYSTEM  —  USER-SIDE DEVICE
 * =============================================
 * สวมใส่บนผู้สูงอายุ (Chest / Shirt / Pants)
 *
 * Hardware:
 *   ESP32-S3
 *   MPU6050  (I2C: SDA=8, SCL=9)   shared bus with OLED
 *   OLED SSD1306 128x64 (I2C 0x3C, SDA=8, SCL=9)
 *   GPS Module (UART RX=16, read-only)
 *   LED Green=4  Yellow=5  Red=6
 *   Buzzer GPIO 7
 *   BTN_MODE=10  BTN_EMERGENCY=13
 *
 * Libraries (Arduino Library Manager):
 *   - Blynk  by Volodymyr Shymanskyy
 *   - Adafruit MPU6050
 *   - Adafruit Unified Sensor
 *   - Adafruit GFX Library
 *   - Adafruit SSD1306
 *   - TinyGPS  by Mikal Hart
 *   - ArduinoFFT  by Enrique Condes   ← NEW (ต้องติดตั้งจาก Library Manager)
 *
 * Blynk Virtual Pins:
 *   V0 – Status   0=NORMAL 1=WARNING 2=FALL
 *   V1 – GPS Lat  (float)
 *   V2 – GPS Lon  (float)
 *   V3 – Mode     0=CHEST 1=SHIRT 2=PANTS
 *   V4 – Emergency 0/1
 *
 * ML Model headers (สร้างจาก ML/ESP32_export.ipynb ก่อน):
 *   CHEST_model.h  →  FallDetectorChest::predict()
 *   SHIRT_model.h  →  FallDetectorShirt::predict()
 *   PANTS_model.h  →  FallDetectorPants::predict()
 *
 * ⚠️  กรอก BLYNK_TEMPLATE_ID / AUTH_TOKEN / WiFi ก่อน upload
 * =============================================
 */

// ===== CONFIG (Blynk + WiFi credentials) =====
// BLYNK_TEMPLATE_ID / AUTH_TOKEN / WiFi อยู่ใน config.h
#include "config.h"

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "driver/gpio.h"   // ESP-IDF native GPIO API (ใช้ใน silenceBuzzer)
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS.h>
#include <EEPROM.h>
// ArduinoFFT removed — new 22-feature model doesn't use FFT

// ML model headers — สร้างจาก ML/ESP32_export.ipynb แล้ว copy มาไว้ใน sketch folder
#include "CHEST_model.h"
#include "SHIRT_model.h"
#include "PANTS_model.h"

// Bring model classes into global scope (they live in Eloquent::ML::Port namespace)
using namespace Eloquent::ML::Port;

// Model instances (predict() is an instance method, not static)
FallDetectorChest chestModel;
FallDetectorShirt shirtModel;
FallDetectorPants pantsModel;

// WiFi credentials มาจาก config.h (WIFI_SSID / WIFI_PASS)

// ===== PIN DEFINITIONS =====
#define LED_GREEN      4    // LED เขียว
#define LED_YELLOW     5    // LED เหลือง
#define LED_RED        6    // LED แดง

#define BTN_MODE       10   // ปุ่มสลับ mode (short) / clear emergency
#define BTN_EMERGENCY  13   // ปุ่ม SOS / cancel SOS
#define LONG_PRESS_MS  1500 // กดค้าง BTN_MODE >= 1.5s ขณะปกติ = SOS (fallback)

#define BUZZER         7    // Buzzer

#define OLED_SDA        8   // I2C SDA (shared bus: OLED 0x3C + MPU6050 0x68)
#define OLED_SCL        9   // I2C SCL (GPIO9 = SCL only — ห้ามใช้เป็น GPIO อื่น)

// MPU6050: SDA→GPIO8, SCL→GPIO9, VCC→3.3V, GND→GND, AD0→GND
// (AD0→GND → I2C address = 0x68, ไม่ชนกับ OLED 0x3C)

#define GPS_RX         16   // ESP32 RX ← GPS module TX  (รับ NMEA จาก GPS)
#define GPS_TX         17   // ESP32 TX → GPS module RX  (ส่งคำสั่งไป GPS ถ้าต้องการ)

// ===== OLED =====
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
bool oledOK = false;  // set true only when OLED is confirmed on I2C bus

// ===== MPU6050 =====
Adafruit_MPU6050 mpu;
bool    mpuUseRaw = false;   // true when WHO_AM_I=0x70 clone (Adafruit rejects it)
uint8_t mpuI2CAddr = 0x68;  // actual I2C address found during setup (0x68 or 0x69)
float   mpuAccelScale = 4096.0f; // LSB/g — set in setup() by reading ACCEL_CONFIG back

// Raw MPU6050 read — works for 0x70 clones without Adafruit library
// Scale is auto-detected from ACCEL_CONFIG readback in setup()
// ±500°/s gyro (65.5 LSB/°/s)
// Last-good accelerometer reading (safe fallback if I2C times out)
static float mpuLastAx = 0.0f, mpuLastAy = 0.0f, mpuLastAz = 9.80665f;

void mpuGetRaw(sensors_event_t *a, sensors_event_t *g, sensors_event_t *t) {
  Wire.beginTransmission(mpuI2CAddr);
  Wire.write(0x3B);  // ACCEL_XOUT_H
  Wire.endTransmission(true);
  Wire.requestFrom(mpuI2CAddr, (uint8_t)14);
  if (Wire.available() < 14) {
    // I2C timeout — keep last good reading to avoid false fall/garbage
    a->acceleration.x = mpuLastAx;
    a->acceleration.y = mpuLastAy;
    a->acceleration.z = mpuLastAz;
    return;
  }
  int16_t rax = (Wire.read()<<8)|Wire.read();
  int16_t ray = (Wire.read()<<8)|Wire.read();
  int16_t raz = (Wire.read()<<8)|Wire.read();
  int16_t rt  = (Wire.read()<<8)|Wire.read();
  int16_t rgx = (Wire.read()<<8)|Wire.read();
  int16_t rgy = (Wire.read()<<8)|Wire.read();
  int16_t rgz = (Wire.read()<<8)|Wire.read();
  a->acceleration.x = rax * (9.80665f / mpuAccelScale);
  a->acceleration.y = ray * (9.80665f / mpuAccelScale);
  a->acceleration.z = raz * (9.80665f / mpuAccelScale);
  g->gyro.x = rgx * (M_PI / (180.0f * 65.5f));
  g->gyro.y = rgy * (M_PI / (180.0f * 65.5f));
  g->gyro.z = rgz * (M_PI / (180.0f * 65.5f));
  t->temperature = rt / 340.0f + 36.53f;
  // Update last-good cache
  mpuLastAx = a->acceleration.x;
  mpuLastAy = a->acceleration.y;
  mpuLastAz = a->acceleration.z;
}

// Drop-in replacement for mpu.getEvent()
void getMPUEvent(sensors_event_t *a, sensors_event_t *g, sensors_event_t *t) {
  if (mpuUseRaw) mpuGetRaw(a, g, t);
  else mpu.getEvent(a, g, t);
}

// ===== GPS =====
TinyGPS gps;
HardwareSerial GPSSerial(2);
// Default = คณะวิศวกรรมศาสตร์ มหาวิทยาลัยเกษตรศาสตร์ (ใช้เมื่อ GPS หาสัญญาณไม่เจอ)
float gpsLat = 13.8474f, gpsLon = 100.5693f;

// ===== EEPROM =====
#define EEPROM_SIZE      1
#define EEPROM_MODE_ADDR 0

// ===== MODE =====
// enum Mode defined in config.h (ต้องอยู่ก่อน auto-generated prototypes)
Mode currentMode = CHEST;

// ===== FALL DETECTION STATE MACHINE =====
enum FallState {
  FALL_IDLE,
  FALL_FREEFALL,
  FALL_IMPACT,
  FALL_VERIFY,
  FALL_EMERGENCY
};
FallState fallState = FALL_IDLE;

// ===== SYSTEM STATE (SETUP → MONITOR) =====
enum SysState { SYS_SETUP, SYS_MONITOR };
SysState sysState = SYS_MONITOR;  // เริ่ม monitor ทันที

// Fall detection thresholds (หน่วย m/s²)
// ── ปรับค่าเหล่านี้จากผล notebook analysis ──
const float FREEFALL_THRESHOLD  = 3.0f;   // < นี้ = freefall  (tune: min fall mag - margin)
const float IMPACT_THRESHOLD    = 14.7f;  // > นี้ = impact    (tune: p75 ของ fall impact)
const float STILL_MAG_MIN       = 6.5f;   // post-fall: ยังมีแรงโน้มถ่วง
const float STILL_MAG_MAX       = 13.0f;  // post-fall: ยังนิ่งอยู่กับพื้น

// ── Jerk (ความเร็วการเปลี่ยนแปลง) ──────────────────────────
// jerk = |Δacc_vec| ระหว่างสอง sample ที่ห่างกัน 200ms (5Hz)
// การพลิกตัวฉับพลัน/สะดุด: jerk สูงมาก แม้ไม่มี freefall ชัดเจน
const float JERK_THRESHOLD      = 22.0f;  // m/s² ต่อ sample — tune จากข้อมูล
//   walk/run jerk ≈ 3-12  m/s²  → ต้องต่ำกว่า JERK_THRESHOLD
//   fall jerk   ≈ 20-60+ m/s²  → ต้องสูงกว่า JERK_THRESHOLD
// วิธี tune: Serial monitor → "[MPU] jerk=" ขณะเดิน/วิ่ง/ล้ม

const unsigned long FREEFALL_MIN_MS = 120;
const unsigned long VERIFY_WINDOW   = 3000;

// ===== ML BUFFER (50 Hz sliding window) =====
#define ML_WIN    50     // 50 samples = 1s @ 50Hz (window ตรงกับ real-data model)
#define ML_STEP   25     // run inference ทุก 0.5s (overlap 50%)
#define ML_FS     50     // sampling rate

static float ml_ax[ML_WIN], ml_ay[ML_WIN], ml_az[ML_WIN];
static int   ml_idx            = 0;
static int   ml_samples_new    = 0;
static int   ml_total_samples  = 0;
static float ml_features[22];         // 22 features (real-data model)
static bool  ml_fall_flag      = false;

// ===== FLAGS =====
bool modeLocked      = false;
bool emergencyActive = false;
bool buzzerToggle    = false;
bool buzzer5sActive  = false;
bool resetJerkPrev   = false;  // set true เมื่อเปลี่ยนโหมด → processMPU() reset static prev

// ===== TIMING =====
unsigned long freefallStart    = 0;
unsigned long impactTime       = 0;
unsigned long lastMPURead      = 0;
unsigned long lastMLSample     = 0;   // NEW: 50 Hz ML sampling
unsigned long lastGPSUpdate    = 0;
unsigned long lastBlynkSync    = 0;
unsigned long lastModePress     = 0;
unsigned long lastSOSPress      = 0;
unsigned long buzzer5sStart    = 0;
unsigned long emergencyStart   = 0;
unsigned long lastOLEDTimer    = 0;   // OLED emergency timer (global to reset properly)
unsigned long lastOLEDStatus   = 0;   // OLED live status update during monitoring
float lastMag  = 9.8f;                // ค่าล่าสุดสำหรับ OLED display
float lastJerk = 0.0f;

const unsigned long DEBOUNCE_MS    = 300;
const unsigned long BTN_GUARD_MS   = 2000; // ignore BTN_EMERGENCY for 2s after loop() starts
unsigned long       loopStartTime  = 0;    // set in first loop() iteration
const unsigned long MPU_INTERVAL   = 200;  // 5 Hz  (threshold SM)
const unsigned long ML_INTERVAL    =  20;  // 50 Hz (ML buffer fill)
const unsigned long GPS_INTERVAL   = 2000;
const unsigned long BLYNK_INTERVAL = 5000;

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n========================================"));
  Serial.println(F("   ELDERLY SAFETY SYSTEM  v2.0"));
  Serial.println(F("   User-side Device"));
  Serial.println(F("========================================\n"));

  // GPIO
  pinMode(LED_GREEN,     OUTPUT);
  pinMode(LED_YELLOW,    OUTPUT);
  pinMode(LED_RED,       OUTPUT);
  pinMode(BUZZER,        OUTPUT);
  // Max GPIO drive strength = 40mA (vs default 20mA) → louder buzzer
  gpio_set_drive_capability((gpio_num_t)BUZZER, GPIO_DRIVE_CAP_3);
  pinMode(BTN_MODE,      INPUT_PULLUP);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  allLEDOff();
  digitalWrite(BUZZER, HIGH);  // active-LOW buzzer: HIGH = silent
  Serial.println(F("[OK] GPIO configured"));

  // ── DIAGNOSTIC: print button states immediately after pinMode ──────
  delay(50);  // let internal pull-ups settle
  Serial.print(F("[DIAG] BTN_MODE  (GPIO10) = "));
  Serial.println(digitalRead(BTN_MODE)      == HIGH ? F("HIGH (ok)") : F("LOW !! check wiring/short"));
  Serial.print(F("[DIAG] BTN_EMERG (GPIO13) = "));
  Serial.println(digitalRead(BTN_EMERGENCY) == HIGH ? F("HIGH (ok)") : F("LOW !! check wiring/short"));

  // EEPROM — โหลด mode ที่บันทึกไว้
  EEPROM.begin(EEPROM_SIZE);
  uint8_t saved = EEPROM.read(EEPROM_MODE_ADDR);
  currentMode = (saved <= 2) ? (Mode)saved : CHEST;
  Serial.print(F("[OK] EEPROM mode loaded: "));
  printMode(currentMode); Serial.println();

  // OLED — SSD1306 is write-only, use Adafruit begin() for init
  // 10kHz I2C clock for reliable operation on breadboard wires
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(10000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("[FAIL] OLED"));
  } else {
    oledOK = true;
    Serial.println(F("[OK] OLED initialized"));
    showBoot(F("BOOTING..."));
  }

  // MPU6050 — reset I2C bus after OLED init, then reinit at 10kHz
  Wire.end();
  delay(50);
  Wire.begin(OLED_SDA, OLED_SCL);
  Wire.setClock(10000);
  Wire.setTimeOut(30);  // 30ms timeout — prevents I2C bus hang (clone may clock-stretch)
  delay(50);

  // WHO_AM_I check — try 0x68 (AD0=GND) then 0x69 (AD0=VCC/float)
  uint8_t mpuAddr = 0x68;
  uint8_t whoami  = 0xFF;
  for (uint8_t addr : {0x68, 0x69}) {
    Wire.beginTransmission(addr);
    Wire.write(0x75);
    Wire.endTransmission(true);
    delay(5);
    Wire.requestFrom(addr, (uint8_t)1);
    uint8_t w = Wire.available() ? Wire.read() : 0xFF;
    if (w != 0xFF) { whoami = w; mpuAddr = addr; break; }
  }
  mpuI2CAddr = mpuAddr;  // save to global for mpuGetRaw()
  Serial.print(F("[..] MPU6050 WHO_AM_I = 0x")); Serial.print(whoami, HEX);
  Serial.print(F(" @ 0x")); Serial.println(mpuAddr, HEX);

  bool mpuOK = false;
  if (whoami == 0x68 || whoami == 0x69) {
    // genuine MPU6050 — use Adafruit library normally
    mpuOK = mpu.begin(mpuAddr);
    Wire.setClock(10000);
  } else if (whoami == 0x70) {
    // clone MPU6050 (WHO_AM_I=0x70) — bypass Adafruit, init manually
    mpuUseRaw = true;
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x6B); Wire.write(0x00);  // PWR_MGMT_1: wake up
    Wire.endTransmission(true); delay(100);
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x1C); Wire.write(0x10);  // ACCEL_CONFIG: try ±8g
    Wire.endTransmission(true); delay(5);
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x1B); Wire.write(0x08);  // GYRO_CONFIG: ±500°/s
    Wire.endTransmission(true);
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x1A); Wire.write(0x04);  // CONFIG: DLPF ~21Hz
    Wire.endTransmission(true);

    // ── อ่าน ACCEL_CONFIG กลับเพื่อ auto-detect scale จริง ──
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x1C);
    Wire.endTransmission(true);
    Wire.requestFrom(mpuAddr, (uint8_t)1);
    uint8_t acfg = Wire.available() ? Wire.read() : 0x10;
    uint8_t afs  = (acfg >> 3) & 0x03;  // bits 4:3
    // AFS_SEL: 0=±2g(16384), 1=±4g(8192), 2=±8g(4096), 3=±16g(2048)
    const float lsbTable[4] = {16384.0f, 8192.0f, 4096.0f, 2048.0f};
    mpuAccelScale = lsbTable[afs];
    Serial.print(F("[OK] MPU6050 clone (0x70) @ 0x")); Serial.println(mpuAddr, HEX);
    Serial.print(F("[OK] ACCEL_CONFIG=0x")); Serial.print(acfg, HEX);
    Serial.print(F(" AFS_SEL=")); Serial.print(afs);
    Serial.print(F(" scale=")); Serial.print(mpuAccelScale, 0);
    Serial.println(F(" LSB/g"));
    mpuOK = true;
  }
  if (!mpuOK) {
    Serial.println(F("[FAIL] MPU6050 - check wiring: VCC=3.3V GND SDA=8 SCL=9"));
    showBoot(F("MPU6050\nFAILED!\nCheck wires"));
    while (1) delay(100);
  }
  if (!mpuUseRaw) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  Serial.println(F("[OK] MPU6050 initialized (range: +-8g)"));

  // Self-calibrating scale: sample raw mag during 10s still period
  Serial.println(F("[..] Calibrating MPU6050 (10s) - keep STILL..."));
  showBoot(F("CALIBRATING\n  10 sec\n  keep still"));
  {
    float rawMagSum = 0; int rawMagCount = 0;
    for (int i = 0; i < 200; i++) {
      Wire.beginTransmission(mpuI2CAddr);
      Wire.write(0x3B);
      Wire.endTransmission(true);
      Wire.requestFrom(mpuI2CAddr, (uint8_t)6);
      if (Wire.available() >= 6) {
        int16_t rx = ((int16_t)Wire.read() << 8) | Wire.read();
        int16_t ry = ((int16_t)Wire.read() << 8) | Wire.read();
        int16_t rz = ((int16_t)Wire.read() << 8) | Wire.read();
        rawMagSum += sqrt((float)rx*rx + (float)ry*ry + (float)rz*rz);
        rawMagCount++;
      }
      delay(50);
    }
    if (rawMagCount > 10) {
      mpuAccelScale = rawMagSum / rawMagCount;
      Serial.print(F("[OK] Scale auto-calibrated: "));
      Serial.print(mpuAccelScale, 1);
      Serial.println(F(" LSB/g (raw counts per g)"));
    } else {
      Serial.println(F("[WARN] Calibration failed - using default 16384"));
    }
  }
  Serial.println(F("[OK] Calibration complete"));

  // GPS
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println(F("[OK] GPS initialized (RX=16)"));

  // WiFi + Blynk (non-blocking — 10s timeout, ระบบทำงานได้แม้ไม่มี WiFi)
  Serial.print(F("[..] WiFi connecting: ")); Serial.println(WIFI_SSID);
  showBoot(F("Connecting\nWiFi..."));
  WiFi.begin(WIFI_SSID, WIFI_PASS[0] ? WIFI_PASS : nullptr);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 10000) {
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("[OK] WiFi connected"));
    showBoot(F("WiFi OK\nBlynk..."));
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(10000);  // 10s — network บน campus อาจช้า
    if (Blynk.connected()) {
      Serial.println(F("[OK] Blynk connected"));
      Blynk.virtualWrite(V0, 0);
      Blynk.virtualWrite(V3, (int)currentMode);
      Blynk.virtualWrite(V4, 0);
    } else {
      Serial.println(F("[WARN] Blynk timeout - offline"));
    }
  } else {
    Serial.println(F("[WARN] WiFi timeout - offline"));
    showBoot(F("WiFi FAILED\nOffline mode"));
    delay(1000);
  }

  // Initial LED + OLED
  setLED(currentMode);
  displayMode(currentMode);

  // Startup ready beep (2 beeps)
  tone(BUZZER, BUZZER_FREQ, 120); delay(180); silenceBuzzer();
  delay(100);
  tone(BUZZER, BUZZER_FREQ, 120); delay(180); silenceBuzzer();

  Serial.println(F("\n=== SYSTEM READY — MONITORING ==="));
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  // Record the moment loop() first runs (guard is relative to this, not power-on)
  if (loopStartTime == 0) loopStartTime = millis();

  if (Blynk.connected()) Blynk.run();

#ifndef DISABLE_AUTO_FALL
  if (sysState == SYS_MONITOR) {
    if (millis() - lastMLSample >= ML_INTERVAL) {
      lastMLSample = millis();
      sampleForML();
    }
    if (!emergencyActive && millis() - lastMPURead >= MPU_INTERVAL) {
      lastMPURead = millis();
      processMPU();
    }
  }
#endif  // DISABLE_AUTO_FALL

  // ── GPS ────────────────────────────────────────────────────
  while (GPSSerial.available()) gps.encode(GPSSerial.read());
  if (millis() - lastGPSUpdate >= GPS_INTERVAL) {
    lastGPSUpdate = millis();
    readGPS();
  }

  // ── Blynk heartbeat sync ───────────────────────────────────
  if (millis() - lastBlynkSync >= BLYNK_INTERVAL) {
    lastBlynkSync = millis();
    if (Blynk.connected()) {
      Blynk.virtualWrite(V0, emergencyActive ? 2 : 0);
      Blynk.virtualWrite(V1, gpsLat);
      Blynk.virtualWrite(V2, gpsLon);
      Blynk.virtualWrite(V3, (int)currentMode);
    }
    Serial.print(F("[OK] sync emergency="));
    Serial.print(emergencyActive);
    Serial.println(Blynk.connected() ? F(" [Blynk OK]") : F(" [offline]"));
  }

  // ── Buzzer 5 วิ (เมื่อเริ่ม emergency) ─────────────────────
  if (buzzer5sActive) {
    static unsigned long lastBeep = 0;
    if (millis() - buzzer5sStart < 5000) {
      if (millis() - lastBeep > 300) {
        lastBeep = millis();
        tone(BUZZER, BUZZER_FREQ, 150);
      }
    } else {
      buzzer5sActive = false;
      silenceBuzzer();
    }
  }

  // ── Emergency buzzer (toggle ด้วย BTN_EMERGENCY) ──────────
  if (emergencyActive && buzzerToggle && !buzzer5sActive) {
    static unsigned long lastBuzz = 0;
    static bool buzzState = false;
    if (millis() - lastBuzz > 200) {
      lastBuzz = millis();
      buzzState = !buzzState;
      if (buzzState) { tone(BUZZER, BUZZER_FREQ, 150); digitalWrite(LED_RED, HIGH); }
      else           { silenceBuzzer(); digitalWrite(LED_RED, LOW); }
    }
  }

  // ── Emergency timer บน OLED ────────────────────────────────
  if (emergencyActive) {
    if (millis() - lastOLEDTimer >= 1000) {
      lastOLEDTimer = millis();
      displayEmergencyTimer();
    }
  }

  // ── OLED live status (MONITOR, ไม่ emergency) ───────────────
  if (sysState == SYS_MONITOR && !emergencyActive) {
    if (millis() - lastOLEDStatus >= 2000) {
      lastOLEDStatus = millis();
      displayLiveStatus(lastMag, lastJerk);
    }
  }

  // ── ปุ่ม MODE (GPIO10) — falling edge only ───────────────
  {
    static bool modeWasLow = false;
    bool modeLow = (digitalRead(BTN_MODE) == LOW);
    bool modeFall = modeLow && !modeWasLow;
    modeWasLow = modeLow;
    if (modeFall && millis() - lastModePress > DEBOUNCE_MS) {
      lastModePress = millis();
      if (emergencyActive) {
        // ขณะ emergency: clear
        Serial.println(F("[BTN] MODE -> clear emergency"));
        clearEmergency();
      } else {
        // ทั้ง SETUP และ MONITOR: BTN10 = เปลี่ยนโหมดเสมอ
        currentMode = (Mode)((currentMode + 1) % 3);
        saveMode(currentMode);
        if (Blynk.connected()) Blynk.virtualWrite(V3, (int)currentMode);
        resetFallState();  // reset jerk/fall state เพื่อป้องกัน false trigger จากการย้ายตำแหน่ง
        tone(BUZZER, 3200, 80); delay(100); silenceBuzzer();
        setLED(currentMode);
        if (sysState == SYS_SETUP) displaySetup(currentMode);
        else                        displayMode(currentMode);
        Serial.print(F("[MODE] ")); printMode(currentMode); Serial.println();
      }
    }
  }

  // ── ปุ่ม EMERGENCY (GPIO13) — falling edge only ──────────
  {
    static bool sosWasLow = false;
    bool sosLow  = (digitalRead(BTN_EMERGENCY) == LOW);
    bool sosFall = sosLow && !sosWasLow;
    bool guard   = (millis() - loopStartTime > BTN_GUARD_MS);
    sosWasLow = sosLow;
    if (sosFall && guard && millis() - lastSOSPress > DEBOUNCE_MS) {
      lastSOSPress = millis();
      if (sysState == SYS_SETUP) {
        // SETUP: confirm โหมด → เข้า MONITOR
        sysState = SYS_MONITOR;
        if (Blynk.connected()) Blynk.virtualWrite(V3, (int)currentMode);
        // beep สองครั้ง = เริ่ม monitor
        tone(BUZZER, 2200, 180); delay(250); silenceBuzzer();
        delay(80);
        tone(BUZZER, 2800, 180); delay(250); silenceBuzzer();
        setLED(currentMode);
        displayMode(currentMode);
        Serial.print(F("[START] Monitoring — mode: ")); printMode(currentMode); Serial.println();
      } else {
        // MONITOR: SOS / clear
        if (emergencyActive) {
          Serial.println(F("[BTN] SOS -> clear emergency"));
          clearEmergency();
        } else {
          Serial.println(F("[BTN] SOS -> trigger"));
          triggerEmergency("MANUAL SOS");
        }
      }
    }
  }

  delay(10);
}

// ============================================================
//  ML — SAMPLE BUFFER @ 50Hz
// ============================================================
void sampleForML() {
  sensors_event_t a, g, temp;
  getMPUEvent(&a, &g, &temp);

  ml_ax[ml_idx] = a.acceleration.x;
  ml_ay[ml_idx] = a.acceleration.y;
  ml_az[ml_idx] = a.acceleration.z;
  ml_idx = (ml_idx + 1) % ML_WIN;
  ml_samples_new++;
  ml_total_samples++;

  // Run inference every ML_STEP new samples (= every 1 s)
  // Guard: wait until buffer is fully filled (ML_WIN samples) to avoid
  // false positives from zero-padded startup window
  if (ml_samples_new >= ML_STEP && ml_total_samples >= ML_WIN) {
    ml_samples_new = 0;
    // Build a contiguous window from the circular buffer
    float wx[ML_WIN], wy[ML_WIN], wz[ML_WIN];
    for (int i = 0; i < ML_WIN; i++) {
      int j = (ml_idx + i) % ML_WIN;
      wx[i] = ml_ax[j]; wy[i] = ml_ay[j]; wz[i] = ml_az[j];
    }
    extract_features_ml(wx, wy, wz, ML_WIN, ml_features);
    int pred = mlPredict(ml_features);
    // pred: 0=NORMAL  1=FALLING  2=FALLEN
    // FALLING (pred==1) F1 ต่ำ → false positive เยอะ → ไม่ใช้
    // FALLEN (pred==2) ต้องเจอ 2 รอบติดกัน ถึงยิง (consecutive filter)
    static int ml_fallen_count = 0;
    if (pred == 2) {
      ml_fallen_count++;
      ml_fall_flag = (ml_fallen_count >= 2);
    } else {
      ml_fallen_count = 0;
      ml_fall_flag = false;
    }
    if (pred > 0) {
      Serial.print(F("[ML] pred="));
      Serial.print(pred == 1 ? F("FALLING") : F("FALLEN"));
      if (pred == 2) { Serial.print(F(" x")); Serial.print(ml_fallen_count); }
      Serial.println();
    }
  }
}

// ============================================================
//  ML — FEATURE EXTRACTION (22 features, matches RealData_Model.ipynb)
//  feat[0..2]  = ax/ay/az mean
//  feat[3..5]  = ax/ay/az std
//  feat[6..10] = mag mean/std/max/min/iqr
//  feat[11..13]= jerk mean/max/std
//  feat[14]    = energy, feat[15] = peak2peak
//  feat[16..17]= corr_xz / corr_yz
//  feat[18]    = mag skewness
//  feat[19]    = frac samples >19.6 m/s2 (>2g impact)
//  feat[20]    = frac samples <4.9  m/s2 (<0.5g freefall)
//  feat[21]    = jerk energy
// ============================================================
void extract_features_ml(float* ax, float* ay, float* az, int n, float* out) {
  float mag[ML_WIN];
  float sum_ax=0, sum_ay=0, sum_az=0, sum_mag=0;
  for (int i=0;i<n;i++){
    mag[i]=sqrtf(ax[i]*ax[i]+ay[i]*ay[i]+az[i]*az[i]);
    sum_ax+=ax[i]; sum_ay+=ay[i]; sum_az+=az[i]; sum_mag+=mag[i];
  }
  float mean_ax=sum_ax/n, mean_ay=sum_ay/n, mean_az=sum_az/n;
  float mag_mean=sum_mag/n;

  float var_ax=0,var_ay=0,var_az=0,var_mag=0;
  float min_mag=mag[0],max_mag=mag[0];
  int   cnt_high=0, cnt_low=0;
  for (int i=0;i<n;i++){
    var_ax +=(ax[i]-mean_ax)*(ax[i]-mean_ax);
    var_ay +=(ay[i]-mean_ay)*(ay[i]-mean_ay);
    var_az +=(az[i]-mean_az)*(az[i]-mean_az);
    var_mag+=(mag[i]-mag_mean)*(mag[i]-mag_mean);
    if(mag[i]<min_mag)min_mag=mag[i];
    if(mag[i]>max_mag)max_mag=mag[i];
    if(mag[i]>19.6f)cnt_high++;
    if(mag[i]<4.9f) cnt_low++;
  }
  float std_ax =sqrtf(var_ax/n),  std_ay =sqrtf(var_ay/n),  std_az =sqrtf(var_az/n);
  float std_mag=sqrtf(var_mag/n)+1e-9f;

  // IQR of mag (sort-free approximation via mean±std)
  float q75=mag_mean+0.675f*std_mag, q25=mag_mean-0.675f*std_mag;
  float mag_iqr=q75-q25;

  // Skewness
  float skew=0;
  for(int i=0;i<n;i++){float d=(mag[i]-mag_mean)/std_mag; skew+=d*d*d;}
  skew/=n;

  // Jerk
  float jerk_sum=0,jerk_max=0,jerk_sq=0;
  float var_jerk=0; float jerk_vals[ML_WIN-1];
  for(int i=1;i<n;i++){
    float dx=ax[i]-ax[i-1], dy=ay[i]-ay[i-1], dz=az[i]-az[i-1];
    float jm=sqrtf(dx*dx+dy*dy+dz*dz);
    jerk_vals[i-1]=jm;
    jerk_sum+=jm; if(jm>jerk_max)jerk_max=jm; jerk_sq+=jm*jm;
  }
  int nj=n-1;
  float jerk_mean=jerk_sum/nj;
  float jerk_e=jerk_sq/nj;
  float var_j=0; for(int i=0;i<nj;i++) var_j+=(jerk_vals[i]-jerk_mean)*(jerk_vals[i]-jerk_mean);
  float jerk_std=sqrtf(var_j/nj);

  // Energy
  float energy=0;
  for(int i=0;i<n;i++) energy+=ax[i]*ax[i]+ay[i]*ay[i]+az[i]*az[i];
  energy/=n;

  // Correlations xz and yz
  float cxz=0,cyz=0;
  for(int i=0;i<n;i++){
    cxz+=(ax[i]-mean_ax)*(az[i]-mean_az);
    cyz+=(ay[i]-mean_ay)*(az[i]-mean_az);
  }
  float dxz=std_ax*std_az*n+1e-9f;
  float dyz=std_ay*std_az*n+1e-9f;

  // Pack 22 features
  int k=0;
  out[k++]=mean_ax; out[k++]=mean_ay; out[k++]=mean_az;
  out[k++]=std_ax;  out[k++]=std_ay;  out[k++]=std_az;
  out[k++]=mag_mean; out[k++]=std_mag; out[k++]=max_mag; out[k++]=min_mag; out[k++]=mag_iqr;
  out[k++]=jerk_mean; out[k++]=jerk_max; out[k++]=jerk_std;
  out[k++]=energy;
  out[k++]=max_mag-min_mag;
  out[k++]=cxz/dxz; out[k++]=cyz/dyz;
  out[k++]=skew;
  out[k++]=(float)cnt_high/n;
  out[k++]=(float)cnt_low/n;
  out[k++]=jerk_e;  // k=22
}

// ============================================================
//  ML — PREDICT (mode-aware, returns 0=NORMAL 1=FALLING 2=FALLEN)
// ============================================================
int mlPredict(float* features) {
  switch (currentMode) {
    case CHEST: return chestModel.predict(features);
    case SHIRT: return shirtModel.predict(features);
    case PANTS: return pantsModel.predict(features);
    default:    return 0;
  }
}

// ============================================================
//  FALL DETECTION STATE MACHINE
// ============================================================
void processMPU() {
  sensors_event_t a, g, temp;
  getMPUEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float acc_mag = sqrtf(ax*ax + ay*ay + az*az);

  // ── Jerk: ความเร็วการเปลี่ยนแปลง (|Δacc| ระหว่าง 2 sample ห่างกัน 200ms) ──
  static float prev_ax = 0, prev_ay = 0, prev_az = 9.8f;
  if (resetJerkPrev) { prev_ax = ax; prev_ay = ay; prev_az = az; resetJerkPrev = false; }
  float dax = ax - prev_ax, day = ay - prev_ay, daz = az - prev_az;
  float jerk = sqrtf(dax*dax + day*day + daz*daz);
  prev_ax = ax; prev_ay = ay; prev_az = az;
  lastMag = acc_mag; lastJerk = jerk;  // update global for OLED

  switch (fallState) {

    case FALL_IDLE: {
      // Debug: print ทุก 1 วิ
      static unsigned long lastPrint = 0;
      if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        Serial.print(F("[MPU] mag=")); Serial.print(acc_mag, 2);
        Serial.print(F(" jerk="));    Serial.println(jerk, 2);
      }

      // Path A: freefall (ตกอิสระ → ช้าๆ)
      if (acc_mag < FREEFALL_THRESHOLD) {
        fallState     = FALL_FREEFALL;
        freefallStart = millis();
        Serial.print(F("[!] Freefall mag=")); Serial.println(acc_mag, 2);
      }
      // Path B: jerk สูงมาก = พลิกตัวฉับพลัน/สะดุด โดยไม่มี freefall ชัด
      // ต้องมีทั้ง jerk สูง + acc_mag สูงพอ (ไม่ใช่แค่ noise)
      else if (jerk > JERK_THRESHOLD && acc_mag > IMPACT_THRESHOLD * 0.7f) {
        fallState  = FALL_VERIFY;
        impactTime = millis();
        if (Blynk.connected()) Blynk.virtualWrite(V0, 1);
        Serial.print(F("[!] High-jerk fall! jerk=")); Serial.print(jerk, 1);
        Serial.print(F(" mag=")); Serial.println(acc_mag, 1);
      }
      break;
    }

    case FALL_FREEFALL:
      if (acc_mag > IMPACT_THRESHOLD) {
        unsigned long ffDuration = millis() - freefallStart;
        if (ffDuration >= FREEFALL_MIN_MS) {
          // freefall นานพอ → impact จริง
          fallState  = FALL_VERIFY;   // ข้าม FALL_IMPACT (ไม่มีประโยชน์)
          impactTime = millis();
          if (Blynk.connected()) Blynk.virtualWrite(V0, 1);  // WARNING
          Serial.print(F("[!] Impact! freefall="));
          Serial.print(ffDuration);
          Serial.println(F("ms -> verifying 3s..."));
        } else {
          // freefall สั้นเกิน → vibration/tap — กรองออก
          Serial.print(F("[!] Brief freefall filtered ("));
          Serial.print(ffDuration);
          Serial.println(F("ms < 120ms)"));
          fallState = FALL_IDLE;
        }
      } else if (millis() - freefallStart > 2000) {
        fallState = FALL_IDLE;  // freefall นานเกิน 2s → false positive
      }
      break;

    case FALL_IMPACT:
      fallState = FALL_VERIFY;  // legacy path (ไม่ควรเข้ามาแล้ว)
      break;

    case FALL_VERIFY:
      if (millis() - impactTime >= VERIFY_WINDOW) {
        // ตรวจว่าคนยังนิ่ง (ไม่ลุก): acc_mag อยู่ในช่วงแรงโน้มถ่วงปกติ
        // ไม่ดูแค่ az เพื่อรองรับทุก orientation (ล้มข้าง/หลัง/หน้า)
        bool stillOnGround = (acc_mag >= STILL_MAG_MIN && acc_mag <= STILL_MAG_MAX);
        bool mlFall        = ml_fall_flag;

        Serial.print(F("[VERIFY] acc_mag="));   Serial.print(acc_mag, 2);
        Serial.print(F(" still="));             Serial.print(stillOnGround);
        Serial.print(F(" ML="));               Serial.println(mlFall);

        if (stillOnGround && mlFall) {
          // ทั้ง threshold และ ML ยืนยัน → FALL แน่นอน
          Serial.println(F("FALL CONFIRMED (threshold+ML)"));
          fallState = FALL_EMERGENCY;
          triggerEmergency("FALL CONFIRMED");
        } else if (mlFall) {
          // ML เดียวยืนยัน → trigger (ML ถูก train มาสำหรับกรณีนี้)
          Serial.println(F("FALL CONFIRMED (ML only)"));
          fallState = FALL_EMERGENCY;
          triggerEmergency("ML FALL");
        } else if (stillOnGround) {
          // threshold เดียว ไม่มี ML → Near Fall warning เท่านั้น
          Serial.println(F("Near Fall — threshold only, no ML confirm"));
          tone(BUZZER, BUZZER_FREQ, 400); silenceBuzzer();
          if (Blynk.connected()) Blynk.virtualWrite(V0, 1);
          fallState    = FALL_IDLE;
          ml_fall_flag = false;
        } else {
          // ลุกขึ้นได้ทัน → recovered
          Serial.println(F("Recovered — no fall"));
          fallState    = FALL_IDLE;
          ml_fall_flag = false;
          if (Blynk.connected()) Blynk.virtualWrite(V0, 0);
        }
      }
      break;

    case FALL_EMERGENCY:
      break;  // รอ acknowledge
  }
}

// ============================================================
//  NOTIFY CAREGIVER — HTTP GET โดยตรง (ไม่ต้องใช้ Blynk Automation)
//  เรียกครั้งเดียวตอน emergency เกิด/หาย
// ============================================================
void notifyCaregiverHTTP(int val) {
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  String url = "https://blynk.cloud/external/api/update?token=";
  url += CAREGIVER_TOKEN;
  url += "&V4=";
  url += val;
  http.begin(url);
  http.setTimeout(3000);
  int code = http.GET();
  http.end();
  Serial.print(F("[HTTP] Caregiver V4=")); Serial.print(val);
  Serial.print(F(" -> ")); Serial.println(code);
}

// ============================================================
//  TRIGGER EMERGENCY
// ============================================================
void triggerEmergency(const char* reason) {
  if (emergencyActive) return;  // prevent double-trigger
  emergencyActive = true;
  modeLocked      = true;
  buzzerToggle    = true;
  buzzer5sActive  = true;
  buzzer5sStart   = millis();
  emergencyStart  = millis();
  lastOLEDTimer   = 0;      // reset OLED timer so it updates immediately
  ml_fall_flag    = false;  // reset so ML doesn't re-fire

  Serial.println(F("\n[!] ================================"));
  Serial.print(F("   EMERGENCY: ")); Serial.println(reason);
  Serial.print(F("   Mode: ")); printMode(currentMode); Serial.println();
  Serial.print(F("   GPS : ")); Serial.print(gpsLat, 6);
  Serial.print(F(", "));        Serial.println(gpsLon, 6);
  Serial.println(F("[!] ================================\n"));

  // ส่ง Blynk (User device)
  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, 2);
    Blynk.virtualWrite(V1, gpsLat);
    Blynk.virtualWrite(V2, gpsLon);
    Blynk.virtualWrite(V3, (int)currentMode);
    Blynk.virtualWrite(V4, 1);
  }
  // ส่งตรงไปหา Caregiver device ผ่าน HTTP (ไม่ต้องใช้ Blynk Automation)
  notifyCaregiverHTTP(1);

  // LED ทุกดวงติด
  digitalWrite(LED_GREEN,  HIGH);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_RED,    HIGH);

  displayFall(reason);
}

// ============================================================
//  RESET FALL STATE — เรียกเมื่อเปลี่ยนโหมด เพื่อป้องกัน jerk false-trigger
// ============================================================
void resetFallState() {
  fallState    = FALL_IDLE;
  ml_fall_flag = false;
  lastMag      = 9.8f;
  lastJerk     = 0.0f;
  resetJerkPrev = true;  // processMPU() จะ reset static prev บน next call
}

// ============================================================
//  CLEAR EMERGENCY  (กด BTN_MODE ขณะ emergency active)
// ============================================================
void clearEmergency() {
  emergencyActive = false;
  modeLocked      = false;
  buzzerToggle    = false;
  buzzer5sActive  = false;
  fallState       = FALL_IDLE;
  ml_fall_flag    = false;
  silenceBuzzer();
  allLEDOff();
  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, 0);
    Blynk.virtualWrite(V4, 0);
  }
  notifyCaregiverHTTP(0);  // แจ้ง Caregiver ว่า emergency หายแล้ว
  setLED(currentMode);
  displayMode(currentMode);
  Serial.println(F("[OK] Emergency cleared by BTN_MODE"));
}

// ============================================================
//  BUZZER SILENCE HELPER
//  noTone() บน ESP32 อาจไม่คืน pin ให้ GPIO matrix → ต้อง pinMode อีกครั้ง
// ============================================================
void silenceBuzzer() {
  noTone(BUZZER);
  // gpio_reset_pin releases LEDC — re-configure pin as output LOW
  gpio_reset_pin((gpio_num_t)BUZZER);
  gpio_set_direction((gpio_num_t)BUZZER, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)BUZZER, 1);  // HIGH = silent for active-LOW buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, HIGH);  // active-LOW: HIGH = silent
  // Restore 40mA drive (gpio_reset_pin resets it back to default 20mA)
  gpio_set_drive_capability((gpio_num_t)BUZZER, GPIO_DRIVE_CAP_3);
}

// ============================================================
//  GPS
// ============================================================
void readGPS() {
  float lat, lon;
  unsigned long age;
  gps.f_get_position(&lat, &lon, &age);
  if (age != TinyGPS::GPS_INVALID_AGE && age < 5000) {
    gpsLat = lat;
    gpsLon = lon;
    Serial.print(F("GPS: ")); Serial.print(lat, 6);
    Serial.print(F(", "));    Serial.println(lon, 6);
  } else {
    Serial.println(F("GPS: searching for satellites..."));
  }
}

// ============================================================
//  EEPROM
// ============================================================
void saveMode(Mode mode) {
  EEPROM.write(EEPROM_MODE_ADDR, (uint8_t)mode);
  EEPROM.commit();
}

// ============================================================
//  LED
// ============================================================
void setLED(Mode mode) {
  allLEDOff();
  switch (mode) {
    case CHEST: digitalWrite(LED_GREEN,  HIGH); break;
    case SHIRT: digitalWrite(LED_YELLOW, HIGH); break;
    case PANTS: digitalWrite(LED_RED,    HIGH); break;
  }
}

void allLEDOff() {
  digitalWrite(LED_GREEN,  LOW);
  digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_RED,    LOW);
}

// ============================================================
//  OLED DISPLAY
// ============================================================
void showBoot(const __FlashStringHelper* msg) {
  if (!oledOK) return;  // skip if OLED not connected
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("ELDERLY SAFETY v2.0"));
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 16);
  display.println(msg);
  display.display();
}

// แสดง SETUP screen — กด 10 เลือก, กด 13 confirm
void displaySetup(Mode mode) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("-- SELECT MODE --"));
  display.setTextSize(2);
  display.setCursor(0, 14);
  switch (mode) {
    case CHEST: display.println(F("CHEST\nCLIP"));   break;
    case SHIRT: display.println(F("SHIRT\nPOCKET")); break;
    case PANTS: display.println(F("PANTS\nPOCKET")); break;
  }
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.println(F("BTN10=next  BTN13=ok"));
  display.display();
}

// แสดง MONITOR screen — ระบบกำลัง detect อยู่
void displayMode(Mode mode) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  switch (mode) {
    case CHEST: display.println(F("CHEST\nCLIP"));   break;
    case SHIRT: display.println(F("SHIRT\nPOCKET")); break;
    case PANTS: display.println(F("PANTS\nPOCKET")); break;
  }
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.println(F("MONITORING ACTIVE"));
  display.display();
}

// แสดงสถานะ live ระหว่าง MONITOR — อัปเดตทุก 2 วิ
// แสดง: mode / fallState / mag / jerk (บรรทัดล่าง)
void displayLiveStatus(float mag, float jerk) {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // บรรทัด 0: mode
  display.setCursor(0, 0);
  switch (currentMode) {
    case CHEST: display.print(F("CHEST CLIP"));   break;
    case SHIRT: display.print(F("SHIRT POCKET")); break;
    case PANTS: display.print(F("PANTS POCKET")); break;
  }

  // บรรทัด 1: สถานะ fall state machine
  display.setCursor(0, 12);
  switch (fallState) {
    case FALL_IDLE:      display.print(F("[ NORMAL ]"));    break;
    case FALL_FREEFALL:  display.print(F("[ FREEFALL ]"));  break;
    case FALL_VERIFY:    display.print(F("[ VERIFYING ]")); break;
    case FALL_EMERGENCY: display.print(F("[ FALL !! ]"));   break;
    default:             display.print(F("[ ... ]"));       break;
  }

  // divider
  display.drawLine(0, 23, 127, 23, SSD1306_WHITE);

  // บรรทัด 2: mag
  display.setCursor(0, 27);
  display.print(F("mag  "));
  display.print(mag, 2);
  display.print(F(" m/s2"));

  // บรรทัด 3: jerk
  display.setCursor(0, 39);
  display.print(F("jerk "));
  display.print(jerk, 2);
  display.print(F(" m/s2"));

  // บรรทัด 4: hint
  display.setCursor(0, 55);
  display.print(F("13=SOS  10=mode"));

  display.display();
}

void displayFall(const char* reason) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("!! FALL !!"));
  display.setTextSize(1);
  display.setCursor(0, 20);
  display.println(reason);
  display.println(F("Alert sent via"));
  display.println(F("Blynk + GPS"));
  display.display();
}

void displayEmergencyTimer() {
  unsigned long elapsed = (millis() - emergencyStart) / 1000;
  int minutes = elapsed / 60;
  int seconds = elapsed % 60;

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("SOS ACTIVE"));
  display.setTextSize(3);
  display.setCursor(0, 24);
  if (minutes < 10) display.print('0');
  display.print(minutes);
  display.print(':');
  if (seconds < 10) display.print('0');
  display.println(seconds);
  display.setTextSize(1);
  display.setCursor(0, 56);
  printModeOLED(currentMode);
  display.display();
}

void printModeOLED(Mode mode) {
  switch (mode) {
    case CHEST: display.print(F("CHEST")); break;
    case SHIRT: display.print(F("SHIRT")); break;
    case PANTS: display.print(F("PANTS")); break;
  }
}

void printMode(Mode mode) {
  switch (mode) {
    case CHEST: Serial.print(F("CHEST CLIP"));   break;
    case SHIRT: Serial.print(F("SHIRT POCKET")); break;
    case PANTS: Serial.print(F("PANTS POCKET")); break;
  }
}
