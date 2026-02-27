"use client"

import {
  createContext,
  useContext,
  useState,
  useCallback,
  useRef,
  useEffect,
  type ReactNode,
} from "react"
import {
  fetchBlynkState,
  MODE_NAMES,
  type DeviceMode,
  POLL_INTERVAL,
  BLYNK_TOKEN,
} from "@/lib/blynk-config"
import {
  saveUserHistory,
  loadUserHistory,
  type StoredEvent,
} from "@/lib/user-store"

export type SystemStatus   = "NORMAL" | "WARNING" | "FALL"
export type MotionActivity = "Walking" | "Sitting" | "Sleeping" | "Near Fall" | "Fallen"
export type EventType      = "walk" | "sleep" | "near_fall" | "fall" | "sos"
export type EventStatus    = "resolved" | "pending"

export interface HistoryEvent {
  id:     string
  type:   EventType
  label:  string
  time:   Date
  status: EventStatus
}

interface FallDetectionState {
  status:           SystemStatus
  activity:         MotionActivity
  emergencyActive:  boolean
  caregiverActive:  boolean
  emergencyTimer:   number
  history:          HistoryEvent[]
  deviceMode:       DeviceMode
  gpsLat:           number
  gpsLon:           number
  connected:        boolean
  acknowledgeFall:  () => void
  resolveEmergency: () => void
}

const FallDetectionContext = createContext<FallDetectionState | null>(null)

export function useFallDetection() {
  const ctx = useContext(FallDetectionContext)
  if (!ctx) throw new Error("useFallDetection must be used within FallDetectionProvider")
  return ctx
}

function generateId() {
  return Math.random().toString(36).substring(2, 9)
}

function serializeHistory(history: HistoryEvent[]): StoredEvent[] {
  return history.map(e => ({ ...e, time: e.time.toISOString() }))
}

function deserializeHistory(stored: StoredEvent[]): HistoryEvent[] {
  return stored.map(e => ({
    ...e,
    type:   e.type   as EventType,
    status: e.status as EventStatus,
    time:   new Date(e.time),
  }))
}

export function FallDetectionProvider({
  children,
  userCode,
}: {
  children: ReactNode
  userCode: string
}) {
  const [status,          setStatus]          = useState<SystemStatus>("NORMAL")
  const [activity,        setActivity]        = useState<MotionActivity>("Sitting")
  const [emergencyActive, setEmergencyActive] = useState(false)
  const [caregiverActive, setCaregiverActive] = useState(false)
  const [emergencyTimer,  setEmergencyTimer]  = useState(0)
  const [deviceMode,      setDeviceMode]      = useState<DeviceMode>("CHEST")
  const [gpsLat,          setGpsLat]          = useState(0)
  const [gpsLon,          setGpsLon]          = useState(0)
  const [connected,       setConnected]       = useState(false)
  const [history,         setHistory]         = useState<HistoryEvent[]>([])

  const timerRef           = useRef<ReturnType<typeof setInterval> | null>(null)
  const prevStatusRef      = useRef<number>(0)
  const emergencyActiveRef = useRef(false)

  // ── Load user history from localStorage on mount ──────────
  useEffect(() => {
    const stored = loadUserHistory(userCode)
    if (stored.length > 0) {
      setHistory(deserializeHistory(stored))
    }
  }, [userCode])

  // ── Persist history to localStorage whenever it changes ───
  useEffect(() => {
    saveUserHistory(userCode, serializeHistory(history))
  }, [history, userCode])

  // ── Emergency timer ───────────────────────────────────────
  const startEmergencyTimer = useCallback(() => {
    setEmergencyTimer(0)
    if (timerRef.current) clearInterval(timerRef.current)
    timerRef.current = setInterval(() => {
      setEmergencyTimer(p => p + 1)
    }, 1000)
  }, [])

  const stopEmergencyTimer = useCallback(() => {
    if (timerRef.current) {
      clearInterval(timerRef.current)
      timerRef.current = null
    }
  }, [])

  // ── Add event to history ──────────────────────────────────
  const addEvent = useCallback(
    (type: EventType, label: string, eventStatus: EventStatus = "resolved") => {
      setHistory(prev => [
        { id: generateId(), type, label, time: new Date(), status: eventStatus },
        ...prev,
      ])
    },
    []
  )

  // ── Blynk polling ─────────────────────────────────────────
  useEffect(() => {
    if (!BLYNK_TOKEN || BLYNK_TOKEN === "YOUR_USER_DEVICE_AUTH_TOKEN") {
      setConnected(false)
      return
    }

    const poll = async () => {
      const data = await fetchBlynkState()

      if (!data) {
        setConnected(false)
        return
      }

      setConnected(true)
      setDeviceMode(MODE_NAMES[data.mode] ?? "CHEST")
      setGpsLat(data.lat)
      setGpsLon(data.lon)

      if (data.emergency === 1 || data.status === 2) {
        if (prevStatusRef.current !== 2) {
          prevStatusRef.current = 2
          emergencyActiveRef.current = true
          setStatus("FALL")
          setActivity("Fallen")
          setEmergencyActive(true)
          setCaregiverActive(false)
          startEmergencyTimer()
          addEvent("fall", "Fall detected by device", "pending")
        }
      } else if (data.status === 1) {
        if (prevStatusRef.current !== 1) {
          prevStatusRef.current = 1
          setStatus("WARNING")
          setActivity("Near Fall")
          if (emergencyActiveRef.current) {
            emergencyActiveRef.current = false
            setEmergencyActive(false)
            stopEmergencyTimer()
          }
          addEvent("near_fall", "Near fall detected", "pending")
        }
      } else {
        if (prevStatusRef.current !== 0) {
          prevStatusRef.current = 0
          setStatus("NORMAL")
          setActivity("Walking")
          if (emergencyActiveRef.current) {
            emergencyActiveRef.current = false
            setEmergencyActive(false)
            stopEmergencyTimer()
            addEvent("walk", "Device back to normal")
          }
        }
      }
    }

    poll()
    const interval = setInterval(poll, POLL_INTERVAL)
    return () => clearInterval(interval)
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])

  // ── Acknowledge ───────────────────────────────────────────
  const acknowledgeFall = useCallback(() => {
    stopEmergencyTimer()
    emergencyActiveRef.current = false
    setEmergencyActive(false)
    setCaregiverActive(true)
  }, [stopEmergencyTimer])

  // ── Resolve ───────────────────────────────────────────────
  const resolveEmergency = useCallback(() => {
    setCaregiverActive(false)
    setStatus("NORMAL")
    setActivity("Sitting")
    prevStatusRef.current = 0
    setHistory(prev =>
      prev.map(e =>
        e.status === "pending" ? { ...e, status: "resolved" as EventStatus } : e
      )
    )
  }, [])

  return (
    <FallDetectionContext.Provider
      value={{
        status,
        activity,
        emergencyActive,
        caregiverActive,
        emergencyTimer,
        history,
        deviceMode,
        gpsLat,
        gpsLon,
        connected,
        acknowledgeFall,
        resolveEmergency,
      }}
    >
      {children}
    </FallDetectionContext.Provider>
  )
}
