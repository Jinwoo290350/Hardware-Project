// Blynk REST API configuration
// ค่าเหล่านี้อ่านมาจาก .env.local  (NEXT_PUBLIC_ = ใช้ได้ใน browser)

export const BLYNK_TOKEN    = process.env.NEXT_PUBLIC_BLYNK_TOKEN    ?? ""
export const BLYNK_BASE_URL = process.env.NEXT_PUBLIC_BLYNK_BASE_URL ?? "https://blynk.cloud/external/api"
export const POLL_INTERVAL  = Number(process.env.NEXT_PUBLIC_POLL_INTERVAL ?? "2000")

// Virtual pin names (ตรงกับ Arduino)
export const PIN_STATUS    = "V0"  // 0=NORMAL  1=WARNING  2=FALL
export const PIN_LAT       = "V1"  // GPS Latitude  (float)
export const PIN_LON       = "V2"  // GPS Longitude (float)
export const PIN_MODE      = "V3"  // 0=CHEST  1=SHIRT  2=PANTS
export const PIN_EMERGENCY = "V4"  // Emergency active 0/1

export type DeviceMode = "CHEST" | "SHIRT" | "PANTS"

export const MODE_NAMES: Record<number, DeviceMode> = {
  0: "CHEST",
  1: "SHIRT",
  2: "PANTS",
}

// ดึงข้อมูลจาก Blynk ทุก pin ในครั้งเดียว
export async function fetchBlynkState(): Promise<{
  status: number
  lat: number
  lon: number
  mode: number
  emergency: number
} | null> {
  if (!BLYNK_TOKEN || BLYNK_TOKEN === "YOUR_USER_DEVICE_AUTH_TOKEN") return null

  try {
    const url = `${BLYNK_BASE_URL}/get?token=${BLYNK_TOKEN}&${PIN_STATUS}&${PIN_LAT}&${PIN_LON}&${PIN_MODE}&${PIN_EMERGENCY}`
    const res = await fetch(url, { cache: "no-store" })
    if (!res.ok) return null

    const data = await res.json()

    // Blynk API returns { "V0": ["0"], "V1": ["13.79"], ... }
    const parse = (key: string, fallback = 0) => {
      const v = data[key]
      if (Array.isArray(v)) return Number(v[0] ?? fallback)
      return Number(v ?? fallback)
    }

    return {
      status:    parse(PIN_STATUS),
      lat:       parse(PIN_LAT),
      lon:       parse(PIN_LON),
      mode:      parse(PIN_MODE),
      emergency: parse(PIN_EMERGENCY),
    }
  } catch {
    return null
  }
}
