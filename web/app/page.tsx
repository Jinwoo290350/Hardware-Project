"use client"

import { useState, useEffect } from "react"
import {
  FallDetectionProvider,
  useFallDetection,
} from "@/lib/fall-detection-context"
import { DashboardScreen }  from "@/components/dashboard-screen"
import { HistoryScreen }    from "@/components/history-screen"
import { EmergencyScreen }  from "@/components/emergency-screen"
import { CaregiverResponse } from "@/components/caregiver-response"
import { AuthScreen }       from "@/components/auth-screen"
import {
  getSession,
  loginUser,
  clearSession,
  setSession,
  getUserName,
} from "@/lib/user-store"
import {
  LayoutDashboard,
  History,
  ShieldCheck,
  Wifi,
  WifiOff,
  LogOut,
  User,
} from "lucide-react"

type Tab = "dashboard" | "history"

const tabs: { id: Tab; label: string; icon: typeof LayoutDashboard }[] = [
  { id: "dashboard", label: "Dashboard", icon: LayoutDashboard },
  { id: "history",   label: "History",   icon: History },
]

// ── AppShell ─────────────────────────────────────────────────
function AppShell({
  onLogout,
  userName,
  userCode,
}: {
  onLogout: () => void
  userName: string
  userCode: string
}) {
  const [activeTab, setActiveTab] = useState<Tab>("dashboard")
  const { emergencyActive, caregiverActive, connected } = useFallDetection()

  if (emergencyActive)  return <EmergencyScreen />
  if (caregiverActive)  return <CaregiverResponse />

  return (
    <div className="flex min-h-dvh flex-col bg-background">
      {/* Header */}
      <header className="sticky top-0 z-40 border-b border-border/50 glass-panel-strong">
        <div className="mx-auto flex h-16 max-w-lg items-center justify-between px-5">
          {/* Left: Logo */}
          <div className="flex items-center gap-3">
            <div className="relative flex size-10 items-center justify-center rounded-2xl bg-primary shadow-lg shadow-primary/20">
              <ShieldCheck className="size-5 text-primary-foreground" />
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

          {/* Right: Live indicator + User + Logout */}
          <div className="flex items-center gap-2">
            {/* Live indicator */}
            <div className={`flex items-center gap-1.5 rounded-full px-2.5 py-1 ${connected ? "bg-success/10" : "bg-muted"}`}>
              <span className="relative flex size-1.5">
                {connected && (
                  <span className="absolute inline-flex size-full animate-ping rounded-full bg-success opacity-75" />
                )}
                <span className={`relative inline-flex size-1.5 rounded-full ${connected ? "bg-success" : "bg-muted-foreground"}`} />
              </span>
              <span className={`text-[10px] font-semibold tracking-wide ${connected ? "text-success" : "text-muted-foreground"}`}>
                {connected ? "LIVE" : "OFFLINE"}
              </span>
            </div>

            {/* WiFi icon */}
            <div className="flex size-9 items-center justify-center rounded-xl bg-secondary">
              {connected
                ? <Wifi    className="size-4 text-muted-foreground" />
                : <WifiOff className="size-4 text-muted-foreground" />
              }
            </div>

            {/* User chip */}
            <div
              className="flex items-center gap-1.5 rounded-xl bg-secondary px-2.5 py-1.5 cursor-default"
              title={`รหัส: ${userCode}`}
            >
              <User className="size-3.5 text-muted-foreground" />
              <span className="text-[11px] font-semibold text-foreground max-w-15 truncate">
                {userName}
              </span>
            </div>

            {/* Logout */}
            <button
              onClick={onLogout}
              className="flex size-9 items-center justify-center rounded-xl bg-secondary text-muted-foreground hover:text-destructive hover:bg-destructive/10 transition-colors"
              title="ออกจากระบบ"
            >
              <LogOut className="size-4" />
            </button>
          </div>
        </div>
      </header>

      <main className="mx-auto w-full max-w-lg flex-1 px-4 pt-5 pb-2">
        <div className="animate-fade-in-up">
          {activeTab === "dashboard" && <DashboardScreen />}
          {activeTab === "history"   && <HistoryScreen />}
        </div>
      </main>

      {/* Bottom nav */}
      <nav
        className="sticky bottom-0 z-40 border-t border-border/50 glass-panel-strong"
        aria-label="Main navigation"
      >
        <div className="mx-auto flex h-18 max-w-lg items-center justify-around px-4">
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
                <Icon
                  className={`relative size-5 transition-transform duration-300 ${isActive ? "scale-110" : ""}`}
                />
                <span className="relative text-[10px] font-semibold tracking-wide">
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

// ── Page ─────────────────────────────────────────────────────
export default function Page() {
  const [userCode, setUserCode] = useState<string | null>(null)
  const [userName, setUserName] = useState<string>("")
  const [loading,  setLoading]  = useState(true)

  // Check stored session on client mount
  useEffect(() => {
    const code = getSession()
    if (code && loginUser(code)) {
      setUserCode(code)
      setUserName(getUserName(code))
    }
    setLoading(false)
  }, [])

  function handleAuth(code: string, name: string) {
    setSession(code)
    setUserCode(code)
    setUserName(name)
  }

  function handleLogout() {
    clearSession()
    setUserCode(null)
    setUserName("")
  }

  // Prevent hydration flash — wait for client to read localStorage
  if (loading) return null

  if (!userCode) {
    return <AuthScreen onAuth={handleAuth} />
  }

  return (
    <FallDetectionProvider userCode={userCode}>
      <AppShell onLogout={handleLogout} userName={userName} userCode={userCode} />
    </FallDetectionProvider>
  )
}
