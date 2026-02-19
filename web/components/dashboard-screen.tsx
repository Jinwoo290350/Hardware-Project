"use client"

import { useFallDetection } from "@/lib/fall-detection-context"
import { Card, CardContent } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import {
  ShieldCheck,
  AlertTriangle,
  CircleAlert,
  BatteryMedium,
  BatteryFull,
  BatteryLow,
  MapPin,
  User,
  Heart,
} from "lucide-react"
import { MotionAnimation } from "@/components/motion-animation"

/* ---- Hero: Large centered motion animation with ambient glow ---- */
function MotionHero() {
  const { activity, status } = useFallDetection()

  const labelConfig: Record<string, { label: string; sublabel: string; color: string; glowClass: string }> = {
    Walking: { label: "Walking", sublabel: "Active movement detected", color: "text-success", glowClass: "bg-success/15" },
    Sitting: { label: "Sitting", sublabel: "Resting position detected", color: "text-primary", glowClass: "bg-primary/15" },
    Sleeping: { label: "Sleeping", sublabel: "Sleep cycle in progress", color: "text-primary", glowClass: "bg-primary/15" },
    "Near Fall": { label: "Near Fall", sublabel: "Instability detected", color: "text-warning-foreground", glowClass: "bg-warning/20" },
    Fallen: { label: "Fall Detected", sublabel: "Emergency protocol active", color: "text-destructive", glowClass: "bg-destructive/15" },
  }

  const { label, sublabel, color, glowClass } = labelConfig[activity]

  return (
    <Card className="relative overflow-hidden border-0 shadow-lg shadow-primary/5">
      {/* Ambient background glow */}
      <div className={`absolute inset-0 ${glowClass} animate-ambient-breathe`} />
      <div className="absolute inset-0 bg-card/60 backdrop-blur-sm" />

      <CardContent className="relative flex flex-col items-center gap-5 py-8 sm:py-10">
        {/* Status pill */}
        <div className="flex items-center gap-2 rounded-full bg-secondary/80 px-3 py-1 backdrop-blur-sm">
          <span className="relative flex size-2">
            <span className={`absolute inline-flex size-full animate-ping rounded-full ${status === "NORMAL" ? "bg-success" : status === "WARNING" ? "bg-warning" : "bg-destructive"} opacity-75`} />
            <span className={`relative inline-flex size-2 rounded-full ${status === "NORMAL" ? "bg-success" : status === "WARNING" ? "bg-warning" : "bg-destructive"}`} />
          </span>
          <span className="text-[11px] font-semibold text-foreground tracking-wide">
            {status === "NORMAL" ? "All Systems Normal" : status === "WARNING" ? "Attention Required" : "Emergency Active"}
          </span>
        </div>

        {/* Animation */}
        <MotionAnimation activity={activity} size="lg" />

        {/* Label */}
        <div className="flex flex-col items-center gap-1.5">
          <h2 className={`text-2xl font-bold font-mono tracking-tight sm:text-3xl ${color}`}>
            {label}
          </h2>
          <p className="text-sm text-muted-foreground">{sublabel}</p>
        </div>
      </CardContent>
    </Card>
  )
}

/* ---- Compact info cards with premium styling ---- */
function BatteryCard() {
  const { battery } = useFallDetection()

  const BatteryIcon = battery > 60 ? BatteryFull : battery > 20 ? BatteryMedium : BatteryLow
  const batteryColor = battery > 50 ? "text-success" : battery > 20 ? "text-warning-foreground" : "text-destructive"
  const barColor = battery > 50 ? "bg-success" : battery > 20 ? "bg-warning" : "bg-destructive"
  const glowColor = battery > 50 ? "shadow-success/20" : battery > 20 ? "shadow-warning/20" : "shadow-destructive/20"

  return (
    <Card className={`border-0 shadow-md ${glowColor} transition-shadow duration-500`}>
      <CardContent className="flex flex-col items-center gap-2.5 px-3 py-4">
        <div className="flex size-10 items-center justify-center rounded-2xl bg-secondary">
          <BatteryIcon className={`size-5 ${batteryColor}`} />
        </div>
        <p className={`text-xl font-bold font-mono tabular-nums ${batteryColor}`}>{battery}%</p>
        <div className="h-1 w-full overflow-hidden rounded-full bg-muted">
          <div
            className={`h-full rounded-full transition-all duration-700 ease-out ${barColor}`}
            style={{ width: `${battery}%` }}
          />
        </div>
        <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">Battery</p>
      </CardContent>
    </Card>
  )
}

function LocationCard() {
  return (
    <Card className="border-0 shadow-md shadow-accent/10 transition-shadow duration-500">
      <CardContent className="flex flex-col items-center gap-2.5 px-3 py-4">
        <div className="flex size-10 items-center justify-center rounded-2xl bg-accent/10">
          <MapPin className="size-5 text-accent" />
        </div>
        <p className="text-sm font-bold text-foreground">Home</p>
        <p className="text-xs text-muted-foreground leading-tight text-center">Living Room</p>
        <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">Location</p>
      </CardContent>
    </Card>
  )
}

function PatientCard() {
  return (
    <Card className="border-0 shadow-md shadow-primary/10 transition-shadow duration-500">
      <CardContent className="flex flex-col items-center gap-2.5 px-3 py-4">
        <div className="relative flex size-10 items-center justify-center rounded-2xl bg-primary/10">
          <User className="size-5 text-primary" />
          <span className="absolute -right-0.5 -top-0.5 size-2.5 rounded-full bg-success ring-2 ring-card" />
        </div>
        <p className="text-sm font-bold text-foreground text-center leading-tight">Margaret J.</p>
        <p className="text-xs text-muted-foreground">Age 78</p>
        <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">Patient</p>
      </CardContent>
    </Card>
  )
}

/* ---- System Status with ambient glow ---- */
function StatusBar() {
  const { status } = useFallDetection()

  const config = {
    NORMAL: {
      label: "All Systems Normal",
      icon: ShieldCheck,
      bgClass: "bg-success/8",
      textClass: "text-success",
      borderClass: "border-success/15",
      glowClass: "shadow-success/10",
      badgeClass: "bg-success/10 text-success border-success/20",
    },
    WARNING: {
      label: "Attention Required",
      icon: AlertTriangle,
      bgClass: "bg-warning/8",
      textClass: "text-warning-foreground",
      borderClass: "border-warning/15",
      glowClass: "shadow-warning/10",
      badgeClass: "bg-warning/10 text-warning-foreground border-warning/20",
    },
    FALL: {
      label: "Emergency Active",
      icon: CircleAlert,
      bgClass: "bg-destructive/8",
      textClass: "text-destructive",
      borderClass: "border-destructive/15",
      glowClass: "shadow-destructive/10",
      badgeClass: "bg-destructive/10 text-destructive border-destructive/20",
    },
  }

  const c = config[status]
  const Icon = c.icon

  return (
    <Card className={`relative overflow-hidden border ${c.borderClass} shadow-lg ${c.glowClass} transition-all duration-500`}>
      {/* Subtle ambient fill */}
      <div className={`absolute inset-0 ${c.bgClass}`} />
      <CardContent className="relative flex items-center gap-3.5 py-3.5">
        <div className={`flex size-11 items-center justify-center rounded-2xl ${c.bgClass}`}>
          <Icon className={`size-5 ${c.textClass}`} />
        </div>
        <div className="flex flex-1 flex-col gap-0.5">
          <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">System Status</p>
          <p className={`text-sm font-bold font-mono ${c.textClass}`}>{c.label}</p>
        </div>
        <Badge variant="outline" className={`${c.badgeClass} text-[10px] font-semibold px-2.5 py-0.5`}>
          {status}
        </Badge>
      </CardContent>
    </Card>
  )
}

/* ---- Vitals mini strip ---- */
function VitalsStrip() {
  return (
    <Card className="border-0 shadow-md shadow-primary/5">
      <CardContent className="flex items-center justify-around py-3">
        <div className="flex items-center gap-2">
          <Heart className="size-4 text-destructive" />
          <div className="flex flex-col">
            <span className="text-sm font-bold font-mono text-foreground tabular-nums">72</span>
            <span className="text-[9px] text-muted-foreground uppercase tracking-wider">BPM</span>
          </div>
        </div>
        <div className="h-6 w-px bg-border" />
        <div className="flex items-center gap-2">
          <span className="text-[10px] font-semibold text-accent">SpO2</span>
          <div className="flex flex-col">
            <span className="text-sm font-bold font-mono text-foreground tabular-nums">98%</span>
            <span className="text-[9px] text-muted-foreground uppercase tracking-wider">Oxygen</span>
          </div>
        </div>
        <div className="h-6 w-px bg-border" />
        <div className="flex items-center gap-2">
          <span className="text-[10px] font-semibold text-primary">Temp</span>
          <div className="flex flex-col">
            <span className="text-sm font-bold font-mono text-foreground tabular-nums">36.5</span>
            <span className="text-[9px] text-muted-foreground uppercase tracking-wider">{'Celsius'}</span>
          </div>
        </div>
      </CardContent>
    </Card>
  )
}

/* ---- Dashboard Layout ---- */
export function DashboardScreen() {
  return (
    <div className="flex flex-col gap-4 pb-4">
      <MotionHero />

      {/* Info cards grid */}
      <div className="grid grid-cols-3 gap-3">
        <BatteryCard />
        <LocationCard />
        <PatientCard />
      </div>

      {/* Vitals strip */}
      <VitalsStrip />

      {/* System status */}
      <StatusBar />
    </div>
  )
}
