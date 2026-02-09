//ЛАЗЕРНЫЙ МОДУЛЬ
#include <SPI.h>
#include <RF24.h>
#include <Servo.h>

const int PAN_PIN = 5;
const int TILT_PIN = 6;
const int LASER_PIN = 2;

Servo servoPan;
Servo servoTilt;
RF24 radio(9, 10);

const uint8_t pipeFromController[6] = "LASER";
const uint8_t pipeToController[6] = "CTRLR";

bool scanning = false;
bool laserOn = true;

const int routePan[] = {
  0, -10, -20, -30, -40, -30, -20, -10, 0, 10, 20, 30, 40, 30, 20, 10, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  -10, -20, -30, -40, -30, -20, -10, 0, 10, 20, 30, 40,
  30, 20, 10, 0, -10, -20, -30, -40,
  -30, -20, -10, 0,
  10, 20, 30, 40,
  30, 20, 10, 0
};

const int routeTilt[] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, -10, -20, -30, -40, -30, -20, -10, 0, 10, 20, 30, 40, 30, 20, 10, 0,
  -10, -20, -30, -40, -30, -20, -10, 0, 10, 20, 30, 40,
  40, 40, 40, 40, 40, 40, 40, 40,
  30, 20, 10, 0,
  -10, -20, -30, -40,
  -30, -20, -10, 0
};

const int routeLength = sizeof(routePan) / sizeof(routePan[0]);

int routeIndex = 0;
unsigned long lastAction = 0;
const unsigned long scanInterval = 3000;

struct LaserPacket {
  uint8_t deviceId = 1;
  int16_t panAngle = 0;
  int16_t tiltAngle = 0;
  uint8_t mode = 0;
};

int angleToServo(int angle) {
  return map(angle, -40, 40, 50, 130);
}

void setLaserPosition(int pan, int tilt) {
  servoPan.write(angleToServo(pan));
  servoTilt.write(angleToServo(tilt));
}

void updateLaser() {
  digitalWrite(LASER_PIN, laserOn ? HIGH : LOW);
}

void setup() {
  Serial.begin(9600);
  servoPan.attach(PAN_PIN);
  servoTilt.attach(TILT_PIN);
  pinMode(LASER_PIN, OUTPUT);
  
  laserOn = true;
  updateLaser();
  
  // Установка начальных позиций сервоприводов согласно ТЗ:
  // Пин 5 (PAN) → 120°, Пин 6 (TILT) → 110°
  servoPan.write(120);
  servoTilt.write(110);
  delay(500); // Пауза для завершения движения сервоприводов

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_2MBPS);
  radio.setRetries(3, 5);
  radio.openReadingPipe(1, pipeFromController);
  radio.startListening();

  Serial.println("✅ Лазер готов. Ожидание команды 'start'.");
}

void sendTelemetry(int pan, int tilt, uint8_t mode) {
  LaserPacket pkt;
  pkt.deviceId = 1;
  pkt.panAngle = pan;
  pkt.tiltAngle = tilt;
  pkt.mode = mode;

  radio.stopListening();
  radio.openWritingPipe(pipeToController);
  radio.write(&pkt, sizeof(pkt));
  radio.startListening();
  radio.openReadingPipe(1, pipeFromController);

  Serial.print("📤 PAN=");
  Serial.print(pan);
  Serial.print(", TILT=");
  Serial.print(tilt);
  Serial.print(", MODE=");
  Serial.println(mode);
}

void performScanStep() {
  if (!scanning) return;
  if (millis() - lastAction < scanInterval) return;

  if (routeIndex >= routeLength) {
    setLaserPosition(0, 0);
    sendTelemetry(0, 0, 0);
    scanning = false;
    routeIndex = 0;
    Serial.println("🏁 Сканирование завершено.");
    return;
  }

  int pan = routePan[routeIndex];
  int tilt = routeTilt[routeIndex];

  setLaserPosition(pan, tilt);
  sendTelemetry(pan, tilt, 1);

  routeIndex++;
  lastAction = millis();
}

void handleCommand(const char* cmd) {
  Serial.print("📥 ");
  Serial.println(cmd);

  if (strcmp(cmd, "start") == 0) {
    if (!scanning) {
      scanning = true;
      routeIndex = 0;
      lastAction = millis() - scanInterval;
      Serial.println("🟢 Сканирование запущено");
    }
  } else if (strcmp(cmd, "laseron") == 0) {
    laserOn = true; updateLaser(); Serial.println("🔦 ON");
  } else if (strcmp(cmd, "laseroff") == 0) {
    laserOn = false; updateLaser(); Serial.println("🌑 OFF");
  } else if (strcmp(cmd, "vertos") == 0) {
    setLaserPosition(0, 5); delay(200); setLaserPosition(0, 0); Serial.println("↕️ vertos");
  } else if (strcmp(cmd, "goros") == 0) {
    setLaserPosition(5, 0); delay(200); setLaserPosition(0, 0); Serial.println("↔️ goros");
  }
}

void loop() {
  if (radio.available()) {
    char cmd[16] = {0};
    radio.read(cmd, sizeof(cmd));
    handleCommand(cmd);
  }
  if (scanning) performScanStep();
}
