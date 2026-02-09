// КОНТРОЛЛЕР
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

RF24 radio(9, 10); // CE, CSN

const uint8_t pipeToLaser[6]   = "LASER";
const uint8_t pipeFromLaser[6] = "CTRLR";

struct LaserPacket {
  uint8_t deviceId;
  int16_t panAngle;
  int16_t tiltAngle;
  uint8_t mode;
};

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_2MBPS);
  radio.setRetries(3, 5); // небольшие ретраи для надёжности

  radio.openReadingPipe(1, pipeFromLaser);
  radio.startListening();

  Serial.println("📡 Контроллер запущен. Готов к приёму телеметрии.");
  Serial.println("Команды: start, laseron, laseroff, vertos, goros");
}

void loop() {
  if (radio.available()) {
    LaserPacket pkt;
    radio.read(&pkt, sizeof(pkt)); // <-- теперь без if()

    Serial.print("📍 [ID=");
    Serial.print(pkt.deviceId);
    Serial.print("] PAN=");
    Serial.print(pkt.panAngle);
    Serial.print("°, TILT=");
    Serial.print(pkt.tiltAngle);
    Serial.print("°, MODE=");
    Serial.println(pkt.mode);
  }
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd == "start" || cmd == "laseron" || cmd == "laseroff" || 
        cmd == "vertos" || cmd == "goros") {
      radio.stopListening();
      radio.openWritingPipe(pipeToLaser);
      radio.write(cmd.c_str(), cmd.length() + 1);
      Serial.print("✅ Отправлено: ");
      Serial.println(cmd);
      radio.openReadingPipe(1, pipeFromLaser);
      radio.startListening();
    } else {
      Serial.println("⚠️ Неизвестная команда");
    }
  }
}
