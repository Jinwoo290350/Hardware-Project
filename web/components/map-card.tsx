"use client"

import dynamic from "next/dynamic"
import { MapPin } from "lucide-react"
import { Card, CardContent } from "@/components/ui/card"
import { useFallDetection } from "@/lib/fall-detection-context"

// Dynamic import — Leaflet ใช้ browser API (window/document) ไม่รองรับ SSR
const MapView = dynamic(() => import("./map-view"), {
  ssr: false,
  loading: () => (
    <div className="h-52 w-full animate-pulse rounded-t-xl bg-muted" />
  ),
})

// Default = คณะวิศวกรรมศาสตร์ มก.
const DEFAULT_LAT = 13.8474
const DEFAULT_LON = 100.5693

export function MapCard() {
  const { gpsLat, gpsLon, connected } = useFallDetection()

  const lat = gpsLat !== 0 ? gpsLat : DEFAULT_LAT
  const lon = gpsLon !== 0 ? gpsLon : DEFAULT_LON
  const hasGPS = connected && (gpsLat !== 0 || gpsLon !== 0)

  return (
    <Card className="border-0 shadow-md overflow-hidden">
      <MapView lat={lat} lon={lon} />
      <CardContent className="flex items-center gap-2 px-4 py-2.5">
        <MapPin className="size-3.5 text-accent shrink-0" />
        <span className="flex-1 text-[11px] font-mono text-muted-foreground tabular-nums">
          {lat.toFixed(5)},&nbsp;{lon.toFixed(5)}
        </span>
        {!hasGPS && (
          <span className="text-[10px] text-muted-foreground/60 italic">
            {connected ? "searching…" : "default"}
          </span>
        )}
      </CardContent>
    </Card>
  )
}
