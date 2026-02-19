"use client"

import { useFallDetection } from "@/lib/fall-detection-context"
import { Button } from "@/components/ui/button"
import { CircleAlert, Phone, ShieldAlert, MapPin } from "lucide-react"

export function EmergencyScreen() {
  const { emergencyTimer, acknowledgeFall } = useFallDetection()

  const formatTime = (seconds: number) => {
    const mins = Math.floor(seconds / 60)
    const secs = seconds % 60
    return `${mins.toString().padStart(2, "0")}:${secs.toString().padStart(2, "0")}`
  }

  return (
    <div className="fixed inset-0 z-50 flex flex-col bg-destructive">
      {/* Ambient glow overlay */}
      <div className="absolute inset-0 bg-[radial-gradient(circle_at_50%_30%,oklch(0.7_0.2_22/0.3),transparent_70%)]" />

      <div className="relative flex flex-1 flex-col items-center justify-center px-6">
        <div className="flex w-full max-w-sm flex-col items-center gap-10 text-center">
          {/* Pulsing icon with multiple rings */}
          <div className="relative flex items-center justify-center">
            <div className="absolute size-32 rounded-full bg-card/10 animate-pulse-ring" />
            <div className="absolute size-32 rounded-full bg-card/5 animate-pulse-ring-delayed" />
            <div className="absolute size-28 rounded-full border border-card/10 animate-ambient-breathe" />
            <div className="relative flex size-24 items-center justify-center rounded-full bg-card/15 backdrop-blur-md ring-1 ring-card/20">
              <ShieldAlert className="size-11 text-card" />
            </div>
          </div>

          {/* Alert text */}
          <div className="flex flex-col gap-3">
            <p className="text-xs font-semibold uppercase tracking-[0.25em] text-card/50">Emergency Alert</p>
            <h1 className="text-4xl font-bold tracking-tight text-card font-mono sm:text-5xl">
              FALL DETECTED
            </h1>
            <p className="text-base leading-relaxed text-card/70">
              Emergency services will be contacted automatically
            </p>
          </div>

          {/* Timer */}
          <div className="flex flex-col items-center gap-2 rounded-3xl bg-card/10 px-8 py-5 backdrop-blur-sm ring-1 ring-card/10">
            <p className="text-[10px] font-semibold uppercase tracking-[0.2em] text-card/40">Elapsed Time</p>
            <p className="font-mono text-5xl font-bold tabular-nums text-card tracking-tight sm:text-6xl">
              {formatTime(emergencyTimer)}
            </p>
          </div>

          {/* Location */}
          <div className="flex items-center gap-2 text-card/50">
            <MapPin className="size-3.5" />
            <span className="text-xs font-medium">Home - Living Room</span>
          </div>

          {/* Actions */}
          <div className="flex w-full flex-col gap-3">
            <Button
              size="lg"
              className="h-14 rounded-2xl bg-card text-destructive text-base font-bold hover:bg-card/90 shadow-2xl shadow-card/20 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
              onClick={acknowledgeFall}
            >
              <CircleAlert className="mr-2 size-5" />
              I'm Okay - Acknowledge
            </Button>
            <Button
              size="lg"
              variant="outline"
              className="h-14 rounded-2xl border-2 border-card/20 bg-card/5 text-card text-base font-semibold hover:bg-card/10 hover:text-card backdrop-blur-sm transition-all duration-300"
            >
              <Phone className="mr-2 size-5" />
              Call Emergency
            </Button>
          </div>
        </div>
      </div>

      {/* Footer */}
      <div className="relative px-6 pb-8 pt-4 text-center">
        <p className="text-[10px] tracking-wider text-card/30 uppercase">SafeStep Fall Detection System</p>
      </div>
    </div>
  )
}
