/*
 * =============================================
 * ELDERLY SAFETY SYSTEM  —  CAREGIVER-SIDE
 * =============================================
 * รับแจ้งเตือนจาก User Device ผ่าน Blynk
 *
 * Hardware:
 *   ESP32 (ทุกรุ่น)
 *   LED แดง       → GPIO 2  (หรือ LED_BUILTIN)
 *   Buzzer        → GPIO 15
 *   BTN_ACK       → GPIO 0  (ปุ่ม BOOT บน ESP32)
 *                   กดเพื่อ acknowledge emergency
 *
 * Library: Blynk by Volodymyr Shymanskyy
 *
 * Blynk Virtual Pins (ตรงกับ User Device):
 *   V0 – Status    0=NORMAL 1=WARNING 2=FALL
 *   V1 – GPS Lat
 *   V2 – GPS Lon
 *   V3 – Mode      0=CHEST 1=SHIRT 2=PANTS
 *   V4 – Emergency 0/1  ← trigger หลักของอุปกรณ์นี้
 *
 * ⚠️  TEMPLATE_ID และ WIFI ต้องตรงกับ User Device
 *     AUTH_TOKEN ต้องเป็นของ Caregiver Device (คนละ token)
 *     ตั้ง Blynk Automation:
 *       "When User Device V4 = 1 → Write Caregiver V4 = 1"
 *
 * ⚠️  กรอก BLYNK_TEMPLATE_ID / AUTH_TOKEN / WiFi ก่อน upload
 * =============================================
 */

// ===== CONFIG (Blynk + WiFi credentials) =====
// BLYNK_TEMPLATE_ID / AUTH_TOKEN / WiFi อยู่ใน config.h
#include "config.h"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials มาจาก config.h (WIFI_SSID / WIFI_PASS)

// ===== PIN DEFINITIONS =====
#define LED_ALERT  2    // LED แดง (LED_BUILTIN บน ESP32 ส่วนใหญ่)
#define BUZZER     15   // Buzzer
#define BTN_ACK    0    // ปุ่ม Acknowledge (BOOT button)

// ===== STATE =====
bool alertActive   = false;
bool nearFallAlert = false;

unsigned long lastBuzz        = 0;
unsigned long lastButtonPress = 0;

float receivedLat  = 0.0f;
float receivedLon  = 0.0f;
int   receivedMode = 0;

const unsigned long DEBOUNCE_MS = 300;

// ============================================================
//  BLYNK CALLBACKS  —  รับข้อมูลจาก User Device
// ============================================================

// V4 = Emergency active (0/1)
BLYNK_WRITE(V4) {
  int val = param.asInt();

  if (val == 1 && !alertActive) {
    alertActive   = true;
    nearFallAlert = false;

    Serial.println(F("\n🚨 ══════════════════════════════"));
    Serial.println(F("   EMERGENCY ALERT RECEIVED!"));
    printReceivedInfo();
    Serial.println(F("🚨 ══════════════════════════════\n"));

    // Startup alarm burst
    for (int i = 0; i < 3; i++) {
      tone(BUZZER, 3000, 200);
      digitalWrite(LED_ALERT, HIGH);
      delay(250);
      noTone(BUZZER);
      digitalWrite(LED_ALERT, LOW);
      delay(100);
    }

  } else if (val == 0 && alertActive) {
    // ถูก acknowledge จาก User Device หรือ Web
    alertActive = false;
    noTone(BUZZER);
    digitalWrite(LED_ALERT, LOW);
    Serial.println(F("✓ Emergency cleared (remote acknowledge)"));
  }
}

// V0 = Status (0=NORMAL, 1=WARNING, 2=FALL)
BLYNK_WRITE(V0) {
  int status = param.asInt();
  if (status == 1 && !alertActive) {
    // Near Fall warning — beep 1 ครั้ง + LED กระพริบสั้น
    nearFallAlert = true;
    Serial.println(F("⚠️  Near Fall WARNING received"));
    tone(BUZZER, 1500, 400);
    digitalWrite(LED_ALERT, HIGH);
    delay(500);
    noTone(BUZZER);
    digitalWrite(LED_ALERT, LOW);
  } else if (status == 0) {
    nearFallAlert = false;
    if (!alertActive) {
      Serial.println(F("✓ System NORMAL"));
    }
  }
}

// V1 = GPS Lat
BLYNK_WRITE(V1) { receivedLat = param.asFloat(); }

// V2 = GPS Lon
BLYNK_WRITE(V2) { receivedLon = param.asFloat(); }

// V3 = Mode (0=CHEST, 1=SHIRT, 2=PANTS)
BLYNK_WRITE(V3) {
  receivedMode = param.asInt();
  Serial.print(F("ℹ️  Mode updated: "));
  printModeName(receivedMode);
  Serial.println();
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n========================================"));
  Serial.println(F("   ELDERLY SAFETY SYSTEM"));
  Serial.println(F("   Caregiver-Side Device"));
  Serial.println(F("========================================\n"));

  pinMode(LED_ALERT, OUTPUT);
  pinMode(BUZZER,    OUTPUT);
  pinMode(BTN_ACK,   INPUT_PULLUP);

  digitalWrite(LED_ALERT, LOW);
  digitalWrite(BUZZER,    LOW);

  // Startup indication
  digitalWrite(LED_ALERT, HIGH);
  tone(BUZZER, 1000, 150); delay(200); noTone(BUZZER);
  tone(BUZZER, 1500, 150); delay(200); noTone(BUZZER);
  digitalWrite(LED_ALERT, LOW);

  Serial.print(F("⏳ Connecting WiFi: ")); Serial.println(WIFI_SSID);
  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASS[0] ? WIFI_PASS : nullptr);

  Serial.println(F("✓ Blynk connected"));
  Serial.println(F("📡 Waiting for alerts from User Device..."));
  Serial.println(F("   Press BTN_ACK (GPIO 0) to acknowledge emergency\n"));

  // Confirm connected
  tone(BUZZER, 2000, 100); delay(150); noTone(BUZZER);
  Blynk.syncAll();  // sync current state from Blynk cloud
}

// ============================================================
//  MAIN LOOP
// ============================================================
void loop() {
  Blynk.run();

  // ── Emergency: LED กระพริบ + Buzzer รัว ──────────────────
  if (alertActive) {
    static bool buzzState = false;
    if (millis() - lastBuzz > 300) {
      lastBuzz  = millis();
      buzzState = !buzzState;
      if (buzzState) {
        tone(BUZZER, 2800, 200);
        digitalWrite(LED_ALERT, HIGH);
      } else {
        noTone(BUZZER);
        digitalWrite(LED_ALERT, LOW);
      }
    }
  }

  // ── ปุ่ม Acknowledge ─────────────────────────────────────
  if (digitalRead(BTN_ACK) == LOW && alertActive) {
    if (millis() - lastButtonPress > DEBOUNCE_MS) {
      lastButtonPress = millis();
      alertActive = false;
      noTone(BUZZER);
      digitalWrite(LED_ALERT, LOW);

      // แจ้ง Blynk ว่า acknowledge แล้ว
      Blynk.virtualWrite(V4, 0);

      Serial.println(F("✓ Emergency acknowledged by caregiver"));
      Serial.print(F("  Location was: "));
      Serial.print(receivedLat, 6); Serial.print(F(", "));
      Serial.println(receivedLon, 6);

      // Confirmation beep
      tone(BUZZER, 1200, 150); delay(200); noTone(BUZZER);
      tone(BUZZER, 1600, 150); delay(200); noTone(BUZZER);
    }
  }

  delay(10);
}

// ============================================================
//  HELPERS
// ============================================================
void printReceivedInfo() {
  Serial.print(F("   Mode: ")); printModeName(receivedMode); Serial.println();
  Serial.print(F("   Lat : ")); Serial.println(receivedLat, 6);
  Serial.print(F("   Lon : ")); Serial.println(receivedLon, 6);
  Serial.println(F("   Press BTN_ACK to acknowledge"));
}

void printModeName(int mode) {
  switch (mode) {
    case 0: Serial.print(F("CHEST CLIP"));   break;
    case 1: Serial.print(F("SHIRT POCKET")); break;
    case 2: Serial.print(F("PANTS POCKET")); break;
    default: Serial.print(F("UNKNOWN"));     break;
  }
}
