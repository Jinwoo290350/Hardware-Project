#pragma once
/*
 * =============================================
 * config.h  —  USER DEVICE (ฝั่งผู้สูงอายุ)
 * =============================================
 * ⚠️  ห้าม commit ไฟล์นี้ขึ้น GitHub (มี credentials)
 * =============================================
 */

// ── Blynk (ต้องอยู่ก่อน #include <BlynkSimpleEsp32.h>) ──────
#define BLYNK_TEMPLATE_ID   "TMPL6SvhV0s65"
#define BLYNK_TEMPLATE_NAME "ElderlySafetySystem"
#define BLYNK_AUTH_TOKEN    "3vx4baXQOJYI8nFrQt9ya3CaeghTU5KV"

// ── WiFi ─────────────────────────────────────────────────────
// เปลี่ยนเป็น hotspot มือถือ: ใส่ชื่อ WiFi และรหัสผ่านด้านล่าง
#define WIFI_SSID  "Frank"
#define WIFI_PASS  "frank285"

// ── Device placement mode ─────────────────────────────────────
// (ต้องอยู่ใน config.h เพื่อให้ compiler เห็น enum ก่อน auto-generated prototypes)
enum Mode { CHEST = 0, SHIRT = 1, PANTS = 2 };

// ── Buzzer resonant frequency ────────────────────────────────
// ปรับค่านี้เพื่อหาความถี่ที่ buzzer ดังที่สุด (ทดสอบจาก startup sweep)
// buzzer ราคาถูกส่วนใหญ่อยู่ที่ 2400-3000Hz
#define BUZZER_FREQ  2730

// ── Debug: ปิด fall detection อัตโนมัติ (ใช้เฉพาะทดสอบ) ────────────
// Uncomment บรรทัดด้านล่างเพื่อปิด threshold + ML fall detection
// (BTN_EMERGENCY ยังทำงานได้ปกติ)
// #define DISABLE_AUTO_FALL   // ← เปิด fall detection แล้ว

// ── Caregiver device token (ใช้ส่ง HTTP โดยตรงแทน Blynk Automation) ────
#define CAREGIVER_TOKEN "zKh2MMHzfkT4l2fXpKjc_TcjgLIT23LS"

// ── MQTT (สำรอง — ไม่ได้ใช้ใน Blynk mode) ────────────────────
// #define MQTT_BROKER "iot.cpe.ku.ac.th"
// #define MQTT_USER   "b6810503943"
// #define MQTT_PASS   "veerapong.h@ku.th"
// #define TOPIC_PREFIX "b6810503943"
