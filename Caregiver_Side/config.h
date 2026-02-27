#pragma once
/*
 * =============================================
 * config.h  —  CAREGIVER DEVICE (ฝั่งผู้ดูแล)
 * =============================================
 * ⚠️  ห้าม commit ไฟล์นี้ขึ้น GitHub (มี credentials)
 * =============================================
 */

// ── Blynk (ต้องอยู่ก่อน #include <BlynkSimpleEsp32.h>) ──────
#define BLYNK_TEMPLATE_ID   "TMPL6SvhV0s65"
#define BLYNK_TEMPLATE_NAME "ElderlySafetySystem"
#define BLYNK_AUTH_TOKEN    "zKh2MMHzfkT4l2fXpKjc_TcjgLIT23LS"

// ── WiFi ─────────────────────────────────────────────────────
#define WIFI_SSID  "KUWIN-IOT"
#define WIFI_PASS  ""           // open network — ไม่มีรหัสผ่าน

// ── MQTT (สำรอง — ไม่ได้ใช้ใน Blynk mode) ────────────────────
// #define MQTT_BROKER "iot.cpe.ku.ac.th"
// #define MQTT_USER   "b6810503943"
// #define MQTT_PASS   "veerapong.h@ku.th"
// #define TOPIC_PREFIX "b6810503943"
