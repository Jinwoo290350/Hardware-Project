"use client"

import { useState } from "react"
import {
  FallDetectionProvider,
  useFallDetection,
} from "@/lib/fall-detection-context"
import { DashboardScreen } from "@/components/dashboard-screen"
import { SimulationPanel } from "@/components/simulation-panel"
import { HistoryScreen } from "@/components/history-screen"
import { EmergencyScreen } from "@/components/emergency-screen"
import { CaregiverResponse } from "@/components/caregiver-response"
import {
  LayoutDashboard,
  FlaskConical,
  History,
  ShieldCheck,
  Wifi,
} from "lucide-react"

type Tab = "dashboard" | "simulation" | "history"

const tabs: { id: Tab; label: string; icon: typeof LayoutDashboard }[] = [
  { id: "dashboard", label: "Dashboard", icon: LayoutDashboard },
  { id: "simulation", label: "Simulate", icon: FlaskConical },
  { id: "history", label: "History", icon: History },
]

function AppShell() {
  const [activeTab, setActiveTab] = useState<Tab>("dashboard")
  const { emergencyActive, caregiverActive, status } = useFallDetection()

  if (emergencyActive) return <EmergencyScreen />
  if (caregiverActive) return <CaregiverResponse />

  return (
    <div className="flex min-h-dvh flex-col bg-background">
      {/* Premium glassmorphism header */}
      <header className="sticky top-0 z-40 border-b border-border/50 glass-panel-strong">
        <div className="mx-auto flex h-16 max-w-lg items-center justify-between px-5">
          <div className="flex items-center gap-3">
            <div className="relative flex size-10 items-center justify-center rounded-2xl bg-primary shadow-lg shadow-primary/20">
              <ShieldCheck className="size-5 text-primary-foreground" />
              {/* Ambient glow behind logo */}
              <div className="absolute -inset-1 -z-10 rounded-2xl bg-primary/20 blur-md animate-ambient-breathe" />
            </div>
            <div className="flex flex-col">
              <h1 className="text-base font-bold tracking-tight text-foreground font-mono">
                SafeStep
              </h1>
              <p className="text-[10px] leading-none text-muted-foreground tracking-wide uppercase">
                Fall Detection
              </p>
            </div>
          </div>
          <div className="flex items-center gap-2">
            <div className="flex items-center gap-1.5 rounded-full bg-success/10 px-2.5 py-1">
              <span className="relative flex size-1.5">
                <span className="absolute inline-flex size-full animate-ping rounded-full bg-success opacity-75" />
                <span className="relative inline-flex size-1.5 rounded-full bg-success" />
              </span>
              <span className="text-[10px] font-semibold text-success tracking-wide">LIVE</span>
            </div>
            <div className="flex size-9 items-center justify-center rounded-xl bg-secondary">
              <Wifi className="size-4 text-muted-foreground" />
            </div>
          </div>
        </div>
      </header>

      <main className="mx-auto w-full max-w-lg flex-1 px-4 pt-5 pb-2">
        <div className="animate-fade-in-up">
          {activeTab === "dashboard" && <DashboardScreen />}
          {activeTab === "simulation" && <SimulationPanel />}
          {activeTab === "history" && <HistoryScreen />}
        </div>
      </main>

      {/* Premium glassmorphism nav */}
      <nav
        className="sticky bottom-0 z-40 border-t border-border/50 glass-panel-strong"
        aria-label="Main navigation"
      >
        <div className="mx-auto flex h-[72px] max-w-lg items-center justify-around px-4">
          {tabs.map(({ id, label, icon: Icon }) => {
            const isActive = activeTab === id
            return (
              <button
                key={id}
                onClick={() => setActiveTab(id)}
                className={`relative flex flex-col items-center gap-1.5 rounded-2xl px-5 py-2 transition-all duration-300 ${
                  isActive
                    ? "text-primary"
                    : "text-muted-foreground hover:text-foreground"
                }`}
                aria-label={label}
                aria-current={isActive ? "page" : undefined}
              >
                {isActive && (
                  <div className="absolute inset-0 rounded-2xl bg-primary/8" />
                )}
                <Icon className={`relative size-5 transition-transform duration-300 ${isActive ? "scale-110" : ""}`} />
                <span className={`relative text-[10px] font-semibold tracking-wide ${isActive ? "" : ""}`}>
                  {label}
                </span>
                {isActive && (
                  <span className="absolute -top-0.5 left-1/2 -translate-x-1/2 h-0.5 w-8 rounded-full bg-primary shadow-sm shadow-primary/50" />
                )}
              </button>
            )
          })}
        </div>
      </nav>
    </div>
  )
}

export default function Page() {
  return (
    <FallDetectionProvider>
      <AppShell />
    </FallDetectionProvider>
  )
}
