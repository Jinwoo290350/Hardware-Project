/**
 * RealData Logger — MPU6050 Auto-Cycle Data Collection
 * บันทึก accelerometer 50Hz สำหรับ training ML model fall detection
 *
 * ══ CONFIG (แก้ก่อน upload) ══════════════════════════════════
 *   LOGGER_MODE  →  CHEST / SHIRT / PANTS
 * ════════════════════════════════════════════════════════════
 *
 * Auto-cycle sequence (5 รอบ แล้วหยุด):
 *   NORMAL  30s → [3 beep] → FALLING 5s → [3 beep] → FALLEN 10s → [3 beep] → repeat
 *
 * Serial output @ 115200 — CSV:
 *   ms,ax,ay,az,label,mode
 *   label:  0=NORMAL   1=FALLING   2=FALLEN
 *   mode:   0=CHEST    1=SHIRT     2=PANTS
 */

// ============================================================
//  ★ CONFIG — แก้ตรงนี้ก่อน upload ★
// ============================================================
#define LOGGER_MODE   PANTS    // เลือก: CHEST / SHIRT / PANTS

// ============================================================
//  INCLUDES
// ============================================================
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <driver/gpio.h>    // GPIO_DRIVE_CAP_3 (40mA)

// ============================================================
//  PINS
// ============================================================
#define SDA_PIN     8
#define SCL_PIN     9
#define LED_GREEN   4
#define LED_YELLOW  5
#define LED_RED     6
#define BUZZER      7

// ============================================================
//  OLED
// ============================================================
#define SCREEN_W  128
#define SCREEN_H   64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);
bool oledOK = false;

// ============================================================
//  STATE
// ============================================================
enum Mode  : uint8_t { CHEST = 0, SHIRT = 1, PANTS = 2 };
enum Label : uint8_t { LBL_NORMAL = 0, LBL_FALLING = 1, LBL_FALLEN = 2 };

const Mode  fixedMode  = LOGGER_MODE;
Label currentLabel     = LBL_NORMAL;

const char* const modeName[]  = { "CHEST",  "SHIRT",  "PANTS"  };
const char* const labelName[] = { "NORMAL", "FALLING","FALLEN" };

// ============================================================
//  MPU6050
// ============================================================
uint8_t mpuAddr  = 0x68;
float   mpuScale = 4096.0f;
static float lastAx = 0.0f, lastAy = 0.0f, lastAz = 9.80665f;

// ============================================================
//  TIMING — AUTO CYCLE
// ============================================================
#define SAMPLE_MS      20           // 50Hz
#define NORMAL_MS      (30*1000UL)  // 30 วินาที
#define FALLING_MS     ( 5*1000UL)  //  5 วินาที
#define FALLEN_MS      (10*1000UL)  // 10 วินาที
#define MAX_LOOPS      5            // รอบทั้งหมด

unsigned long lastSample   = 0;
unsigned long sampleTotal  = 0;
unsigned long labelSamples = 0;
unsigned long stateStart   = 0;     // millis() เมื่อ label เริ่ม
int           loopCount    = 0;     // รอบที่เสร็จแล้ว (0-based)
bool          logDone      = false;

// ============================================================
//  HELPERS
// ============================================================
void setLED(Mode m) {
  digitalWrite(LED_GREEN,  m == CHEST ? HIGH : LOW);
  digitalWrite(LED_YELLOW, m == SHIRT ? HIGH : LOW);
  digitalWrite(LED_RED,    m == PANTS ? HIGH : LOW);
}

// ── ปิด buzzer และคืน drive 40mA (noTone() reset drive strength) ──
void silenceBuzzer() {
  noTone(BUZZER);
  gpio_reset_pin((gpio_num_t)BUZZER);
  gpio_set_direction((gpio_num_t)BUZZER, GPIO_MODE_OUTPUT);
  gpio_set_level((gpio_num_t)BUZZER, 0);
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  gpio_set_drive_capability((gpio_num_t)BUZZER, GPIO_DRIVE_CAP_3);  // 40mA
}

void beepN(int n, int freq, int durMs = 150, int gapMs = 200) {
  for (int i = 0; i < n; i++) {
    tone(BUZZER, freq, durMs);
    delay(durMs + gapMs);
  }
  silenceBuzzer();
}

// ── OLED แสดง label ปัจจุบัน + เวลาเหลือ + รอบ ─────────────
void updateOLED() {
  if (!oledOK || logDone) return;

  unsigned long dur = (currentLabel == LBL_NORMAL)   ? NORMAL_MS  :
                      (currentLabel == LBL_FALLING)  ? FALLING_MS : FALLEN_MS;
  unsigned long elapsed = millis() - stateStart;
  int secLeft = (int)(max(0UL, dur - elapsed) / 1000UL);

  display.clearDisplay();

  // Label ใหญ่
  display.setTextSize(2); display.setCursor(0, 0);
  display.println(labelName[currentLabel]);

  // เวลาเหลือ
  display.setTextSize(2); display.setCursor(0, 20);
  display.print(secLeft); display.println("s ");

  // รอบ + total samples
  display.setTextSize(1); display.setCursor(0, 44);
  display.print("Loop "); display.print(loopCount + 1);
  display.print("/"); display.print(MAX_LOOPS);
  display.print("  N:"); display.println(sampleTotal);

  display.display();
}

void showDoneOLED() {
  if (!oledOK) return;
  display.clearDisplay();
  display.setTextSize(2); display.setCursor(0, 10);
  display.println("  DONE!");
  display.setTextSize(1); display.setCursor(0, 44);
  display.print("Total: "); display.println(sampleTotal);
  display.display();
}

// ============================================================
//  MPU6050 INIT
// ============================================================
bool mpuInit() {
  for (uint8_t addr : {0x68, 0x69}) {
    Wire.beginTransmission(addr);
    Wire.write(0x75);
    if (Wire.endTransmission(true) != 0) continue;
    Wire.requestFrom(addr, (uint8_t)1);
    if (!Wire.available()) continue;
    if (Wire.read() == 0xFF) continue;
    mpuAddr = addr;
    break;
  }
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x6B); Wire.write(0x00);
  if (Wire.endTransmission(true) != 0) return false;
  delay(100);
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x1A); Wire.write(0x04);
  Wire.endTransmission(true);
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x1C); Wire.write(0x00);
  Wire.endTransmission(true);
  delay(10);
  return true;
}

void readAccel(float &ax, float &ay, float &az) {
  Wire.beginTransmission(mpuAddr);
  Wire.write(0x3B);
  Wire.endTransmission(true);
  Wire.requestFrom(mpuAddr, (uint8_t)6);
  if (Wire.available() < 6) { ax = lastAx; ay = lastAy; az = lastAz; return; }
  int16_t rx = ((int16_t)Wire.read() << 8) | Wire.read();
  int16_t ry = ((int16_t)Wire.read() << 8) | Wire.read();
  int16_t rz = ((int16_t)Wire.read() << 8) | Wire.read();
  ax = rx * (9.80665f / mpuScale);
  ay = ry * (9.80665f / mpuScale);
  az = rz * (9.80665f / mpuScale);
  lastAx = ax; lastAy = ay; lastAz = az;
}

// ============================================================
//  SELF-CALIBRATE SCALE  (10s — วางนิ่ง)
// ============================================================
void calibrateScale() {
  Serial.println(F("# [CAL] Keep STILL 10s..."));
  float rawMagSum = 0; int cnt = 0;
  for (int i = 0; i < 200; i++) {
    if (i % 50 == 0 && oledOK) {
      display.clearDisplay();
      display.setTextSize(2); display.setCursor(0, 0);
      display.println("CALIBRATE");
      display.setTextSize(1); display.setCursor(0, 22);
      display.print("STILL "); display.print((200 - i) / 20); display.println("s");
      display.display();
    }
    Wire.beginTransmission(mpuAddr);
    Wire.write(0x3B); Wire.endTransmission(true);
    Wire.requestFrom(mpuAddr, (uint8_t)6);
    if (Wire.available() >= 6) {
      int16_t rx = ((int16_t)Wire.read() << 8) | Wire.read();
      int16_t ry = ((int16_t)Wire.read() << 8) | Wire.read();
      int16_t rz = ((int16_t)Wire.read() << 8) | Wire.read();
      rawMagSum += sqrtf((float)rx*rx + (float)ry*ry + (float)rz*rz);
      cnt++;
    }
    delay(50);
  }
  if (cnt > 20) {
    mpuScale = rawMagSum / cnt;
    Serial.print(F("# [CAL] Scale=")); Serial.print(mpuScale, 1); Serial.println(F(" LSB/g"));
  } else {
    mpuScale = 16384.0f;
    Serial.println(F("# [CAL] WARN: fallback 16384"));
  }
}

// ── เปลี่ยน state ถัดไป (ด้วย 3 beep เตือนก่อน) ─────────────
void advanceState() {
  // คำนวณ state ถัดไป
  Label nextLbl;
  if (currentLabel == LBL_NORMAL)        nextLbl = LBL_FALLING;
  else if (currentLabel == LBL_FALLING)  nextLbl = LBL_FALLEN;
  else                                   nextLbl = LBL_NORMAL;

  // beep เตือนก่อนเปลี่ยน (เสียงของ state ใหม่)
  int freq = (nextLbl == LBL_FALLING) ? 3800
           : (nextLbl == LBL_FALLEN)  ? 2000
                                      : 2700;
  beepN(3, freq, 150, 200);   // blocking ~1.05s

  // เปลี่ยน state
  currentLabel = nextLbl;
  labelSamples = 0;
  stateStart   = millis();    // reset timer หลัง beep

  Serial.print(F("# LABEL: ")); Serial.println(labelName[currentLabel]);

  // นับรอบ: FALLEN → NORMAL = จบ 1 รอบ
  if (currentLabel == LBL_NORMAL) {
    loopCount++;
    Serial.print(F("# LOOP: ")); Serial.print(loopCount);
    Serial.print(F("/")); Serial.println(MAX_LOOPS);

    if (loopCount >= MAX_LOOPS) {
      logDone = true;
      Serial.println(F("# === DONE all loops ==="));
      showDoneOLED();
      beepN(5, 2700, 200, 100);  // เสร็จแล้ว
      return;
    }
  }

  updateOLED();
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("# === RealData Logger (Auto-Cycle) ==="));
  Serial.print(F("# MODE: ")); Serial.println(modeName[fixedMode]);
  Serial.print(F("# NORMAL=")); Serial.print(NORMAL_MS/1000);
  Serial.print(F("s  FALLING=")); Serial.print(FALLING_MS/1000);
  Serial.print(F("s  FALLEN=")); Serial.print(FALLEN_MS/1000);
  Serial.print(F("s  x")); Serial.print(MAX_LOOPS);
  Serial.println(F(" loops"));

  pinMode(LED_GREEN,  OUTPUT);
  pinMode(LED_YELLOW, OUTPUT);
  pinMode(LED_RED,    OUTPUT);
  pinMode(BUZZER,     OUTPUT);
  digitalWrite(BUZZER, LOW);
  gpio_set_drive_capability((gpio_num_t)BUZZER, GPIO_DRIVE_CAP_3);  // 40mA max

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(10000);
  Wire.setTimeOut(30);

  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledOK = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1); display.setCursor(0, 0);
    display.println("DataLogger AutoCycle");
    display.print("Mode: "); display.println(modeName[fixedMode]);
    display.display();
  }

  if (!mpuInit()) {
    Serial.println(F("# [FAIL] MPU6050"));
    while (1) delay(500);
  }

  calibrateScale();

  Serial.println(F("#"));
  Serial.print(F("# === MODE: ")); Serial.println(modeName[fixedMode]);
  Serial.println(F("# ms,ax,ay,az,label,mode"));
  Serial.println(F("# label: 0=NORMAL  1=FALLING  2=FALLEN"));
  Serial.println(F("# mode:  0=CHEST   1=SHIRT    2=PANTS"));
  Serial.println(F("#"));

  setLED(fixedMode);
  stateStart = millis();
  updateOLED();

  // Frequency sweep 2000–4000Hz เพื่อหา resonant ของ buzzer นี้
  Serial.println(F("# [SWEEP] Finding resonant frequency..."));
  for (int f = 2000; f <= 4000; f += 200) {
    tone(BUZZER, f, 100); delay(130);
  }
  silenceBuzzer();
  delay(200);
  beepN(3, 2700, 100, 150);   // ready

  Serial.println(F("# [START] Loop 1 - NORMAL"));
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  if (logDone) return;

  unsigned long now = millis();

  // ── Sample @ 50Hz ─────────────────────────────────────────
  if (now - lastSample >= SAMPLE_MS) {
    lastSample = now;
    float ax, ay, az;
    readAccel(ax, ay, az);
    sampleTotal++;
    labelSamples++;

    Serial.print(now);                    Serial.print(',');
    Serial.print(ax, 4);                  Serial.print(',');
    Serial.print(ay, 4);                  Serial.print(',');
    Serial.print(az, 4);                  Serial.print(',');
    Serial.print((uint8_t)currentLabel);  Serial.print(',');
    Serial.println((uint8_t)fixedMode);

    if (sampleTotal % 50 == 0) updateOLED();   // อัพเดต OLED ทุก 1s
  }

  // ── Auto state machine ──────────────────────────────────────
  unsigned long dur = (currentLabel == LBL_NORMAL)   ? NORMAL_MS  :
                      (currentLabel == LBL_FALLING)  ? FALLING_MS : FALLEN_MS;

  if (now - stateStart >= dur) {
    advanceState();   // beep 3 ครั้ง แล้วเปลี่ยน label
  }
}
