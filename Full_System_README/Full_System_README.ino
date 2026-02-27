/*
 * =============================================
 * ELDERLY SAFETY SYSTEM  —  USER-SIDE DEVICE
 * =============================================
 * สวมใส่บนผู้สูงอายุ (Chest / Shirt / Pants)
 *
 * Hardware:
 *   ESP32-S3
 *   MPU6050  (I2C: SDA=37, SCL=38)  ← shared bus with OLED
 *   OLED SSD1306 128×64 (I2C 0x3C)
 *   GPS Module (UART RX=16, read-only)
 *   LED Green=4  Yellow=5  Red=6
 *   Buzzer GPIO 7
 *   BTN_MODE=9  BTN_EMERGENCY=10
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
#include <BlynkSimpleEsp32.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TinyGPS.h>
#include <EEPROM.h>
#include <arduinoFFT.h>      // ArduinoFFT by Enrique Condes

// ML model headers — สร้างจาก ML/ESP32_export.ipynb แล้ว copy มาไว้ใน sketch folder
#include "CHEST_model.h"
#include "SHIRT_model.h"
#include "PANTS_model.h"

// WiFi credentials มาจาก config.h (WIFI_SSID / WIFI_PASS)

// ===== PIN DEFINITIONS =====
#define LED_GREEN      4    // LED เขียว
#define LED_YELLOW     5    // LED เหลือง
#define LED_RED        6    // LED แดง

#define BTN_MODE       9    // ปุ่มสลับ mode
#define BTN_EMERGENCY  10   // ปุ่ม SOS ฉุกเฉิน

#define BUZZER         7    // Buzzer

#define OLED_SDA       37   // I2C SDA (shared: OLED 0x3C + MPU6050 0x68)
#define OLED_SCL       38   // I2C SCL (shared: OLED 0x3C + MPU6050 0x68)

// MPU6050: ยังไม่ได้ติด → ต่อสาย SDA→GPIO37, SCL→GPIO38, VCC→3.3V, GND→GND, AD0→GND
// (AD0→GND ทำให้ I2C address = 0x68 ไม่ชนกับ OLED ที่ 0x3C)

#define GPS_RX         16   // ESP32 RX ← GPS module TX  (รับ NMEA จาก GPS)
#define GPS_TX         17   // ESP32 TX → GPS module RX  (ส่งคำสั่งไป GPS ถ้าต้องการ)

// ===== OLED =====
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== MPU6050 =====
Adafruit_MPU6050 mpu;

// ===== GPS =====
TinyGPS gps;
HardwareSerial GPSSerial(2);
float gpsLat = 0.0f, gpsLon = 0.0f;

// ===== EEPROM =====
#define EEPROM_SIZE      1
#define EEPROM_MODE_ADDR 0

// ===== MODE =====
enum Mode { CHEST = 0, SHIRT = 1, PANTS = 2 };
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

// Fall detection thresholds (หน่วย m/s²)
const float FREEFALL_THRESHOLD  = 3.0f;   // ต่ำกว่านี้ = กำลังตกอิสระ
const float IMPACT_THRESHOLD    = 20.0f;  // สูงกว่านี้ = กระแทกพื้น
const float LYING_THRESHOLD     = 5.0f;   // az ต่ำกว่านี้ = นอนราบ
const unsigned long VERIFY_WINDOW = 3000; // ms — รอยืนยัน 3 วิ

// ===== ML BUFFER (50 Hz sliding window) =====
#define ML_WIN   100     // 100 samples = 2 s @ 50 Hz  (CHEST/SHIRT)
#define ML_WIN_P  80     // 80 samples = 2 s @ 40 Hz   (PANTS proxy)
#define ML_STEP   50     // run inference every 50 new samples
#define ML_FS     50     // sampling rate for ML feature extraction

static float ml_ax[ML_WIN], ml_ay[ML_WIN], ml_az[ML_WIN];
static int   ml_idx            = 0;   // circular buffer write position
static int   ml_samples_new    = 0;   // samples collected since last inference
static float ml_features[26];         // output of extract_features_ml()
static bool  ml_fall_flag      = false; // set true when ML predicts FALL

// ===== FLAGS =====
bool modeLocked      = false;
bool emergencyActive = false;
bool buzzerToggle    = false;
bool buzzer5sActive  = false;

// ===== TIMING =====
unsigned long freefallStart    = 0;
unsigned long impactTime       = 0;
unsigned long lastMPURead      = 0;
unsigned long lastMLSample     = 0;   // NEW: 50 Hz ML sampling
unsigned long lastGPSUpdate    = 0;
unsigned long lastBlynkSync    = 0;
unsigned long lastButtonPress  = 0;
unsigned long buzzer5sStart    = 0;
unsigned long emergencyStart   = 0;

const unsigned long DEBOUNCE_MS    = 300;
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
  pinMode(BTN_MODE,      INPUT_PULLUP);
  pinMode(BTN_EMERGENCY, INPUT_PULLUP);
  allLEDOff();
  digitalWrite(BUZZER, LOW);
  Serial.println(F("✓ GPIO configured"));

  // EEPROM — โหลด mode ที่บันทึกไว้
  EEPROM.begin(EEPROM_SIZE);
  uint8_t saved = EEPROM.read(EEPROM_MODE_ADDR);
  currentMode = (saved <= 2) ? (Mode)saved : CHEST;
  Serial.print(F("✓ EEPROM mode loaded: "));
  printMode(currentMode); Serial.println();

  // OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("✗ OLED FAILED — check SDA:37 SCL:38"));
  } else {
    Serial.println(F("✓ OLED initialized"));
    showBoot(F("BOOTING..."));
  }

  // MPU6050 — แชร์ I2C bus กับ OLED (address 0x68)
  if (!mpu.begin()) {
    Serial.println(F("✗ MPU6050 FAILED — check SDA:37 SCL:38, AD0→GND"));
    showBoot(F("MPU6050\nFAILED!"));
    while (1) delay(100);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println(F("✓ MPU6050 initialized (range: ±8g)"));

  // Calibration 10 วิ — วางอุปกรณ์นิ่งๆ
  Serial.println(F("⏳ Calibrating MPU6050 (10s) — keep device STILL..."));
  showBoot(F("CALIBRATING\n  10 sec\n  keep still"));
  delay(10000);
  Serial.println(F("✓ Calibration complete"));

  // GPS
  GPSSerial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  Serial.println(F("✓ GPS initialized (RX=GPIO16, read-only)"));

  // WiFi + Blynk
  Serial.print(F("⏳ WiFi connecting: ")); Serial.println(WIFI_SSID);
  showBoot(F("Connecting\nWiFi+Blynk..."));
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS[0] ? WIFI_PASS : nullptr);
  Serial.println(F("✓ Blynk connected"));

  // Initial Blynk state
  Blynk.virtualWrite(V0, 0);
  Blynk.virtualWrite(V3, (int)currentMode);
  Blynk.virtualWrite(V4, 0);

  // Initial LED + OLED
  setLED(currentMode);
  displayMode(currentMode);

  // Welcome beep
  tone(BUZZER, 2000, 80); delay(110); noTone(BUZZER);
  tone(BUZZER, 2500, 80); delay(110); noTone(BUZZER);

  Serial.println(F("\n=== SYSTEM READY — MONITORING ==="));
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  Blynk.run();

  // ── ML buffer fill @ 50Hz (ทำงานเสมอ รวมถึงระหว่าง emergency ด้วย) ──
  if (millis() - lastMLSample >= ML_INTERVAL) {
    lastMLSample = millis();
    sampleForML();
  }

  // ── Fall detection @ 5Hz (ไม่ทำงานขณะ emergency) ──────────
  if (!emergencyActive && millis() - lastMPURead >= MPU_INTERVAL) {
    lastMPURead = millis();
    processMPU();
  }

  // ── GPS ────────────────────────────────────────────────────
  while (GPSSerial.available()) gps.encode(GPSSerial.read());
  if (millis() - lastGPSUpdate >= GPS_INTERVAL) {
    lastGPSUpdate = millis();
    readGPS();
  }

  // ── Blynk heartbeat sync ───────────────────────────────────
  if (millis() - lastBlynkSync >= BLYNK_INTERVAL) {
    lastBlynkSync = millis();
    Blynk.virtualWrite(V0, emergencyActive ? 2 : 0);
    Blynk.virtualWrite(V1, gpsLat);
    Blynk.virtualWrite(V2, gpsLon);
    Blynk.virtualWrite(V3, (int)currentMode);
  }

  // ── Buzzer 5 วิ (เมื่อเริ่ม emergency) ─────────────────────
  if (buzzer5sActive) {
    static unsigned long lastBeep = 0;
    if (millis() - buzzer5sStart < 5000) {
      if (millis() - lastBeep > 300) {
        lastBeep = millis();
        tone(BUZZER, 2500, 150);
      }
    } else {
      buzzer5sActive = false;
      noTone(BUZZER);
    }
  }

  // ── Emergency buzzer (toggle ด้วย BTN_EMERGENCY) ──────────
  if (emergencyActive && buzzerToggle && !buzzer5sActive) {
    static unsigned long lastBuzz = 0;
    static bool buzzState = false;
    if (millis() - lastBuzz > 200) {
      lastBuzz = millis();
      buzzState = !buzzState;
      if (buzzState) { tone(BUZZER, 2800, 150); digitalWrite(LED_RED, HIGH); }
      else           { noTone(BUZZER);           digitalWrite(LED_RED, LOW);  }
    }
  }

  // ── Emergency timer บน OLED ────────────────────────────────
  if (emergencyActive) {
    static unsigned long lastOLED = 0;
    if (millis() - lastOLED >= 1000) {
      lastOLED = millis();
      displayEmergencyTimer();
    }
  }

  // ── ปุ่ม MODE ─────────────────────────────────────────────
  if (digitalRead(BTN_MODE) == LOW && !modeLocked) {
    if (millis() - lastButtonPress > DEBOUNCE_MS) {
      lastButtonPress = millis();
      currentMode = (Mode)((currentMode + 1) % 3);
      saveMode(currentMode);
      Blynk.virtualWrite(V3, (int)currentMode);
      tone(BUZZER, 2200, 80); delay(100); noTone(BUZZER);
      setLED(currentMode);
      displayMode(currentMode);
      Serial.print(F("→ Mode: ")); printMode(currentMode); Serial.println();
    }
  }

  // ── ปุ่ม EMERGENCY ────────────────────────────────────────
  if (digitalRead(BTN_EMERGENCY) == LOW) {
    if (millis() - lastButtonPress > DEBOUNCE_MS) {
      lastButtonPress = millis();
      if (!emergencyActive) {
        triggerEmergency("MANUAL SOS");
      } else {
        // กดซ้ำ = toggle buzzer
        buzzerToggle = !buzzerToggle;
        if (!buzzerToggle) { noTone(BUZZER); digitalWrite(LED_RED, LOW); }
        Serial.println(buzzerToggle ? F("🚨 Buzzer ON") : F("🔇 Buzzer OFF"));
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
  mpu.getEvent(&a, &g, &temp);

  ml_ax[ml_idx] = a.acceleration.x;
  ml_ay[ml_idx] = a.acceleration.y;
  ml_az[ml_idx] = a.acceleration.z;
  ml_idx = (ml_idx + 1) % ML_WIN;
  ml_samples_new++;

  // Run inference every ML_STEP new samples (= every 1 s)
  if (ml_samples_new >= ML_STEP) {
    ml_samples_new = 0;
    // Build a contiguous window from the circular buffer
    float wx[ML_WIN], wy[ML_WIN], wz[ML_WIN];
    for (int i = 0; i < ML_WIN; i++) {
      int j = (ml_idx + i) % ML_WIN;
      wx[i] = ml_ax[j]; wy[i] = ml_ay[j]; wz[i] = ml_az[j];
    }
    extract_features_ml(wx, wy, wz, ML_WIN, ml_features, ML_FS);
    int pred = mlPredict(ml_features);
    ml_fall_flag = (pred == 1);
    if (ml_fall_flag && !emergencyActive) {
      Serial.println(F("[ML] FALL predicted → triggering emergency"));
      triggerEmergency("ML FALL");
    }
  }
}

// ============================================================
//  ML — FEATURE EXTRACTION (23 features, matches Python)
// ============================================================
void extract_features_ml(float* ax, float* ay, float* az,
                          int n, float* out, float fs) {
  // ── Statistics ─────────────────────────────────────────────
  float sum_ax = 0, sum_ay = 0, sum_az = 0;
  float mag[ML_WIN];
  for (int i = 0; i < n; i++) {
    mag[i] = sqrt(ax[i]*ax[i] + ay[i]*ay[i] + az[i]*az[i]);
    sum_ax += ax[i]; sum_ay += ay[i]; sum_az += az[i];
  }
  float mean_ax = sum_ax/n, mean_ay = sum_ay/n, mean_az = sum_az/n;
  float mag_sum = 0;
  for (int i = 0; i < n; i++) mag_sum += mag[i];
  float mag_mean = mag_sum / n;

  float var_ax = 0, var_ay = 0, var_az = 0, var_mag = 0;
  for (int i = 0; i < n; i++) {
    var_ax  += (ax[i]-mean_ax)*(ax[i]-mean_ax);
    var_ay  += (ay[i]-mean_ay)*(ay[i]-mean_ay);
    var_az  += (az[i]-mean_az)*(az[i]-mean_az);
    var_mag += (mag[i]-mag_mean)*(mag[i]-mag_mean);
  }
  float std_ax = sqrt(var_ax/n), std_ay = sqrt(var_ay/n), std_az = sqrt(var_az/n);
  float std_mag = sqrt(var_mag/n) + 1e-9f;

  float min_mag = mag[0], max_mag = mag[0];
  for (int i = 1; i < n; i++) {
    if (mag[i] < min_mag) min_mag = mag[i];
    if (mag[i] > max_mag) max_mag = mag[i];
  }

  float rms_ax = 0, rms_ay = 0, rms_az = 0;
  for (int i = 0; i < n; i++) {
    rms_ax += ax[i]*ax[i]; rms_ay += ay[i]*ay[i]; rms_az += az[i]*az[i];
  }
  rms_ax = sqrt(rms_ax/n); rms_ay = sqrt(rms_ay/n); rms_az = sqrt(rms_az/n);

  // ── Skewness + Kurtosis ─────────────────────────────────────
  float skew = 0, kurt = 0;
  for (int i = 0; i < n; i++) {
    float d = (mag[i] - mag_mean) / std_mag;
    skew += d*d*d; kurt += d*d*d*d;
  }
  skew /= n;
  kurt  = kurt/n - 3.0f;  // excess kurtosis

  // ── Zero-crossing ───────────────────────────────────────────
  int zc = 0;
  for (int i = 1; i < n; i++) {
    if ((mag[i]-mag_mean) * (mag[i-1]-mag_mean) < 0) zc++;
  }

  // ── SMA ─────────────────────────────────────────────────────
  float sma = 0;
  for (int i = 0; i < n; i++) sma += fabs(ax[i]) + fabs(ay[i]) + fabs(az[i]);
  sma /= n;

  // ── FFT — dominant freq + spectral energy ──────────────────
  static double vReal[ML_WIN], vImag[ML_WIN];
  for (int i = 0; i < n; i++) { vReal[i] = mag[i]; vImag[i] = 0.0; }
  ArduinoFFT<double> FFT(vReal, vImag, n, fs);
  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();
  float dom_freq  = (float)FFT.majorPeak();
  float spec_e = 0;
  for (int i = 0; i < n/2; i++) spec_e += (float)(vReal[i]*vReal[i]);

  // ── Max Jerk ────────────────────────────────────────────────
  float max_jerk = 0;
  for (int i = 1; i < n; i++) {
    float j = fabs(mag[i] - mag[i-1]) * fs;
    if (j > max_jerk) max_jerk = j;
  }

  // ── Pearson Correlations ─────────────────────────────────────
  float cxy = 0, cyz = 0, cxz = 0;
  for (int i = 0; i < n; i++) {
    cxy += (ax[i]-mean_ax)*(ay[i]-mean_ay);
    cyz += (ay[i]-mean_ay)*(az[i]-mean_az);
    cxz += (ax[i]-mean_ax)*(az[i]-mean_az);
  }
  float dxy = std_ax * std_ay * n + 1e-9f;
  float dyz = std_ay * std_az * n + 1e-9f;
  float dxz = std_ax * std_az * n + 1e-9f;

  // ── Pack 23 features ─────────────────────────────────────────
  int k = 0;
  out[k++] = mean_ax; out[k++] = mean_ay; out[k++] = mean_az;
  out[k++] = std_ax;  out[k++] = std_ay;  out[k++] = std_az;
  out[k++] = min_mag; out[k++] = max_mag; out[k++] = max_mag - min_mag;
  out[k++] = rms_ax;  out[k++] = rms_ay;  out[k++] = rms_az;
  out[k++] = skew;    out[k++] = kurt;
  out[k++] = (float)zc; out[k++] = sma;
  out[k++] = dom_freq;  out[k++] = spec_e;
  out[k++] = cxy/dxy;   out[k++] = cyz/dyz;   out[k++] = cxz/dxz;
  out[k++] = max_jerk;
  out[k++] = var_mag/n;   // acc_variance  (k now = 23)
}

// ============================================================
//  ML — PREDICT (mode-aware, returns 0=ADL 1=FALL)
// ============================================================
int mlPredict(float* features) {
  switch (currentMode) {
    case CHEST: return FallDetectorChest::predict(features);
    case SHIRT: return FallDetectorShirt::predict(features);
    case PANTS: return FallDetectorPants::predict(features);
    default:    return 0;
  }
}

// ============================================================
//  FALL DETECTION STATE MACHINE
// ============================================================
void processMPU() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;
  float acc_mag = sqrt(ax*ax + ay*ay + az*az);

  switch (fallState) {

    case FALL_IDLE:
      if (acc_mag < FREEFALL_THRESHOLD) {
        fallState     = FALL_FREEFALL;
        freefallStart = millis();
        Serial.println(F("⚠️  Freefall detected"));
      }
      break;

    case FALL_FREEFALL:
      if (acc_mag > IMPACT_THRESHOLD) {
        fallState  = FALL_IMPACT;
        impactTime = millis();
        Blynk.virtualWrite(V0, 1);  // WARNING
        Serial.println(F("💥 Impact detected → starting verification"));
      } else if (millis() - freefallStart > 2000) {
        fallState = FALL_IDLE;  // freefall นานเกิน → false positive
      }
      break;

    case FALL_IMPACT:
      fallState = FALL_VERIFY;
      break;

    case FALL_VERIFY:
      if (millis() - impactTime >= VERIFY_WINDOW) {
        // az ต่ำ + magnitude ~1g = นอนอยู่ยังไม่ลุก
        bool thresholdFall = (abs(az) < LYING_THRESHOLD && acc_mag < 12.0f);
        // ML ยืนยันจาก window ล่าสุด (ลด false positive)
        bool mlFall = ml_fall_flag;
        if (thresholdFall || mlFall) {
          Serial.print(F("FALL CONFIRMED — threshold:"));
          Serial.print(thresholdFall); Serial.print(F(" ML:"));
          Serial.println(mlFall);
          fallState = FALL_EMERGENCY;
          triggerEmergency(mlFall ? "ML+THRESH FALL" : "THRESH FALL");
        } else {
          // ลุกขึ้นได้ = Near Fall เท่านั้น
          Serial.println(F("Near Fall — person recovered"));
          tone(BUZZER, 1500, 400);
          Blynk.virtualWrite(V0, 1);   // WARNING brief
          fallState = FALL_IDLE;
          ml_fall_flag = false;
        }
      }
      break;

    case FALL_EMERGENCY:
      break;  // รอ acknowledge
  }
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
  ml_fall_flag    = false;  // reset so ML doesn't re-fire

  Serial.println(F("\n🔴 ══════════════════════════════"));
  Serial.print(F("   EMERGENCY: ")); Serial.println(reason);
  Serial.print(F("   Mode: ")); printMode(currentMode); Serial.println();
  Serial.print(F("   GPS : ")); Serial.print(gpsLat, 6);
  Serial.print(F(", "));        Serial.println(gpsLon, 6);
  Serial.println(F("🔴 ══════════════════════════════\n"));

  // ส่ง Blynk
  Blynk.virtualWrite(V0, 2);               // FALL
  Blynk.virtualWrite(V1, gpsLat);
  Blynk.virtualWrite(V2, gpsLon);
  Blynk.virtualWrite(V3, (int)currentMode);
  Blynk.virtualWrite(V4, 1);              // Emergency active

  // LED ทุกดวงติด
  digitalWrite(LED_GREEN,  HIGH);
  digitalWrite(LED_YELLOW, HIGH);
  digitalWrite(LED_RED,    HIGH);

  displayFall(reason);
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

void displayMode(Mode mode) {
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
