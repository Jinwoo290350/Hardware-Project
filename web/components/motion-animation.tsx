"use client"

import type { MotionActivity } from "@/lib/fall-detection-context"

function WalkingAnimation() {
  return (
    <svg viewBox="0 0 80 80" className="size-full" aria-label="Walking animation">
      {/* Left leg stepping */}
      <g className="animate-walk-left origin-[32px_48px]">
        <line x1="32" y1="48" x2="24" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      </g>
      {/* Right leg stepping */}
      <g className="animate-walk-right origin-[32px_48px]">
        <line x1="32" y1="48" x2="40" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      </g>
      {/* Body */}
      <line x1="32" y1="28" x2="32" y2="48" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Left arm */}
      <g className="animate-walk-right origin-[32px_32px]">
        <line x1="32" y1="34" x2="22" y2="44" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      </g>
      {/* Right arm */}
      <g className="animate-walk-left origin-[32px_32px]">
        <line x1="32" y1="34" x2="42" y2="44" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      </g>
      {/* Head */}
      <circle cx="32" cy="20" r="8" fill="currentColor" opacity="0.2" />
      <circle cx="32" cy="20" r="8" stroke="currentColor" strokeWidth="3" fill="none" />
      {/* Ground line */}
      <line x1="10" y1="72" x2="54" y2="72" stroke="currentColor" strokeWidth="1.5" opacity="0.2" strokeDasharray="4 3" />
    </svg>
  )
}

function SittingAnimation() {
  return (
    <svg viewBox="0 0 80 80" className="size-full" aria-label="Sitting animation">
      {/* Chair */}
      <path d="M18 42 L18 70 M50 42 L50 70 M14 42 L54 42" stroke="currentColor" strokeWidth="2" strokeLinecap="round" opacity="0.25" />
      {/* Legs (bent) */}
      <line x1="30" y1="48" x2="30" y2="58" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      <line x1="30" y1="58" x2="22" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      <line x1="38" y1="48" x2="38" y2="58" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      <line x1="38" y1="58" x2="46" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Body */}
      <line x1="34" y1="26" x2="34" y2="48" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Arms resting */}
      <line x1="34" y1="34" x2="24" y2="42" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      <line x1="34" y1="34" x2="44" y2="42" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Head with gentle bob */}
      <g className="animate-gentle-bob">
        <circle cx="34" cy="18" r="8" fill="currentColor" opacity="0.2" />
        <circle cx="34" cy="18" r="8" stroke="currentColor" strokeWidth="3" fill="none" />
      </g>
    </svg>
  )
}

function SleepingAnimation() {
  return (
    <svg viewBox="0 0 80 80" className="size-full" aria-label="Sleeping animation">
      {/* Bed / horizontal surface */}
      <rect x="8" y="48" width="60" height="4" rx="2" fill="currentColor" opacity="0.15" />
      {/* Lying body */}
      <line x1="14" y1="44" x2="52" y2="44" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Legs */}
      <line x1="52" y1="44" x2="62" y2="44" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
      {/* Head */}
      <circle cx="14" cy="38" r="7" fill="currentColor" opacity="0.2" />
      <circle cx="14" cy="38" r="7" stroke="currentColor" strokeWidth="3" fill="none" />
      {/* Pillow */}
      <rect x="6" y="34" width="12" height="6" rx="3" fill="currentColor" opacity="0.1" stroke="currentColor" strokeWidth="1" opacity="0.2" />
      {/* Zzz animation */}
      <g className="animate-zzz-1">
        <text x="28" y="28" fill="currentColor" fontSize="10" fontWeight="bold" opacity="0.6">z</text>
      </g>
      <g className="animate-zzz-2">
        <text x="36" y="20" fill="currentColor" fontSize="13" fontWeight="bold" opacity="0.4">z</text>
      </g>
      <g className="animate-zzz-3">
        <text x="44" y="12" fill="currentColor" fontSize="16" fontWeight="bold" opacity="0.25">z</text>
      </g>
      {/* Blanket */}
      <path d="M20 46 Q38 40 56 46" stroke="currentColor" strokeWidth="1.5" fill="currentColor" fillOpacity="0.06" opacity="0.3" />
    </svg>
  )
}

function NearFallAnimation() {
  return (
    <svg viewBox="0 0 80 80" className="size-full" aria-label="Near fall animation">
      <g className="animate-wobble origin-[34px_68px]">
        {/* Legs */}
        <line x1="30" y1="48" x2="26" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        <line x1="38" y1="48" x2="42" y2="68" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        {/* Body tilting */}
        <line x1="34" y1="28" x2="34" y2="48" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        {/* Arms flailing */}
        <g className="animate-flail-left origin-[34px_34px]">
          <line x1="34" y1="34" x2="18" y2="28" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        </g>
        <g className="animate-flail-right origin-[34px_34px]">
          <line x1="34" y1="34" x2="50" y2="28" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        </g>
        {/* Head */}
        <circle cx="34" cy="20" r="8" fill="currentColor" opacity="0.2" />
        <circle cx="34" cy="20" r="8" stroke="currentColor" strokeWidth="3" fill="none" />
      </g>
      {/* Warning marks */}
      <g className="animate-blink">
        <line x1="58" y1="14" x2="62" y2="10" stroke="currentColor" strokeWidth="2" strokeLinecap="round" opacity="0.5" />
        <line x1="62" y1="18" x2="68" y2="16" stroke="currentColor" strokeWidth="2" strokeLinecap="round" opacity="0.5" />
        <line x1="56" y1="22" x2="60" y2="24" stroke="currentColor" strokeWidth="2" strokeLinecap="round" opacity="0.3" />
      </g>
      {/* Ground */}
      <line x1="10" y1="72" x2="54" y2="72" stroke="currentColor" strokeWidth="1.5" opacity="0.2" strokeDasharray="4 3" />
    </svg>
  )
}

function FallenAnimation() {
  return (
    <svg viewBox="0 0 80 80" className="size-full" aria-label="Fallen animation">
      {/* Ground */}
      <line x1="6" y1="62" x2="74" y2="62" stroke="currentColor" strokeWidth="1.5" opacity="0.2" />
      {/* Body on ground */}
      <g className="animate-fall-impact">
        <line x1="20" y1="56" x2="52" y2="56" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        {/* Legs */}
        <line x1="52" y1="56" x2="60" y2="48" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        <line x1="52" y1="56" x2="62" y2="58" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        {/* Arms */}
        <line x1="30" y1="56" x2="26" y2="46" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        <line x1="38" y1="56" x2="42" y2="48" stroke="currentColor" strokeWidth="3" strokeLinecap="round" />
        {/* Head */}
        <circle cx="14" cy="52" r="7" fill="currentColor" opacity="0.2" />
        <circle cx="14" cy="52" r="7" stroke="currentColor" strokeWidth="3" fill="none" />
      </g>
      {/* Impact lines */}
      <g className="animate-impact-lines">
        <line x1="10" y1="42" x2="6" y2="36" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" opacity="0.4" />
        <line x1="18" y1="40" x2="16" y2="34" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" opacity="0.3" />
        <line x1="26" y1="42" x2="28" y2="36" stroke="currentColor" strokeWidth="1.5" strokeLinecap="round" opacity="0.4" />
      </g>
      {/* Alert circle */}
      <g className="animate-alert-ring">
        <circle cx="60" cy="22" r="10" stroke="currentColor" strokeWidth="2" fill="none" opacity="0.3" />
        <text x="57" y="27" fill="currentColor" fontSize="14" fontWeight="bold" opacity="0.6">!</text>
      </g>
    </svg>
  )
}

export function MotionAnimation({
  activity,
  size = "sm",
}: {
  activity: MotionActivity
  size?: "sm" | "lg"
}) {
  const config: Record<MotionActivity, { component: React.FC; color: string; bg: string }> = {
    Walking: { component: WalkingAnimation, color: "text-success", bg: "bg-success/10" },
    Sitting: { component: SittingAnimation, color: "text-primary", bg: "bg-primary/10" },
    Sleeping: { component: SleepingAnimation, color: "text-primary", bg: "bg-primary/10" },
    "Near Fall": { component: NearFallAnimation, color: "text-warning-foreground", bg: "bg-warning/10" },
    Fallen: { component: FallenAnimation, color: "text-destructive", bg: "bg-destructive/10" },
  }

  const { component: AnimComponent, color, bg } = config[activity]

  const sizeClasses = size === "lg" ? "size-36 sm:size-44" : "size-12"
  const containerClasses =
    size === "lg"
      ? "rounded-[2rem] p-8 sm:p-10 ring-1 ring-inset"
      : "rounded-2xl p-2"
  const ringClass = size === "lg" ? {
    Walking: "ring-success/15",
    Sitting: "ring-primary/15",
    Sleeping: "ring-primary/15",
    "Near Fall": "ring-warning/20",
    Fallen: "ring-destructive/20",
  }[activity] : ""

  return (
    <div className={`relative flex items-center justify-center ${containerClasses} ${bg} ${color} ${ringClass} transition-all duration-500`}>
      {size === "lg" && <div className={`absolute inset-0 rounded-[2rem] ${bg} blur-xl opacity-50 -z-10 animate-ambient-breathe`} />}
      <div className={sizeClasses}>
        <AnimComponent />
      </div>
    </div>
  )
}
