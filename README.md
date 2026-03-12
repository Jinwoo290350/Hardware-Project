# SafeStep — ระบบแจ้งเตือนการล้มสำหรับผู้สูงอายุด้วยปัญญาประดิษฐ์
### AI-Powered Fall Detection & Emergency Alert System for the Elderly

---

## ข้อมูลโครงการ

| | |
|---|---|
| **วิชา** | โครงงานวิศวกรรมคอมพิวเตอร์ |
| **ภาควิชา** | วิศวกรรมคอมพิวเตอร์ |
| **คณะ** | วิศวกรรมศาสตร์ |
| **มหาวิทยาลัย** | มหาวิทยาลัยเกษตรศาสตร์ (Kasetsart University) |
| **ปีการศึกษา** | 2568 |

---

## ผู้จัดทำ

| ชื่อ-นามสกุล | รหัสนักศึกษา |
|---|---|
| วีรพงษ์ ฮะภูริวัฒน์ | 6810503943 |
| ปณิธิ โลภาส | 6810503706 |
| ศิวกร แพรกปาน | 6810503986 |
| ภัทรวัฒน์ ตันหัน | 6810503820 |

---

## ภาพรวมระบบ

ระบบตรวจจับการล้มและแจ้งเตือนฉุกเฉินสำหรับผู้สูงอายุแบบ Real-time
ใช้ **ESP32-S3 + MPU6050 + GPS + Machine Learning** ตรวจจับการล้ม
และส่งแจ้งเตือนผ่าน **Blynk IoT** ไปยังผู้ดูแลและ **Web Dashboard** ทันที

```
[User ESP32-S3]  ──Blynk──►  [Caregiver ESP32]
  MPU6050                       LED + Buzzer
  GPS Module                         │
  OLED Display                       ▼
  ML Model (RF)              [Blynk Cloud]
  Threshold SM                       │
       │                             ▼
       └──────────────────►  [Web Dashboard]
                              Next.js + Auth
                              Leaflet Map
```

| Component | เทคโนโลยี |
|---|---|
| User Device | ESP32-S3, MPU6050, GPS, OLED SSD1306 |
| Caregiver Device | ESP32, LED (RGB), Buzzer, LDR |
| Cloud | Blynk IoT (Free plan) |
| Web Dashboard | Next.js 15, TypeScript, Tailwind CSS, Leaflet |
| ML (Training) | Python, AutoGluon, scikit-learn, micromlgen |
| ML (Inference) | Random Forest 20 trees บน ESP32 (< 500 KB) |

---

## โครงสร้างไฟล์

```
Hardware-Project/
├── Full_System_README/
│   ├── Full_System_README.ino   ← Firmware หลัก (User Device ESP32-S3)
│   ├── config.h                 ← WiFi / Blynk token (ไม่รวมใน git)
│   ├── CHEST_model.h            ← ML model สำหรับโหมด Chest Clip
│   ├── SHIRT_model.h            ← ML model สำหรับโหมด Shirt Pocket
│   └── PANTS_model.h            ← ML model สำหรับโหมด Pants Pocket
│
├── Caregiver_Side/
│   ├── Caregiver_Side.ino       ← Firmware ฝั่งผู้ดูแล (Caregiver ESP32)
│   └── config.h                 ← WiFi / Blynk token (ไม่รวมใน git)
│
├── ML/
│   ├── CHEST_mode.ipynb         ← วิเคราะห์และ train model โหมด CHEST
│   ├── SHIRT_mode.ipynb         ← วิเคราะห์และ train model โหมด SHIRT
│   ├── PANTS_mode.ipynb         ← วิเคราะห์และ train model โหมด PANTS
│   ├── Evaluation.ipynb         ← เปรียบเทียบประสิทธิภาพ model ทุกโหมด
│   ├── ESP32_export.ipynb       ← Export RF model → C++ header (.h)
│   ├── RealData_Model.ipynb     ← Train จากข้อมูล real-world
│   └── models/esp32/            ← ไฟล์ .h ที่ export แล้ว
│
├── DataLogger/                  ← Arduino sketch บันทึกข้อมูล MPU6050
├── data_lable/                  ← ข้อมูลที่บันทึกและ label แล้ว
│
└── web/                         ← Web Dashboard (Next.js)
    ├── app/                     ← Pages (layout, page)
    ├── components/              ← UI components
    │   ├── dashboard-screen.tsx ← หน้า Dashboard หลัก + Map
    │   ├── history-screen.tsx   ← ประวัติเหตุการณ์
    │   ├── emergency-screen.tsx ← หน้าฉุกเฉิน
    │   ├── auth-screen.tsx      ← Register / Login (รหัส 6 หลัก)
    │   └── map-card.tsx         ← แผนที่ GPS (Leaflet + OpenStreetMap)
    └── lib/
        ├── fall-detection-context.tsx  ← State management + Blynk polling
        ├── blynk-config.ts             ← Blynk REST API config
        └── user-store.ts               ← Auth ด้วย localStorage
```

---

## Hardware

### User Device (ESP32-S3)

| ส่วนประกอบ | GPIO |
|---|---|
| SDA (OLED + MPU6050) | GPIO 8 |
| SCL (OLED + MPU6050) | GPIO 9 |
| LED เขียว | GPIO 4 |
| LED เหลือง | GPIO 5 |
| LED แดง | GPIO 6 |
| Buzzer | GPIO 7 |
| BTN_MODE (เลือกโหมด) | GPIO 10 |
| BTN_EMERGENCY (SOS) | GPIO 13 |
| GPS RX | GPIO 16 |
| GPS TX | GPIO 17 |

### Caregiver Device (ESP32)

| ส่วนประกอบ | GPIO |
|---|---|
| LED เขียว | GPIO 40 |
| LED เหลือง | GPIO 41 |
| LED แดง | GPIO 42 |
| Buzzer | GPIO 15 |
| Switch (Acknowledge) | GPIO 2 |
| LDR (แสงสว่าง) | GPIO 4 |

---

## Blynk Virtual Pins

| Pin | ข้อมูล | ประเภท |
|---|---|---|
| V0 | System Status (0=NORMAL, 1=WARNING, 2=FALL) | int |
| V1 | GPS Latitude | float |
| V2 | GPS Longitude | float |
| V3 | Mode (0=CHEST, 1=SHIRT, 2=PANTS) | int |
| V4 | Emergency Flag (0/1) | int |

---

## ML Model

| โหมด | Dataset | Features | F1 Score (Small RF) |
|---|---|---|---|
| CHEST | SisFall | 23 | 0.764 |
| SHIRT | FallAllD (Waist) | 23 | 0.687 |
| PANTS | FallAllD (Waist) | 26 | 0.685 |

**Architecture:** RandomForestClassifier (n=20, max_depth=10, max_leaf_nodes=80)
**Export:** micromlgen → C++ if/else header < 500 KB ต่อโหมด

---

## การติดตั้งและใช้งาน

### 1. Arduino Firmware

```bash
# ติดตั้ง Libraries ที่ต้องการใน Arduino IDE
- Blynk by Volodymyr Shymanskyy
- Adafruit MPU6050
- Adafruit SSD1306 + Adafruit GFX
- TinyGPS by Mikal Hart
- ArduinoFFT by Enrique Condes

# กรอกข้อมูลใน config.h ทั้งสองเครื่อง
#define BLYNK_TEMPLATE_ID   "..."
#define BLYNK_AUTH_TOKEN    "..."
#define WIFI_SSID           "..."
#define WIFI_PASS           "..."
```

### 2. Web Dashboard

```bash
cd web
pnpm install
# สร้างไฟล์ .env.local
echo "NEXT_PUBLIC_BLYNK_TOKEN=your_token_here" > .env.local
pnpm dev
```

### 3. Blynk Setup

1. สมัคร [blynk.cloud](https://blynk.cloud)
2. สร้าง Template ชื่อ `ElderlySafetySystem`
3. เพิ่ม Datastreams: V0 (int), V1 (double), V2 (double), V3 (int), V4 (int)
4. สร้าง 2 Devices: User Device + Caregiver Device
5. ตั้ง Automation: `When UserDevice V4=1 → Write CaregiverDevice V4=1`

### 4. Export ML Models (ถ้าต้องการ retrain)

```bash
source venv/bin/activate
jupyter notebook ML/ESP32_export.ipynb
# Run All Cells → ได้ไฟล์ใน ML/models/esp32/
cp ML/models/esp32/*.h Full_System_README/
```

---

## การทำงานของระบบ

```
เปิดเครื่อง
    │
    ├─ Calibrate MPU6050 (10 วินาที)
    ├─ Connect WiFi + Blynk
    ▼
MONITORING ACTIVE
    │
    ├─ [MPU6050 @ 5Hz] Threshold State Machine
    │       Freefall → Impact → Verify → FALL
    │       High-Jerk → Verify → FALL
    │
    ├─ [MPU6050 @ 50Hz] ML Inference (ทุก 1 วินาที)
    │       Random Forest → pred=1 (fall) x2 → FALL
    │
    ├─ [BTN10] เปลี่ยนโหมด CHEST/SHIRT/PANTS
    ├─ [BTN13] Manual SOS
    │
    ▼ เมื่อ FALL
    ├─ Blynk V0=2, V4=1 → Caregiver รับแจ้งเตือน
    ├─ GPS ส่งพิกัด (V1, V2)
    ├─ Web Dashboard แสดง Emergency + Map
    └─ กด Acknowledge → ล้าง emergency
```

---

## License

MIT License — ดูรายละเอียดใน [LICENSE](LICENSE)

---

*SafeStep — Elderly Safety System | คณะวิศวกรรมศาสตร์ มหาวิทยาลัยเกษตรศาสตร์ 2567*
