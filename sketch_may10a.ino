#include <ESP8266WiFi.h>       // Основная библиотека для работы Wi-Fi на ESP8266
#include <WiFiClientSecure.h>  // Библиотека для защищенного соединения (SSL)
#include <UniversalTelegramBot.h> // Библиотека для взаимодействия с Telegram API

// Настройки подключения к сети Wi-Fi
const char* ssid = "SAR1979";      // Имя точки доступа
const char* password = "998222619"; // Пароль от сети

// Параметры Telegram бота
const char* botToken = "8784895599:AAH3p1WyFzdNDdPO_ag3XnYD49CbSY09WqA"; // Уникальный токен вашего бота
const char* chatId = "156112560";     // Ваш ID пользователя в Telegram

// Инициализация защищенного клиента и бота
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

void setup() {
  Serial.begin(9600); // Инициализация порта для приема данных от Arduino
  
  // Подключение к беспроводной сети
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Ожидание установки соединения
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Настройка безопасности (игнорирование проверки сертификата для упрощения)
  client.setInsecure();
  
  // Отправка уведомления об успешном запуске системы в облако
  bot.sendMessage(chatId, "🤖 Станция мониторинга активна и подключена к сети!", "");
}

void loop() {
  // Проверка наличия входящих команд от Arduino по последовательному порту
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n'); // Чтение входящей строки
    message.trim(); // Удаление лишних пробелов

    // Логика обработки команды ALERT_GAS (Загазованность)
    if (message == "ALERT_GAS") {
      bot.sendMessage(chatId, "⚠️ Внимание! Обнаружена загазованность в помещении!", "");
    }
  }
}
