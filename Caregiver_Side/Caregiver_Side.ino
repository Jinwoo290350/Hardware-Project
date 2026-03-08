/*
 * =============================================
 * ELDERLY SAFETY SYSTEM  —  CAREGIVER-SIDE
 * =============================================
 * รับแจ้งเตือนจาก User Device ผ่าน Blynk
 *
 * Hardware:
 *   ESP32 (ทุกรุ่น)
 *   LED เขียว    → GPIO 40  (Blynk connected / NORMAL)
 *   LED เหลือง   → GPIO 41  (Near-Fall WARNING)
 *   LED แดง      → GPIO 42  (EMERGENCY — กระพริบ)
 *   Buzzer        → GPIO 15  (active-LOW passive buzzer)
 *   SWITCH        → GPIO 2   (Acknowledge emergency)
 *   LDR           → GPIO 4   (อ่านแสงสว่าง → log Serial)
 *
 * Blynk Virtual Pins (ตรงกับ User Device):
 *   V0 – Status    0=NORMAL 1=WARNING 2=FALL
 *   V1 – GPS Lat
 *   V2 – GPS Lon
 *   V3 – Mode      0=CHEST 1=SHIRT 2=PANTS
 *   V4 – Emergency 0/1  ← trigger หลักของอุปกรณ์นี้
 *
 * ⚠️  TEMPLATE_ID / AUTH_TOKEN / WiFi อยู่ใน config.h
 * =============================================
 */

#include "config.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// ===== PIN DEFINITIONS =====
#define LED_GREEN   40   // Blynk connected / NORMAL
#define LED_YELLOW  41   // Near-Fall WARNING
#define LED_RED     42   // EMERGENCY (กระพริบ)
#define BUZZER      15   // active-LOW passive buzzer
#define SWITCH_PIN   2   // Acknowledge button (INPUT_PULLUP → LOW when pressed)
#define LDR_PIN      4   // Light Dependent Resistor (ADC)

// ===== TIMING =====
const unsigned long DEBOUNCE_MS    = 300;
const unsigned long LDR_INTERVAL   = 5000;  // อ่าน LDR ทุก 5 วิ
const unsigned long BLINK_INTERVAL = 300;   // กระพริบ RED ทุก 300ms

// ===== STATE =====
bool alertActive   = false;
bool nearFallAlert = false;

float receivedLat  = 0.0f;
float receivedLon  = 0.0f;
int   receivedMode = 0;

unsigned long lastBuzz        = 0;
unsigned long lastButtonPress = 0;
unsigned long lastLdrRead     = 0;
bool          buzzState       = false;

// ===== HELPERS =====
// active-LOW buzzer: HIGH = silent
inline void silenceBuzzer() { noTone(BUZZER); digitalWrite(BUZZER, HIGH); }

void setLEDs(bool green, bool yellow, bool red) {
  digitalWrite(LED_GREEN,  green  ? HIGH : LOW);
  digitalWrite(LED_YELLOW, yellow ? HIGH : LOW);
  digitalWrite(LED_RED,    red    ? HIGH : LOW);
}

void printModeName(int mode) {
  switch (mode) {
    case 0: Serial.print(F("CHEST CLIP"));   break;
    case 1: Serial.print(F("SHIRT POCKET")); break;
    case 2: Serial.print(F("PANTS POCKET")); break;
    default: Serial.print(F("UNKNOWN"));     break;
  }
}

void printReceivedInfo() {
  Serial.print(F("   Mode: ")); printModeName(receivedMode); Serial.println();
  Serial.print(F("   Lat : ")); Serial.println(receivedLat, 6);
  Serial.print(F("   Lon : ")); Serial.println(receivedLon, 6);
  Serial.println(F("   Press SWITCH (GPIO 2) to acknowledge"));
}

// ============================================================
//  BLYNK CALLBACKS
// ============================================================

// V4 = Emergency active (0/1)
BLYNK_WRITE(V4) {
  int val = param.asInt();

  if (val == 1 && !alertActive) {
    alertActive   = true;
    nearFallAlert = false;

    Serial.println(F("\n[EMERGENCY] ══════════════════════════════"));
    Serial.println(F("   EMERGENCY ALERT RECEIVED!"));
    printReceivedInfo();
    Serial.println(F("[EMERGENCY] ══════════════════════════════\n"));

    // ปิด LED เหลือง, เปิด LED แดง
    digitalWrite(LED_YELLOW, LOW);

    // Startup alarm burst — 3 beeps + RED flash
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 3000, 200);
      digitalWrite(LED_RED, HIGH);
      delay(250);
      silenceBuzzer();
      digitalWrite(LED_RED, LOW);
      delay(100);
    }

  } else if (val == 0 && alertActive) {
    alertActive = false;
    silenceBuzzer();
    setLEDs(true, false, false);  // กลับสู่ NORMAL (เขียว)
    Serial.println(F("[OK] Emergency cleared (remote acknowledge)"));
  }
}

// V0 = Status (0=NORMAL, 1=WARNING, 2=FALL)
BLYNK_WRITE(V0) {
  int status = param.asInt();

  if (status == 2 && !alertActive) {
    // FALL status จาก V0 (backup ถ้า V4 ยังไม่มา)
    // ปล่อยให้ V4 เป็น trigger หลัก — ไม่ต้องทำซ้ำ

  } else if (status == 1 && !alertActive) {
    nearFallAlert = true;
    Serial.println(F("[WARN] Near Fall WARNING received"));
    // Yellow LED flash 2 ครั้ง + short beep
    for (int i = 0; i < 2; i++) {
      tone(BUZZER, 1800, 200);
      digitalWrite(LED_YELLOW, HIGH);
      delay(300);
      silenceBuzzer();
      digitalWrite(LED_YELLOW, LOW);
      delay(150);
    }

  } else if (status == 0 && !alertActive) {
    nearFallAlert = false;
    setLEDs(true, false, false);  // GREEN steady = NORMAL
    Serial.println(F("[OK] System NORMAL"));
  }
}

// V1 = GPS Lat
BLYNK_WRITE(V1) { receivedLat = param.asFloat(); }

// V2 = GPS Lon
BLYNK_WRITE(V2) { receivedLon = param.asFloat(); }

// V3 = Mode
BLYNK_WRITE(V3) {
  receivedMode = param.asInt();
  Serial.print(F("[INFO] Mode updated: "));
  printModeName(receivedMode);
  Serial.println();
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(800);
  Serial.println(F("\n========================================"));
  Serial.println(F("   ELDERLY SAFETY SYSTEM"));
  Serial.println(F("   Caregiver-Side Device"));
  Serial.println(F("========================================\n"));

  // Output pins
  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BUZZER,     OUTPUT);

  // Input pins
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  // LDR: ADC — no pinMode needed (analogRead handles it)

  // Safe idle state
  setLEDs(false, false, false);
  digitalWrite(BUZZER, HIGH);  // active-LOW: HIGH = silent

  // Startup LED sweep
  digitalWrite(LED_RED,    HIGH); delay(200); digitalWrite(LED_RED,    LOW);
  digitalWrite(LED_YELLOW, HIGH); delay(200); digitalWrite(LED_YELLOW, LOW);
  digitalWrite(LED_GREEN,  HIGH); delay(200);

  tone(BUZZER, 1200, 120); delay(180); silenceBuzzer();
  delay(80);
  tone(BUZZER, 1800, 120); delay(180); silenceBuzzer();

  // Connect Blynk (blocking)
  Serial.print(F("[WiFi] Connecting: ")); Serial.println(WIFI_SSID);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS[0] ? WIFI_PASS : nullptr);

  Serial.println(F("[OK] Blynk connected"));
  Serial.println(F("[OK] Waiting for alerts from User Device..."));
  Serial.println(F("     Press SWITCH (GPIO 2) to acknowledge emergency\n"));

  // Connection confirmation beep
  tone(BUZZER, 2200, 100); delay(150); silenceBuzzer();

  setLEDs(true, false, false);  // GREEN = connected

  Blynk.syncAll();
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  Blynk.run();

  unsigned long now = millis();

  // ── Emergency: RED กระพริบ + Buzzer รัว ─────────────────
  if (alertActive) {
    if (now - lastBuzz > BLINK_INTERVAL) {
      lastBuzz  = now;
      buzzState = !buzzState;
      if (buzzState) {
        tone(BUZZER, 2800, 200);
        digitalWrite(LED_RED, HIGH);
      } else {
        silenceBuzzer();
        digitalWrite(LED_RED, LOW);
      }
    }
  }

  // ── SWITCH Acknowledge ────────────────────────────────────
  if (digitalRead(SWITCH_PIN) == LOW && alertActive) {
    if (now - lastButtonPress > DEBOUNCE_MS) {
      lastButtonPress = now;
      alertActive     = false;
      buzzState       = false;

      silenceBuzzer();
      setLEDs(true, false, false);  // กลับ GREEN

      // แจ้ง Blynk ว่า acknowledge แล้ว
      Blynk.virtualWrite(V4, 0);

      Serial.println(F("[ACK] Emergency acknowledged by caregiver"));
      Serial.print(F("      Location: "));
      Serial.print(receivedLat, 6); Serial.print(F(", "));
      Serial.println(receivedLon, 6);

      // Confirmation beeps
      tone(BUZZER, 1400, 120); delay(180); silenceBuzzer();
      delay(80);
      tone(BUZZER, 1800, 120); delay(180); silenceBuzzer();
    }
  }

  // ── LDR: อ่านแสงสว่างทุก 5 วิ ───────────────────────────
  if (now - lastLdrRead > LDR_INTERVAL) {
    lastLdrRead = now;
    int ldr = analogRead(LDR_PIN);
    Serial.print(F("[LDR] Ambient light = "));
    Serial.print(ldr);
    Serial.println(F(" (0=dark, 4095=bright)"));
  }

  delay(10);
}
