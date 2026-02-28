#include <Wire.h>

// Test specific address with requestFrom (READ probe — different from write probe)
void testAddr(uint8_t addr) {
  byte n = Wire.requestFrom(addr, (uint8_t)1);
  while (Wire.available()) Wire.read();
  if (n > 0) {
    Serial.print("  [FOUND] 0x");
    Serial.println(addr, HEX);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Wire.begin(8, 9);
  Wire.setClock(10000);  // 10kHz very slow
  delay(500);

  Serial.println("=== I2C Diagnostic (slow clock) ===");
  Serial.println("Scanning all addresses with requestFrom...");

  bool any = false;
  for (byte addr = 1; addr < 127; addr++) {
    byte n = Wire.requestFrom(addr, (uint8_t)1);
    while (Wire.available()) Wire.read();
    if (n > 0) {
      Serial.print("  FOUND: 0x");
      Serial.print(addr, HEX);
      if (addr == 0x3C || addr == 0x3D) Serial.print(" <- OLED");
      if (addr == 0x68) Serial.print(" <- MPU6050 (AD0=GND)");
      if (addr == 0x69) Serial.print(" <- MPU6050 (AD0=VCC)");
      Serial.println();
      any = true;
    }
  }
  if (!any) Serial.println("  No devices found.");

  // Also report exact error codes at known addresses
  Serial.println();
  Serial.println("Error codes at key addresses:");
  uint8_t addrs[] = {0x3C, 0x3D, 0x68, 0x69};
  for (int i = 0; i < 4; i++) {
    Wire.beginTransmission(addrs[i]);
    Wire.write((uint8_t)0x00);
    byte err = Wire.endTransmission();
    Serial.print("  0x"); Serial.print(addrs[i], HEX);
    Serial.print(" err="); Serial.println(err);
    // err: 0=ACK(found), 2=NACK addr, 3=NACK data, 4=other, 5=timeout
  }
  Serial.println("===================================");
}

void loop() {}
