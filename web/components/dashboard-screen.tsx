"use client"

import { useFallDetection } from "@/lib/fall-detection-context"
import { Card, CardContent } from "@/components/ui/card"
import { Badge }             from "@/components/ui/badge"
import {
  ShieldCheck,
  AlertTriangle,
  CircleAlert,
  MapPin,
  Shirt,
  Wifi,
  WifiOff,
} from "lucide-react"
import { MotionAnimation } from "@/components/motion-animation"

/* ── Hero ── */
function MotionHero() {
  const { activity, status } = useFallDetection()

  const labelConfig: Record<string, { label: string; sublabel: string; color: string; glowClass: string }> = {
    Walking:    { label: "Walking",      sublabel: "Active movement detected",   color: "text-success",              glowClass: "bg-success/15"     },
    Sitting:    { label: "Sitting",      sublabel: "Resting position detected",  color: "text-primary",              glowClass: "bg-primary/15"     },
    Sleeping:   { label: "Sleeping",     sublabel: "Sleep cycle in progress",    color: "text-primary",              glowClass: "bg-primary/15"     },
    "Near Fall":{ label: "Near Fall",    sublabel: "Instability detected",       color: "text-warning-foreground",   glowClass: "bg-warning/20"     },
    Fallen:     { label: "Fall Detected",sublabel: "Emergency protocol active",  color: "text-destructive",          glowClass: "bg-destructive/15" },
  }

  const { label, sublabel, color, glowClass } = labelConfig[activity]

  return (
    <Card className="relative overflow-hidden border-0 shadow-lg shadow-primary/5">
      <div className={`absolute inset-0 ${glowClass} animate-ambient-breathe`} />
      <div className="absolute inset-0 bg-card/60 backdrop-blur-sm" />
      <CardContent className="relative flex flex-col items-center gap-5 py-8 sm:py-10">
        <div className="flex items-center gap-2 rounded-full bg-secondary/80 px-3 py-1 backdrop-blur-sm">
          <span className="relative flex size-2">
            <span className={`absolute inline-flex size-full animate-ping rounded-full ${
              status === "NORMAL" ? "bg-success" : status === "WARNING" ? "bg-warning" : "bg-destructive"
            } opacity-75`} />
            <span className={`relative inline-flex size-2 rounded-full ${
              status === "NORMAL" ? "bg-success" : status === "WARNING" ? "bg-warning" : "bg-destructive"
            }`} />
          </span>
          <span className="text-[11px] font-semibold text-foreground tracking-wide">
            {status === "NORMAL" ? "All Systems Normal" : status === "WARNING" ? "Attention Required" : "Emergency Active"}
          </span>
        </div>
        <MotionAnimation activity={activity} size="lg" />
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

/* ── GPS Location ── */
function LocationCard() {
  const { gpsLat, gpsLon, connected } = useFallDetection()
  const hasGPS = connected && (gpsLat !== 0 || gpsLon !== 0)

  return (
    <Card className="border-0 shadow-md shadow-accent/10">
      <CardContent className="flex flex-col items-center gap-2.5 px-3 py-4">
        <div className="flex size-10 items-center justify-center rounded-2xl bg-accent/10">
          <MapPin className="size-5 text-accent" />
        </div>
        {hasGPS ? (
          <>
            <p className="text-[10px] font-bold font-mono text-foreground text-center leading-tight tabular-nums">
              {gpsLat.toFixed(4)}
            </p>
            <p className="text-[10px] font-mono text-muted-foreground text-center">
              {gpsLon.toFixed(4)}
            </p>
          </>
        ) : (
          <p className="text-xs text-muted-foreground text-center leading-tight">
            {connected ? "Searching…" : "No device"}
          </p>
        )}
        <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">GPS</p>
      </CardContent>
    </Card>
  )
}

/* ── Device Mode (CHEST / SHIRT / PANTS) ── */
function DeviceModeCard() {
  const { deviceMode, connected } = useFallDetection()

  const modeConfig = {
    CHEST: { label: "Chest",  sub: "Clip",   color: "text-success", bg: "bg-success/10" },
    SHIRT: { label: "Shirt",  sub: "Pocket", color: "text-primary", bg: "bg-primary/10" },
    PANTS: { label: "Pants",  sub: "Pocket", color: "text-accent",  bg: "bg-accent/10"  },
  }
  const cfg = modeConfig[deviceMode]

  return (
    <Card className="border-0 shadow-md shadow-primary/10">
      <CardContent className="flex flex-col items-center gap-2.5 px-3 py-4">
        <div className={`relative flex size-10 items-center justify-center rounded-2xl ${cfg.bg}`}>
          <Shirt className={`size-5 ${cfg.color}`} />
          <span className={`absolute -right-0.5 -top-0.5 size-2.5 rounded-full ring-2 ring-card ${connected ? "bg-success" : "bg-muted-foreground"}`} />
        </div>
        <p className={`text-sm font-bold text-center leading-tight ${cfg.color}`}>{cfg.label}</p>
        <p className="text-xs text-muted-foreground">{cfg.sub}</p>
        <p className="text-[10px] font-medium text-muted-foreground uppercase tracking-wider">Mode</p>
      </CardContent>
    </Card>
  )
}

/* ── Connection strip ── */
function ConnectionStrip() {
  const { connected } = useFallDetection()

  return (
    <Card className="border-0 shadow-md shadow-primary/5">
      <CardContent className="flex items-center justify-between py-3 px-4">
        <div className="flex items-center gap-2">
          {connected
            ? <Wifi    className="size-4 text-success" />
            : <WifiOff className="size-4 text-muted-foreground" />
          }
          <span className="text-xs font-medium text-foreground">
            {connected ? "Device connected via Blynk" : "Waiting for device…"}
          </span>
        </div>
        <Badge
          variant="outline"
          className={connected
            ? "bg-success/10 text-success border-success/20 text-[10px] font-semibold"
            : "bg-muted text-muted-foreground text-[10px] font-semibold"
          }
        >
          {connected ? "ONLINE" : "OFFLINE"}
        </Badge>
      </CardContent>
    </Card>
  )
}

/* ── System Status ── */
function StatusBar() {
  const { status } = useFallDetection()

  const config = {
    NORMAL:  { label: "All Systems Normal", icon: ShieldCheck,   bgClass: "bg-success/8",     textClass: "text-success",            borderClass: "border-success/15",     glowClass: "shadow-success/10",     badgeClass: "bg-success/10 text-success border-success/20"                       },
    WARNING: { label: "Attention Required", icon: AlertTriangle, bgClass: "bg-warning/8",     textClass: "text-warning-foreground", borderClass: "border-warning/15",     glowClass: "shadow-warning/10",     badgeClass: "bg-warning/10 text-warning-foreground border-warning/20"            },
    FALL:    { label: "Emergency Active",   icon: CircleAlert,   bgClass: "bg-destructive/8", textClass: "text-destructive",        borderClass: "border-destructive/15", glowClass: "shadow-destructive/10", badgeClass: "bg-destructive/10 text-destructive border-destructive/20"          },
  }

  const c    = config[status]
  const Icon = c.icon

  return (
    <Card className={`relative overflow-hidden border ${c.borderClass} shadow-lg ${c.glowClass} transition-all duration-500`}>
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

/* ── Layout ── */
export function DashboardScreen() {
  return (
    <div className="flex flex-col gap-4 pb-4">
      <MotionHero />
      <div className="grid grid-cols-2 gap-3">
        <LocationCard />
        <DeviceModeCard />
      </div>
      <ConnectionStrip />
      <StatusBar />
    </div>
  )
}
