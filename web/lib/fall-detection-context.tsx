"use client"

import { createContext, useContext, useState, useCallback, useRef, type ReactNode } from "react"

export type SystemStatus = "NORMAL" | "WARNING" | "FALL"
export type MotionActivity = "Walking" | "Sitting" | "Sleeping" | "Near Fall" | "Fallen"
export type EventType = "walk" | "sleep" | "near_fall" | "fall" | "sos"
export type EventStatus = "resolved" | "pending"

export interface HistoryEvent {
  id: string
  type: EventType
  label: string
  time: Date
  status: EventStatus
}

interface FallDetectionState {
  status: SystemStatus
  activity: MotionActivity
  battery: number
  simulationMode: boolean
  emergencyActive: boolean
  caregiverActive: boolean
  emergencyTimer: number
  history: HistoryEvent[]
  setSimulationMode: (v: boolean) => void
  simulateWalk: () => void
  simulateSleep: () => void
  simulateNearFall: () => void
  simulateFall: () => void
  simulateSOS: () => void
  acknowledgeFall: () => void
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

export function FallDetectionProvider({ children }: { children: ReactNode }) {
  const [status, setStatus] = useState<SystemStatus>("NORMAL")
  const [activity, setActivity] = useState<MotionActivity>("Sitting")
  const [battery] = useState(78)
  const [simulationMode, setSimulationMode] = useState(false)
  const [emergencyActive, setEmergencyActive] = useState(false)
  const [caregiverActive, setCaregiverActive] = useState(false)
  const [emergencyTimer, setEmergencyTimer] = useState(0)
  const [history, setHistory] = useState<HistoryEvent[]>([
    {
      id: generateId(),
      type: "walk",
      label: "Walking detected",
      time: new Date(Date.now() - 3600000 * 2),
      status: "resolved",
    },
    {
      id: generateId(),
      type: "sleep",
      label: "Sleeping detected",
      time: new Date(Date.now() - 3600000 * 5),
      status: "resolved",
    },
    {
      id: generateId(),
      type: "near_fall",
      label: "Near fall detected",
      time: new Date(Date.now() - 3600000 * 8),
      status: "resolved",
    },
  ])
  const timerRef = useRef<ReturnType<typeof setInterval> | null>(null)

  const addEvent = useCallback(
    (type: EventType, label: string, eventStatus: EventStatus = "resolved") => {
      setHistory((prev) => [
        {
          id: generateId(),
          type,
          label,
          time: new Date(),
          status: eventStatus,
        },
        ...prev,
      ])
    },
    []
  )

  const startEmergencyTimer = useCallback(() => {
    setEmergencyTimer(0)
    if (timerRef.current) clearInterval(timerRef.current)
    timerRef.current = setInterval(() => {
      setEmergencyTimer((prev) => prev + 1)
    }, 1000)
  }, [])

  const stopEmergencyTimer = useCallback(() => {
    if (timerRef.current) {
      clearInterval(timerRef.current)
      timerRef.current = null
    }
  }, [])

  const simulateWalk = useCallback(() => {
    setStatus("NORMAL")
    setActivity("Walking")
    setEmergencyActive(false)
    setCaregiverActive(false)
    stopEmergencyTimer()
    addEvent("walk", "Walking detected")
  }, [addEvent, stopEmergencyTimer])

  const simulateSleep = useCallback(() => {
    setStatus("NORMAL")
    setActivity("Sleeping")
    setEmergencyActive(false)
    setCaregiverActive(false)
    stopEmergencyTimer()
    addEvent("sleep", "Sleeping detected")
  }, [addEvent, stopEmergencyTimer])

  const simulateNearFall = useCallback(() => {
    setStatus("WARNING")
    setActivity("Near Fall")
    setEmergencyActive(false)
    setCaregiverActive(false)
    stopEmergencyTimer()
    addEvent("near_fall", "Near fall detected", "pending")
  }, [addEvent, stopEmergencyTimer])

  const simulateFall = useCallback(() => {
    setStatus("FALL")
    setActivity("Fallen")
    setEmergencyActive(true)
    setCaregiverActive(false)
    startEmergencyTimer()
    addEvent("fall", "Fall detected", "pending")
  }, [addEvent, startEmergencyTimer])

  const simulateSOS = useCallback(() => {
    setStatus("FALL")
    setActivity("Fallen")
    setEmergencyActive(true)
    setCaregiverActive(false)
    startEmergencyTimer()
    addEvent("sos", "SOS activated", "pending")
  }, [addEvent, startEmergencyTimer])

  const acknowledgeFall = useCallback(() => {
    stopEmergencyTimer()
    setEmergencyActive(false)
    setCaregiverActive(true)
  }, [stopEmergencyTimer])

  const resolveEmergency = useCallback(() => {
    setCaregiverActive(false)
    setStatus("NORMAL")
    setActivity("Sitting")
    setHistory((prev) =>
      prev.map((e) =>
        e.status === "pending" ? { ...e, status: "resolved" as EventStatus } : e
      )
    )
  }, [])

  return (
    <FallDetectionContext.Provider
      value={{
        status,
        activity,
        battery,
        simulationMode,
        emergencyActive,
        caregiverActive,
        emergencyTimer,
        history,
        setSimulationMode,
        simulateWalk,
        simulateSleep,
        simulateNearFall,
        simulateFall,
        simulateSOS,
        acknowledgeFall,
        resolveEmergency,
      }}
    >
      {children}
    </FallDetectionContext.Provider>
  )
}
