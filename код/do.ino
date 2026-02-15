#include <Servo.h>
#include <SPI.h>
#include <RF24.h>

// --- Объекты ---
Servo servoX;  // Нижний сервопривод (ось X)
Servo servoY;  // Верхний сервопривод (ось Y)
RF24 radio(10, 9);  // CE=10, CSN=9

// Адреса для двусторонней связи
const byte addressRX[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7}; // приём команд
const byte addressTX[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2}; // отправка отчётов
// --- Пины ---
const int servoX_pin = 5;   // Нижний сервопривод (ось X)
const int servoY_pin = 6;   // Верхний сервопривод (ось Y)
const int laser_pin = 7;    // Лазер

// Структура для отправки отчета об углах
struct FeedbackPacket {
  byte servoX_angle;
  byte servoY_angle;
};

// --- Вспомогательная функция для отправки отчета ---
void sendFeedback() {
  radio.stopListening(); // Переключаемся в режим передачи

  FeedbackPacket feedback;
  feedback.servoX_angle = servoX.read();
  feedback.servoY_angle = servoY.read();

  radio.write(&feedback, sizeof(FeedbackPacket));
  radio.startListening(); // Снова слушаем команды
}

void setup() {
  Serial.begin(9600);
  radio.printDetails();
  radio.setAutoAck(true);        // Включить подтверждение получения
  radio.setRetries(15, 15);      // 15 попыток с задержкой 15*250 мкс
  radio.setCRCLength(RF24_CRC_16); // Включить CRC-проверку
  Serial.println("=== NRF24L01 Приемник ===");

  // Инициализация сервоприводов
  servoX.attach(servoX_pin);
  servoY.attach(servoY_pin);
  servoX.write(90);
  servoY.write(90);

  pinMode(laser_pin, OUTPUT);
  digitalWrite(laser_pin, LOW);

  // Инициализация радиомодуля
  if (!radio.begin()) {
    Serial.println("Ошибка: Модуль NRF24L01 не отвечает!");
    while (1) {}
  }

  // --- Явная настройка канала и скорости (одинаково с передатчиком) ---
  radio.setChannel(76);          // Канал 76 (2.476 ГГц)
  radio.setDataRate(RF24_1MBPS); // Скорость 1 Mbps
  radio.setPALevel(RF24_PA_MIN); // Минимальная мощность (для тестов)

  // Настройка двусторонней связи
  radio.openWritingPipe(addressTX);    // Труба для отправки отчетов
  radio.openReadingPipe(1, addressRX); // Труба для приема команд
  radio.startListening(); // Начинаем слушать

  Serial.println("Приемник готов к работе!");
}

void loop() {
  if (radio.available()) {
    byte commandID;
    radio.read(&commandID, sizeof(commandID));

    Serial.print("[Прием] Команда: ");
    Serial.println(commandID);

    switch (commandID) {
      case 1: turnX(); break;
      case 2: turnY(); break;
      case 3: turnXY(); break;
      case 4: turn_negXY(); break;
      default:
        Serial.println("[Ошибка] Неизвестная команда");
        return;
    }
    Serial.println("[Статус] Последовательность завершена.\n");
  }
}

// --- Далее идут функции turnX, turnY, turnXY, turn_negXY (без изменений) ---
void turnX() {
  Serial.println("-> Движение по оси X");
  digitalWrite(laser_pin, HIGH);
  for (int angle = 90; angle <= 130; angle += 10) { servoX.write(angle); delay(500); sendFeedback(); }
  for (int angle = 130; angle >= 50; angle -= 10) { servoX.write(angle); delay(500); sendFeedback(); }
  for (int angle = 50; angle <= 90; angle += 10)  { servoX.write(angle); delay(500); sendFeedback(); }
  digitalWrite(laser_pin, LOW);
}

void turnY() {
  Serial.println("-> Движение по оси Y");
  digitalWrite(laser_pin, HIGH);
  for (int angle = 90; angle <= 130; angle += 10) { servoY.write(angle); delay(500); sendFeedback(); }
  for (int angle = 130; angle >= 50; angle -= 10) { servoY.write(angle); delay(500); sendFeedback(); }
  for (int angle = 50; angle <= 90; angle += 10)  { servoY.write(angle); delay(500); sendFeedback(); }
  digitalWrite(laser_pin, LOW);
}

void turnXY() {
  Serial.println("-> Синхронное движение X и Y");
  digitalWrite(laser_pin, HIGH);
  for (int angle = 90; angle <= 130; angle += 10) { servoX.write(angle); servoY.write(angle); delay(500); sendFeedback(); }
  for (int angle = 130; angle >= 50; angle -= 10) { servoX.write(angle); servoY.write(angle); delay(500); sendFeedback(); }
  for (int angle = 50; angle <= 90; angle += 10)  { servoX.write(angle); servoY.write(angle); delay(500); sendFeedback(); }
  digitalWrite(laser_pin, LOW);
}

void turn_negXY() {
  Serial.println("-> Противофазное движение");
  digitalWrite(laser_pin, HIGH);
  for (int i = 0; i <= 4; i++) {
    servoX.write(90 - i * 10);
    servoY.write(90 + i * 10);
    delay(500); sendFeedback();
  }
  for (int i = 3; i >= 0; i--) {
    servoX.write(90 - i * 10);
    servoY.write(90 + i * 10);
    delay(500); sendFeedback();
  }
  digitalWrite(laser_pin, LOW);
}
