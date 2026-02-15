#include <SPI.h>
#include <RF24.h>

RF24 radio(10, 9); // CE=10, CSN=9

// Адреса для двусторонней связи
const byte addressTX[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7}; // для отправки
const byte addressRX[5] = {0xC2, 0xC2, 0xC2, 0xC2, 0xC2}; // для приёма


// Структура для приема отчета
struct FeedbackPacket {
  byte servo1_angle;
  byte servo2_angle;
};

void setup() {
  Serial.begin(9600);
  radio.printDetails();
  radio.setAutoAck(true);        // Включить подтверждение получения
  radio.setRetries(15, 15);      // 15 попыток с задержкой 15*250 мкс
  radio.setCRCLength(RF24_CRC_16); // Включить CRC-проверку
  Serial.println("=== NRF24L01 Передатчик ===");
  Serial.println("Введите 1-4 для отправки команды");

  if (!radio.begin()) {
    Serial.println("Ошибка: Модуль NRF24L01 не отвечает!");
    while (1) {}
  }

  // --- Явная настройка канала и скорости (одинаково с приемником) ---
  radio.setChannel(76);          // Канал 76 (2.476 ГГц)
  radio.setDataRate(RF24_1MBPS); // Скорость 1 Mbps
  radio.setPALevel(RF24_PA_MIN); // Минимальная мощность

  // Настройка двусторонней связи
  radio.openWritingPipe(addressTX);     // Труба для отправки команд
  radio.openReadingPipe(1, addressRX);  // Труба для приема отчетов
  radio.startListening(); // Начинаем слушать отчеты
}

void loop() {
  // --- Прием отчетов от приемника ---
  if (radio.available()) {
    FeedbackPacket feedback;
    radio.read(&feedback, sizeof(FeedbackPacket));
    Serial.print("[Прием] Серво1: ");
    Serial.print(feedback.servo1_angle);
    Serial.print("°, Серво2: ");
    Serial.print(feedback.servo2_angle);
    Serial.println("°");
  }

  // --- Отправка команд через Serial Monitor ---
  if (Serial.available() > 0) {
    char commandChar = Serial.read();
    if (commandChar == '\n' || commandChar == '\r') return;

    byte commandToSend = commandChar - '0';
    if (commandToSend >= 1 && commandToSend <= 4) {
      radio.stopListening(); // Переключаемся в режим передачи

      if (radio.write(&commandToSend, sizeof(commandToSend))) {
        Serial.print("[Отправка] Команда '");
        Serial.print(commandToSend);
        Serial.println("' успешно отправлена!");
      } else {
        Serial.println("[Ошибка] Не удалось отправить команду!");
      }

      radio.startListening(); // Возвращаемся в режим приема
    } else {
      Serial.println("[Ошибка] Введите команду от 1 до 4");
    }
  }
}
