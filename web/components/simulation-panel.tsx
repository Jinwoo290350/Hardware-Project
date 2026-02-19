"use client"

import { useFallDetection } from "@/lib/fall-detection-context"
import { Card, CardContent, CardHeader, CardTitle } from "@/components/ui/card"
import { Button } from "@/components/ui/button"
import { Switch } from "@/components/ui/switch"
import { Badge } from "@/components/ui/badge"
import {
  Footprints,
  Moon,
  AlertTriangle,
  CircleAlert,
  Siren,
  FlaskConical,
  ToggleLeft,
  Zap,
} from "lucide-react"

export function SimulationPanel() {
  const {
    simulationMode,
    setSimulationMode,
    simulateWalk,
    simulateSleep,
    simulateNearFall,
    simulateFall,
    simulateSOS,
  } = useFallDetection()

  return (
    <div className="flex flex-col gap-4 pb-4">
      {/* Toggle card */}
      <Card className="relative overflow-hidden border-0 shadow-lg shadow-primary/5">
        <div className="absolute inset-0 bg-primary/3" />
        <CardHeader className="relative pb-2">
          <CardTitle className="flex items-center justify-between">
            <span className="flex items-center gap-2.5 text-base">
              <div className="flex size-9 items-center justify-center rounded-xl bg-primary/10">
                <FlaskConical className="size-4.5 text-primary" />
              </div>
              <span className="font-bold">Simulation Mode</span>
            </span>
            <div className="flex items-center gap-2.5">
              <Badge
                variant={simulationMode ? "default" : "secondary"}
                className={`text-[10px] font-semibold tracking-wide ${
                  simulationMode
                    ? "bg-success/10 text-success border border-success/20"
                    : ""
                }`}
              >
                {simulationMode ? "Active" : "Inactive"}
              </Badge>
              <Switch
                checked={simulationMode}
                onCheckedChange={setSimulationMode}
                aria-label="Toggle simulation mode"
              />
            </div>
          </CardTitle>
        </CardHeader>
        <CardContent className="relative">
          <p className="text-sm text-muted-foreground leading-relaxed">
            Enable simulation to test the full system flow with mock sensor events.
          </p>
        </CardContent>
      </Card>

      {simulationMode && (
        <div className="flex flex-col gap-4 animate-fade-in-up">
          {/* Normal states */}
          <div className="flex flex-col gap-2.5">
            <div className="flex items-center gap-2">
              <Zap className="size-3 text-muted-foreground" />
              <p className="text-[10px] font-semibold uppercase tracking-[0.15em] text-muted-foreground">
                Normal States
              </p>
            </div>
            <div className="grid grid-cols-2 gap-3">
              <Button
                variant="outline"
                className="flex h-auto flex-col gap-3 rounded-2xl border-0 bg-card py-5 text-foreground shadow-md shadow-success/5 hover:shadow-lg hover:shadow-success/10 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
                onClick={simulateWalk}
              >
                <div className="flex size-11 items-center justify-center rounded-2xl bg-success/10">
                  <Footprints className="size-5 text-success" />
                </div>
                <span className="text-sm font-semibold">Walk</span>
              </Button>
              <Button
                variant="outline"
                className="flex h-auto flex-col gap-3 rounded-2xl border-0 bg-card py-5 text-foreground shadow-md shadow-primary/5 hover:shadow-lg hover:shadow-primary/10 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
                onClick={simulateSleep}
              >
                <div className="flex size-11 items-center justify-center rounded-2xl bg-primary/10">
                  <Moon className="size-5 text-primary" />
                </div>
                <span className="text-sm font-semibold">Sleep</span>
              </Button>
            </div>
          </div>

          {/* Alert states */}
          <div className="flex flex-col gap-2.5">
            <div className="flex items-center gap-2">
              <AlertTriangle className="size-3 text-muted-foreground" />
              <p className="text-[10px] font-semibold uppercase tracking-[0.15em] text-muted-foreground">
                Alert States
              </p>
            </div>
            <Button
              variant="outline"
              className="flex h-auto items-center gap-4 rounded-2xl border-0 bg-card py-4 text-foreground shadow-md shadow-warning/5 hover:shadow-lg hover:shadow-warning/10 transition-all duration-300 hover:scale-[1.01] active:scale-[0.99]"
              onClick={simulateNearFall}
            >
              <div className="flex size-11 items-center justify-center rounded-2xl bg-warning/10">
                <AlertTriangle className="size-5 text-warning-foreground" />
              </div>
              <span className="text-sm font-semibold">Simulate Near Fall</span>
            </Button>
          </div>

          {/* Emergency states */}
          <div className="flex flex-col gap-2.5">
            <div className="flex items-center gap-2">
              <CircleAlert className="size-3 text-muted-foreground" />
              <p className="text-[10px] font-semibold uppercase tracking-[0.15em] text-muted-foreground">
                Emergency States
              </p>
            </div>
            <div className="grid grid-cols-2 gap-3">
              <Button
                variant="outline"
                className="flex h-auto flex-col gap-3 rounded-2xl border-0 bg-card py-5 text-foreground shadow-md shadow-destructive/5 hover:shadow-lg hover:shadow-destructive/10 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
                onClick={simulateFall}
              >
                <div className="flex size-11 items-center justify-center rounded-2xl bg-destructive/10">
                  <CircleAlert className="size-5 text-destructive" />
                </div>
                <span className="text-sm font-semibold">Fall</span>
              </Button>
              <Button
                variant="outline"
                className="flex h-auto flex-col gap-3 rounded-2xl border-0 bg-card py-5 text-foreground shadow-md shadow-destructive/5 hover:shadow-lg hover:shadow-destructive/10 transition-all duration-300 hover:scale-[1.02] active:scale-[0.98]"
                onClick={simulateSOS}
              >
                <div className="flex size-11 items-center justify-center rounded-2xl bg-destructive/10">
                  <Siren className="size-5 text-destructive" />
                </div>
                <span className="text-sm font-semibold">SOS</span>
              </Button>
            </div>
          </div>
        </div>
      )}

      {!simulationMode && (
        <Card className="border-dashed border-0 shadow-sm bg-muted/50">
          <CardContent className="flex flex-col items-center gap-4 py-10 text-center">
            <div className="flex size-14 items-center justify-center rounded-2xl bg-muted">
              <ToggleLeft className="size-7 text-muted-foreground/40" />
            </div>
            <div className="flex flex-col gap-1">
              <p className="text-sm font-semibold text-muted-foreground">Simulation Disabled</p>
              <p className="text-xs text-muted-foreground/60">
                Enable Simulation Mode above to access test controls
              </p>
            </div>
          </CardContent>
        </Card>
      )}
    </div>
  )
}
