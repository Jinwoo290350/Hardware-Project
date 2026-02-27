# SafeStep — Elderly Fall Detection System

ระบบตรวจจับการล้มและแจ้งเตือนฉุกเฉินสำหรับผู้สูงอายุ
ใช้ **ESP32-S3 + MPU6050 + GPS + Machine Learning** ตรวจจับการล้ม และแจ้งเตือนผ่าน **Blynk** ไปยังผู้ดูแลและ **Web Dashboard** แบบ Real-time

---

## ภาพรวม

```
[User ESP32-S3]  ──Blynk──►  [Caregiver ESP32]
  MPU6050 + GPS                LED + Buzzer
  ML Model (RF)                    │
  Threshold SM                     ▼
       │                    [Blynk Cloud]
       └────────────────────────►  │
                                   ▼
                           [Web Dashboard]
                           Next.js + Auth
```

| Component | เทคโนโลยี |
|---|---|
| User Device | ESP32-S3, MPU6050, GPS, OLED SSD1306 |
| Caregiver Device | ESP32, LED, Buzzer |
| Cloud | Blynk IoT (Free plan) |
| Web | Next.js 14, TypeScript, Tailwind CSS |
| ML | AutoGluon (training) → Random Forest → micromlgen (ESP32) |

---

## โครงสร้างโปรเจกต์

```
Hardware-Project/
├── Full_System_README/          # User-side firmware
│   ├── Full_System_README.ino   # โค้ดหลัก (fall detection + ML + Blynk)
│   ├── config.h                 # Blynk token + WiFi (ไม่ commit ขึ้น Git)
│   ├── CHEST_model.h            # ML model สำหรับ Chest mode (generate จาก ML/)
│   ├── SHIRT_model.h            # ML model สำหรับ Shirt mode
│   └── PANTS_model.h            # ML model สำหรับ Pants mode
│
├── Caregiver_Side/              # Caregiver-side firmware
│   ├── Caregiver_Side.ino
│   └── config.h                 # Blynk token + WiFi (ไม่ commit ขึ้น Git)
│
├── ML/                          # Machine Learning pipeline
│   ├── CHEST_mode.ipynb         # AutoGluon training บน SisFall dataset
│   ├── SHIRT_mode.ipynb         # AutoGluon training บน FallAllD (Waist)
│   ├── PANTS_mode.ipynb         # AutoGluon training + 3 gait features
│   ├── Evaluation.ipynb         # Cross-placement comparison
│   ├── ESP32_export.ipynb       # Retrain small RF → export .h headers
│   ├── data/                    # Datasets (ไม่ commit — ขนาดใหญ่)
│   │   ├── SisFall/             # SisFall dataset (200 Hz, chest/waist)
│   │   └── FallAllD/            # FallAllD dataset (40 Hz, multi-device pkl)
│   └── models/                  # ผลการ train
│       ├── CHEST_results.csv
│       ├── SHIRT_results.csv
│       ├── PANTS_results.csv
│       ├── final_report.csv
│       └── esp32/               # .h headers สำหรับ ESP32
│
└── web/                         # Web Dashboard (Next.js)
    ├── app/
    ├── components/
    └── lib/
```

---

## Hardware Pinout — User Device (ESP32-S3)

| Component | Pin | หมายเหตุ |
|---|---|---|
| LED Green | GPIO 4 | โหมด CHEST |
| LED Yellow | GPIO 5 | โหมด SHIRT |
| LED Red | GPIO 6 | โหมด PANTS / Emergency |
| Buzzer | GPIO 7 | แจ้งเตือนเสียง |
| BTN_MODE | GPIO 9 | สลับโหมด CHEST→SHIRT→PANTS |
| BTN_EMERGENCY | GPIO 10 | กด SOS ฉุกเฉิน |
| OLED SDA | GPIO 37 | I2C shared bus |
| OLED SCL | GPIO 38 | I2C shared bus |
| MPU6050 SDA | GPIO 37 | I2C address 0x68 (AD0→GND) |
| MPU6050 SCL | GPIO 38 | I2C address 0x68 (AD0→GND) |
| GPS TX (ESP32 RX) | GPIO 16 | รับ NMEA จาก GPS module |
| GPS RX (ESP32 TX) | GPIO 17 | ส่งคำสั่งไป GPS module |

> OLED (0x3C) และ MPU6050 (0x68) ใช้ I2C bus เดียวกัน ที่ GPIO 37/38

---

## Blynk Virtual Pins

| Pin | ข้อมูล | ประเภท |
|---|---|---|
| V0 | System Status (0=NORMAL, 1=WARNING, 2=FALL) | int |
| V1 | GPS Latitude | float |
| V2 | GPS Longitude | float |
| V3 | Placement Mode (0=CHEST, 1=SHIRT, 2=PANTS) | int |
| V4 | Emergency Active (0/1) | int |

---

## ML Models

### ผลการ Train (AutoGluon)

| Mode | Dataset | Windows | F1 | AUC | Best Model |
|---|---|---|---|---|---|
| CHEST | SisFall | 70,346 | 0.869 | 0.968 | ExtraTreesEntr |
| SHIRT | FallAllD-Waist | 30,708 | 0.849 | 0.976 | WeightedEnsemble_L2 |
| PANTS | FallAllD-Waist | 30,708 | 0.836 | 0.971 | ExtraTreesGini |

### Features

- **23 features (CHEST/SHIRT)**: mean ax/ay/az, std, min/max/range |acc|, RMS, skewness, kurtosis, zero-crossing, SMA, dominant frequency (FFT), spectral energy, correlations xy/yz/xz, max jerk, acc variance
- **+3 features (PANTS)**: step_freq, gait_regularity, vertical_symmetry

### Fall Detection บน ESP32

ใช้ระบบ 2 ชั้นคู่กัน:

```
loop()
├── Threshold State Machine @ 5Hz  (Freefall → Impact → Verify → Emergency)
│     └── FALL_VERIFY: ตรวจทั้ง threshold + ML flag
│
└── ML Buffer @ 50Hz
      ├── circular buffer 100 samples
      ├── ทุก 50 samples → extract 23 features → RandomForest.predict()
      └── ถ้า ML = FALL → triggerEmergency("ML FALL")
```

---

## การติดตั้งและใช้งาน

### 1. ML Models — สร้าง .h headers

```bash
cd ML
# activate venv ที่มี autogluon + micromlgen
source ../venv/bin/activate

# 1. Train full models (optional — ถ้ายังไม่มี)
jupyter notebook CHEST_mode.ipynb   # run all cells
jupyter notebook SHIRT_mode.ipynb
jupyter notebook PANTS_mode.ipynb

# 2. Export small models สำหรับ ESP32
jupyter notebook ESP32_export.ipynb  # run all cells
# → สร้าง models/esp32/CHEST_model.h, SHIRT_model.h, PANTS_model.h

# 3. Copy headers ไปยัง sketch folder
cp models/esp32/*.h ../Full_System_README/
```

### 2. Arduino Libraries

ติดตั้งใน Arduino IDE Library Manager:

| Library | ผู้พัฒนา |
|---|---|
| Blynk | Volodymyr Shymanskyy |
| Adafruit MPU6050 | Adafruit |
| Adafruit Unified Sensor | Adafruit |
| Adafruit GFX Library | Adafruit |
| Adafruit SSD1306 | Adafruit |
| TinyGPS | Mikal Hart |
| ArduinoFFT | Enrique Condes |

### 3. กรอก Credentials

**`Full_System_README/config.h`**
```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxx"
#define BLYNK_TEMPLATE_NAME "ElderlySafetySystem"
#define BLYNK_AUTH_TOKEN    "user_device_token"
#define WIFI_SSID           "YourWiFi"
#define WIFI_PASS           "YourPassword"
```

**`Caregiver_Side/config.h`**
```cpp
#define BLYNK_TEMPLATE_ID   "TMPLxxxxxxxx"   // Template เดียวกัน
#define BLYNK_TEMPLATE_NAME "ElderlySafetySystem"
#define BLYNK_AUTH_TOKEN    "caregiver_device_token"  // คนละ token
#define WIFI_SSID           "YourWiFi"
#define WIFI_PASS           "YourPassword"
```

**`web/.env.local`**
```env
NEXT_PUBLIC_BLYNK_TOKEN=user_device_token
NEXT_PUBLIC_BLYNK_BASE_URL=https://blynk.cloud/external/api
NEXT_PUBLIC_POLL_INTERVAL=2000
```

### 4. Upload Firmware

```
Board: ESP32S3 Dev Module
Upload Speed: 921600
```

Upload `Full_System_README.ino` → User ESP32
Upload `Caregiver_Side.ino` → Caregiver ESP32

### 5. Blynk Automation

ใน blynk.cloud → **Developer Zone → Automations** → สร้าง:
- Trigger: User Device V4 = 1
- Action: Caregiver Device Write V4 = 1

### 6. Web Dashboard

```bash
cd web
npm install        # หรือ pnpm install
npm run dev        # http://localhost:3000
```

**User flow:**
1. เปิดเว็บ → หน้า Health Profile
2. **ลงทะเบียน** → ใส่ชื่อ → ได้รหัส 6 หลัก (จดเก็บไว้)
3. **เข้าสู่ระบบ** → ใส่รหัส 6 หลัก → ดู Dashboard
4. ประวัติการล้มบันทึกแยกตาม user และคงอยู่ข้าม session

---

## การทดสอบ

### Serial Monitor (115200 baud)

```
✓ GPIO configured
✓ EEPROM mode loaded: CHEST CLIP
✓ OLED initialized
✓ MPU6050 initialized (range: ±8g)
⏳ Calibrating MPU6050 (10s)...
✓ GPS initialized
✓ Blynk connected
=== SYSTEM READY — MONITORING ===

[ML] inference every 1s
FALL CONFIRMED — threshold:1 ML:1
🔴 EMERGENCY: ML+THRESH FALL
   Mode: CHEST CLIP
   GPS : 13.847123, 100.523456
```

### จำลองการล้ม
- เขย่า MPU6050 อย่างรวดเร็วแล้วหยุดนิ่ง
- กด `BTN_EMERGENCY` (GPIO 10)
- รอ 3 วินาที verify window

---

## ข้อจำกัด

| ประเด็น | รายละเอียด |
|---|---|
| SHIRT/PANTS dataset | ทั้งคู่ใช้ FallAllD Waist (ไม่มี hip-specific dataset) |
| ML model บน ESP32 | Retrain ด้วย n=20 trees เท่านั้น (ESP32 Flash จำกัด) |
| GPS บนอาคาร | อาจหา satellite ไม่เจอ ให้แสดง 0,0 |
| Blynk Free Plan | จำกัด 2 devices, 10 data points/sec |

---

## License

MIT License
