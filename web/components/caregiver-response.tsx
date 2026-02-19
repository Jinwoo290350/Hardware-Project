"use client"

import { useFallDetection } from "@/lib/fall-detection-context"
import { Button } from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { HeartHandshake, CheckCircle2, MapPin, Clock, User, ArrowRight } from "lucide-react"
import { useEffect, useState } from "react"

export function CaregiverResponse() {
  const { resolveEmergency } = useFallDetection()
  const [step, setStep] = useState(0)

  useEffect(() => {
    const t1 = setTimeout(() => setStep(1), 600)
    const t2 = setTimeout(() => setStep(2), 1200)
    const t3 = setTimeout(() => setStep(3), 1800)
    return () => { clearTimeout(t1); clearTimeout(t2); clearTimeout(t3) }
  }, [])

  return (
    <div className="fixed inset-0 z-50 flex flex-col bg-background">
      {/* Subtle ambient glow */}
      <div className="absolute inset-0 bg-[radial-gradient(circle_at_50%_20%,oklch(0.62_0.18_158/0.08),transparent_60%)]" />

      <div className="relative flex flex-1 flex-col items-center justify-center px-6">
        <div className="flex w-full max-w-sm flex-col items-center gap-8 text-center">
          {/* Success icon with glow */}
          <div className="relative">
            <div className="absolute -inset-3 rounded-full bg-success/10 blur-xl animate-ambient-breathe" />
            <div className="relative flex size-20 items-center justify-center rounded-3xl bg-success/10 ring-1 ring-success/20">
              <HeartHandshake className="size-10 text-success" />
            </div>
          </div>

          <div className="flex flex-col gap-2">
            <h1 className="text-2xl font-bold tracking-tight text-foreground sm:text-3xl">
              Help is on the way
            </h1>
            <p className="text-muted-foreground leading-relaxed">
              A caregiver has been notified and is responding to the alert
            </p>
          </div>

          {/* Step cards with stagger animation */}
          <div className="flex w-full flex-col gap-2.5">
            {[
              {
                visible: step >= 1,
                icon: CheckCircle2,
                iconBg: "bg-success/10",
                iconColor: "text-success",
                title: "Alert received",
                subtitle: "Emergency acknowledged",
                delay: "delay-0",
              },
              {
                visible: step >= 2,
                icon: User,
                iconBg: "bg-success/10",
                iconColor: "text-success",
                title: "Caregiver notified",
                subtitle: "Dr. Sarah Chen - Primary",
                delay: "delay-100",
              },
              {
                visible: step >= 3,
                icon: MapPin,
                iconBg: "bg-primary/10",
                iconColor: "text-primary",
                title: "ETA: 8 minutes",
                subtitle: "Location shared with responder",
                delay: "delay-200",
              },
            ].map(({ visible, icon: Icon, iconBg, iconColor, title, subtitle }, i) => (
              <Card
                key={i}
                className={`border-0 shadow-md transition-all duration-500 ${
                  visible ? "opacity-100 translate-y-0" : "opacity-0 translate-y-6"
                }`}
              >
                <CardContent className="flex items-center gap-3.5 py-3.5">
                  <div className={`flex size-10 shrink-0 items-center justify-center rounded-2xl ${iconBg}`}>
                    <Icon className={`size-5 ${iconColor}`} />
                  </div>
                  <div className="flex-1 text-left">
                    <p className="text-sm font-semibold text-foreground">{title}</p>
                    <p className="text-xs text-muted-foreground">{subtitle}</p>
                  </div>
                  <Clock className="size-4 text-muted-foreground/30" />
                </CardContent>
              </Card>
            ))}
          </div>

          {/* Resolve */}
          <Button
            size="lg"
            className="mt-2 h-14 w-full rounded-2xl bg-primary text-primary-foreground text-base font-bold hover:bg-primary/90 shadow-xl shadow-primary/15 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
            onClick={resolveEmergency}
          >
            <CheckCircle2 className="mr-2 size-5" />
            Mark as Resolved
            <ArrowRight className="ml-2 size-4" />
          </Button>
        </div>
      </div>

      <div className="relative px-6 pb-8 pt-4 text-center">
        <p className="text-[10px] tracking-wider text-muted-foreground/40 uppercase">SafeStep Fall Detection System</p>
      </div>
    </div>
  )
}
