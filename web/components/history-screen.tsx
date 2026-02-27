"use client"

import { useFallDetection, type EventType, type EventStatus } from "@/lib/fall-detection-context"
import { Card, CardContent } from "@/components/ui/card"
import { Badge } from "@/components/ui/badge"
import {
  Footprints,
  Moon,
  AlertTriangle,
  CircleAlert,
  Siren,
  Clock,
  History,
  CalendarDays,
} from "lucide-react"
import { formatDistanceToNow } from "date-fns"

const eventConfig: Record<
  EventType,
  { icon: typeof Footprints; label: string; color: string; bg: string; glow: string }
> = {
  walk: { icon: Footprints, label: "Walk", color: "text-success", bg: "bg-success/10", glow: "shadow-success/10" },
  sleep: { icon: Moon, label: "Sleep", color: "text-primary", bg: "bg-primary/10", glow: "shadow-primary/10" },
  near_fall: {
    icon: AlertTriangle,
    label: "Near Fall",
    color: "text-warning-foreground",
    bg: "bg-warning/10",
    glow: "shadow-warning/10",
  },
  fall: { icon: CircleAlert, label: "Fall", color: "text-destructive", bg: "bg-destructive/10", glow: "shadow-destructive/10" },
  sos: { icon: Siren, label: "SOS", color: "text-destructive", bg: "bg-destructive/10", glow: "shadow-destructive/10" },
}

const statusConfig: Record<EventStatus, { label: string; className: string }> = {
  resolved: {
    label: "Resolved",
    className: "bg-success/10 text-success border-success/20",
  },
  pending: {
    label: "Pending",
    className: "bg-warning/10 text-warning-foreground border-warning/20",
  },
}

export function HistoryScreen() {
  const { history } = useFallDetection()

  if (history.length === 0) {
    return (
      <div className="flex flex-col items-center gap-4 py-20 text-center">
        <div className="flex size-16 items-center justify-center rounded-3xl bg-muted">
          <History className="size-8 text-muted-foreground/40" />
        </div>
        <div className="flex flex-col gap-1.5">
          <p className="text-base font-semibold text-foreground">No events recorded</p>
          <p className="text-sm text-muted-foreground leading-relaxed">
            Events from the device will appear here
          </p>
        </div>
      </div>
    )
  }

  return (
    <div className="flex flex-col gap-4 pb-4">
      {/* Header */}
      <div className="flex items-center justify-between">
        <div className="flex items-center gap-2">
          <CalendarDays className="size-4 text-muted-foreground" />
          <p className="text-xs font-semibold uppercase tracking-wider text-muted-foreground">
            Event Timeline
          </p>
        </div>
        <Badge variant="secondary" className="font-mono text-[10px] font-semibold tracking-wider">
          {history.length} events
        </Badge>
      </div>

      {/* Timeline */}
      <div className="relative flex flex-col gap-0">
        {/* Timeline line */}
        <div className="absolute left-[23px] top-8 bottom-8 w-px bg-border" />

        {history.map((event, index) => {
          const ec = eventConfig[event.type]
          const sc = statusConfig[event.status]
          const Icon = ec.icon

          return (
            <div
              key={event.id}
              className="relative flex items-start gap-3.5 py-2"
              style={{ animationDelay: `${index * 80}ms` }}
            >
              {/* Timeline dot */}
              <div
                className={`relative z-10 flex size-[46px] shrink-0 items-center justify-center rounded-2xl ${ec.bg} shadow-md ${ec.glow} transition-all duration-300`}
              >
                <Icon className={`size-5 ${ec.color}`} />
              </div>

              {/* Content card */}
              <Card className="flex-1 border-0 shadow-sm hover:shadow-md transition-shadow duration-300">
                <CardContent className="flex flex-col gap-2 px-3.5 py-3">
                  <div className="flex items-center justify-between">
                    <p className="text-sm font-semibold text-foreground">
                      {event.label}
                    </p>
                    <Badge
                      variant="outline"
                      className={`text-[10px] font-semibold px-2 py-0 ${sc.className}`}
                    >
                      {sc.label}
                    </Badge>
                  </div>
                  <div className="flex items-center gap-1.5">
                    <Clock className="size-3 text-muted-foreground/40" />
                    <p className="text-xs text-muted-foreground">
                      {formatDistanceToNow(event.time, { addSuffix: true })}
                    </p>
                  </div>
                </CardContent>
              </Card>
            </div>
          )
        })}
      </div>
    </div>
  )
}
