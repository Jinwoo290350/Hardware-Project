// =============================================
// user-store.ts — localStorage-based user auth
// Each user gets a 6-digit code as their "password"
// History persists per user across sessions
// =============================================

const USERS_KEY   = "safestep_users"
const SESSION_KEY = "safestep_session"

export interface StoredEvent {
  id:     string
  type:   string
  label:  string
  time:   string   // ISO string (Date serialized for localStorage)
  status: string
}

export interface UserProfile {
  code:      string
  name:      string
  createdAt: string
  history:   StoredEvent[]
}

// ── Helpers ──────────────────────────────────────────────────

function isClient() {
  return typeof window !== "undefined"
}

function loadUsers(): Record<string, UserProfile> {
  if (!isClient()) return {}
  try {
    const raw = localStorage.getItem(USERS_KEY)
    return raw ? (JSON.parse(raw) as Record<string, UserProfile>) : {}
  } catch {
    return {}
  }
}

function saveUsers(users: Record<string, UserProfile>) {
  if (!isClient()) return
  localStorage.setItem(USERS_KEY, JSON.stringify(users))
}

function generateUniqueCode(): string {
  const users = loadUsers()
  let code: string
  do {
    code = Math.floor(100000 + Math.random() * 900000).toString()
  } while (users[code])
  return code
}

// ── Public API ───────────────────────────────────────────────

/** สร้าง user ใหม่ → คืน 6-digit code */
export function registerUser(name: string): string {
  const users = loadUsers()
  const code  = generateUniqueCode()
  users[code] = {
    code,
    name:      name.trim(),
    createdAt: new Date().toISOString(),
    history:   [],
  }
  saveUsers(users)
  return code
}

/** ตรวจสอบรหัส → คืน UserProfile หรือ null ถ้าไม่พบ */
export function loginUser(code: string): UserProfile | null {
  const users = loadUsers()
  return users[code] ?? null
}

/** อ่าน session ที่บันทึกไว้ */
export function getSession(): string | null {
  if (!isClient()) return null
  try { return localStorage.getItem(SESSION_KEY) } catch { return null }
}

/** บันทึก session */
export function setSession(code: string) {
  if (!isClient()) return
  localStorage.setItem(SESSION_KEY, code)
}

/** ล้าง session (logout) */
export function clearSession() {
  if (!isClient()) return
  localStorage.removeItem(SESSION_KEY)
}

/** อ่าน UserProfile จาก session ปัจจุบัน */
export function getCurrentUser(): UserProfile | null {
  const code = getSession()
  return code ? loginUser(code) : null
}

/** บันทึก history ของ user */
export function saveUserHistory(code: string, history: StoredEvent[]) {
  const users = loadUsers()
  if (users[code]) {
    users[code].history = history
    saveUsers(users)
  }
}

/** โหลด history ของ user */
export function loadUserHistory(code: string): StoredEvent[] {
  const users = loadUsers()
  return users[code]?.history ?? []
}

/** ดึงชื่อ user จาก code */
export function getUserName(code: string): string {
  const users = loadUsers()
  return users[code]?.name ?? "User"
}
