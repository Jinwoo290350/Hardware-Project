"use client"

import { useState } from "react"
import { ShieldCheck, UserPlus, LogIn, ArrowLeft, Copy, CheckCircle2 } from "lucide-react"
import { Button }        from "@/components/ui/button"
import { Card, CardContent } from "@/components/ui/card"
import { registerUser, loginUser, setSession } from "@/lib/user-store"

type Screen = "choose" | "register" | "code-shown" | "login"

interface AuthScreenProps {
  onAuth: (code: string, name: string) => void
}

export function AuthScreen({ onAuth }: AuthScreenProps) {
  const [screen, setScreen] = useState<Screen>("choose")
  const [name,   setName]   = useState("")
  const [code,   setCode]   = useState("")
  const [generatedCode, setGeneratedCode] = useState("")
  const [error,  setError]  = useState("")
  const [copied, setCopied] = useState(false)

  function handleRegister() {
    if (!name.trim()) { setError("กรุณาใส่ชื่อ"); return }
    setError("")
    const newCode = registerUser(name.trim())
    setGeneratedCode(newCode)
    setScreen("code-shown")
  }

  function handleLogin() {
    if (code.length !== 6) { setError("กรุณาใส่รหัส 6 หลัก"); return }
    const user = loginUser(code)
    if (!user) { setError("ไม่พบรหัสนี้ กรุณาลงทะเบียนก่อน"); return }
    setError("")
    setSession(code)
    onAuth(code, user.name)
  }

  function handleConfirmCode() {
    setSession(generatedCode)
    onAuth(generatedCode, name.trim())
  }

  function handleCopy() {
    navigator.clipboard.writeText(generatedCode).catch(() => {})
    setCopied(true)
    setTimeout(() => setCopied(false), 2000)
  }

  function goBack() {
    setScreen("choose")
    setError("")
    setCode("")
    setName("")
  }

  // ── Choose ──────────────────────────────────────────────────
  if (screen === "choose") return (
    <div className="flex min-h-dvh flex-col items-center justify-center bg-background px-6">
      <div className="flex w-full max-w-sm flex-col items-center gap-10">
        {/* Logo */}
        <div className="flex flex-col items-center gap-4">
          <div className="relative flex size-20 items-center justify-center rounded-3xl bg-primary shadow-xl shadow-primary/25">
            <ShieldCheck className="size-10 text-primary-foreground" />
            <div className="absolute -inset-3 -z-10 rounded-3xl bg-primary/15 blur-2xl animate-ambient-breathe" />
          </div>
          <div className="text-center">
            <h1 className="text-3xl font-bold tracking-tight font-mono">SafeStep</h1>
            <p className="text-sm text-muted-foreground mt-1.5">Fall Detection System</p>
          </div>
        </div>

        {/* Health Profile banner */}
        <div className="w-full rounded-2xl border border-border/50 bg-secondary/40 px-5 py-4 text-center">
          <p className="text-xs font-semibold uppercase tracking-wider text-muted-foreground">Health Profile</p>
          <p className="text-sm text-foreground mt-1">ประวัติการล้มของคุณจะถูกบันทึกแยกตามบัญชี</p>
        </div>

        {/* Buttons */}
        <div className="flex w-full flex-col gap-3">
          <Button
            size="lg"
            className="h-14 rounded-2xl text-base font-bold shadow-lg shadow-primary/15 transition-all hover:scale-[1.02] active:scale-[0.98]"
            onClick={() => { setScreen("register"); setError("") }}
          >
            <UserPlus className="mr-2.5 size-5" />
            ลงทะเบียน
          </Button>
          <Button
            size="lg"
            variant="outline"
            className="h-14 rounded-2xl text-base font-semibold transition-all hover:scale-[1.02] active:scale-[0.98]"
            onClick={() => { setScreen("login"); setError("") }}
          >
            <LogIn className="mr-2.5 size-5" />
            เข้าสู่ระบบ
          </Button>
        </div>

        <p className="text-center text-xs text-muted-foreground/60 leading-relaxed">
          ข้อมูลถูกเก็บบนอุปกรณ์นี้เท่านั้น · ไม่มีการส่งข้อมูลไป server
        </p>
      </div>
    </div>
  )

  // ── Register ────────────────────────────────────────────────
  if (screen === "register") return (
    <div className="flex min-h-dvh flex-col bg-background px-6 pt-14">
      <div className="mx-auto w-full max-w-sm">
        <button
          className="mb-8 flex items-center gap-2 text-sm text-muted-foreground hover:text-foreground transition-colors"
          onClick={goBack}
        >
          <ArrowLeft className="size-4" /> กลับ
        </button>

        <div className="flex flex-col gap-7">
          <div>
            <h2 className="text-2xl font-bold tracking-tight">สร้างบัญชีใหม่</h2>
            <p className="text-sm text-muted-foreground mt-1">ใส่ชื่อของคุณเพื่อสร้างโปรไฟล์</p>
          </div>

          <div className="flex flex-col gap-2">
            <label className="text-sm font-semibold">ชื่อ</label>
            <input
              type="text"
              placeholder="เช่น สมชาย ใจดี"
              value={name}
              onChange={e => { setName(e.target.value); setError("") }}
              onKeyDown={e => e.key === "Enter" && handleRegister()}
              className="h-13 rounded-xl border border-border bg-secondary/50 px-4 py-3 text-sm outline-none focus:ring-2 focus:ring-primary/50 transition-shadow"
              autoFocus
            />
            {error && <p className="text-xs text-destructive">{error}</p>}
          </div>

          <Button
            size="lg"
            className="h-14 rounded-2xl text-base font-bold shadow-lg shadow-primary/15"
            onClick={handleRegister}
            disabled={!name.trim()}
          >
            <UserPlus className="mr-2 size-5" />
            สร้างบัญชี
          </Button>
        </div>
      </div>
    </div>
  )

  // ── Code Shown ──────────────────────────────────────────────
  if (screen === "code-shown") return (
    <div className="flex min-h-dvh flex-col items-center justify-center bg-background px-6">
      <div className="flex w-full max-w-sm flex-col items-center gap-8 text-center">
        <div className="flex size-20 items-center justify-center rounded-3xl bg-success/10 ring-1 ring-success/20">
          <CheckCircle2 className="size-10 text-success" />
        </div>

        <div className="flex flex-col gap-2">
          <h2 className="text-2xl font-bold tracking-tight">ลงทะเบียนสำเร็จ!</h2>
          <p className="text-sm text-muted-foreground leading-relaxed">
            สวัสดี <span className="font-semibold text-foreground">{name}</span> — นี่คือรหัสเข้าระบบของคุณ<br />
            <span className="text-warning-foreground font-medium">จดรหัสไว้ให้ดี</span> ใช้สำหรับเข้าสู่ระบบครั้งหน้า
          </p>
        </div>

        {/* 6-digit code display */}
        <Card className="w-full border-0 shadow-xl">
          <CardContent className="flex flex-col items-center gap-5 py-7">
            <p className="text-[10px] font-bold uppercase tracking-[0.25em] text-muted-foreground">
              รหัสเข้าระบบ 6 หลัก
            </p>
            <div className="flex gap-2.5">
              {generatedCode.split("").map((digit, i) => (
                <div
                  key={i}
                  className="flex size-12 items-center justify-center rounded-xl bg-primary/10 font-mono text-2xl font-bold text-primary ring-1 ring-primary/20"
                >
                  {digit}
                </div>
              ))}
            </div>
            <button
              onClick={handleCopy}
              className="flex items-center gap-1.5 rounded-lg px-3 py-1.5 text-xs text-muted-foreground hover:bg-secondary/80 hover:text-foreground transition-colors"
            >
              {copied
                ? <><CheckCircle2 className="size-3.5 text-success" /> คัดลอกแล้ว</>
                : <><Copy className="size-3.5" /> คัดลอกรหัส</>
              }
            </button>
          </CardContent>
        </Card>

        <Button
          size="lg"
          className="h-14 w-full rounded-2xl text-base font-bold shadow-lg shadow-primary/15 transition-all hover:scale-[1.02]"
          onClick={handleConfirmCode}
        >
          <CheckCircle2 className="mr-2 size-5" />
          จดรหัสแล้ว เข้าสู่ระบบ
        </Button>
      </div>
    </div>
  )

  // ── Login ───────────────────────────────────────────────────
  return (
    <div className="flex min-h-dvh flex-col bg-background px-6 pt-14">
      <div className="mx-auto w-full max-w-sm">
        <button
          className="mb-8 flex items-center gap-2 text-sm text-muted-foreground hover:text-foreground transition-colors"
          onClick={goBack}
        >
          <ArrowLeft className="size-4" /> กลับ
        </button>

        <div className="flex flex-col gap-7">
          <div>
            <h2 className="text-2xl font-bold tracking-tight">เข้าสู่ระบบ</h2>
            <p className="text-sm text-muted-foreground mt-1">ใส่รหัส 6 หลักของคุณ</p>
          </div>

          {/* Code digit boxes (visual) */}
          <div className="flex flex-col gap-4 items-center">
            <div className="flex gap-2.5">
              {Array.from({ length: 6 }).map((_, i) => (
                <div
                  key={i}
                  className={`flex size-12 items-center justify-center rounded-xl border font-mono text-xl font-bold transition-colors ${
                    code[i]
                      ? "border-primary/50 bg-primary/10 text-primary"
                      : "border-border bg-secondary/50 text-muted-foreground/30"
                  }`}
                >
                  {code[i] ?? "–"}
                </div>
              ))}
            </div>

            <input
              type="text"
              inputMode="numeric"
              placeholder="กรอกรหัส 6 หลัก"
              maxLength={6}
              value={code}
              onChange={e => {
                const val = e.target.value.replace(/\D/g, "").slice(0, 6)
                setCode(val)
                setError("")
              }}
              onKeyDown={e => e.key === "Enter" && handleLogin()}
              className="h-12 w-full rounded-xl border border-border bg-secondary/50 px-4 text-center font-mono text-xl tracking-[0.4em] outline-none focus:ring-2 focus:ring-primary/50 transition-shadow"
              autoFocus
            />
            {error && <p className="text-xs text-destructive text-center">{error}</p>}
          </div>

          <Button
            size="lg"
            className="h-14 rounded-2xl text-base font-bold shadow-lg shadow-primary/15"
            onClick={handleLogin}
            disabled={code.length !== 6}
          >
            <LogIn className="mr-2 size-5" />
            เข้าสู่ระบบ
          </Button>
        </div>
      </div>
    </div>
  )
}
