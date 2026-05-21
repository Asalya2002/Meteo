#include <ESP8266WiFi.h>         // Основная библиотека для работы Wi-Fi на ESP8266
#include <WiFiClientSecure.h>    // Библиотека для защищенного соединения (SSL)
#include <UniversalTelegramBot.h> // Библиотека для взаимодействия с Telegram API

// Настройки подключения к сети Wi-Fi
const char* ssid = "iPhone";      // Имя точки доступа
const char* password = "miss2002"; // Пароль от сети

// Параметры Telegram бота
const char* botToken = "8784895599:AAH3p1WyFzdNDdPO_ag3XnYD49CbSY09WqA"; // Токен бота
const char* chatId = "156112560";                                     // Ваш ID пользователя

// Инициализация защищенного клиента и бота
WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

// Флаг состояния, чтобы контролировать отправку сообщений в чат
bool gasAlertSent = false; 

void setup() {
  Serial.begin(9600); // Инициализация порта для приема данных от Arduino (пины RX/TX)
  Serial.setTimeout(50); // Уменьшаем таймаут ожидания строки, чтобы код работал мгновенно
  
  // Подключение к беспроводной сети
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  // Ожидание установки соединения
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  // Настройка безопасности (игнорирование проверки SSL-сертификата для скорости)
  client.setInsecure();
  
  // Отправка уведомления об успешном запуске системы
  bot.sendMessage(chatId, "🤖 Станция мониторинга активна и подключена к сети!", "");
}

void loop() {
  // Проверка наличия входящих команд от Arduino
  if (Serial.available()) {
    String message = Serial.readStringUntil('\n'); // Чтение входящей строки
    message.trim(); // Удаление лишних пробелов и скрытых символов переноса строки

    // Логика обработки команды ALERT_GAS
    if (message == "ALERT_GAS") {
      // Отправляем сообщение, только если оно еще не было отправлено в текущем цикле тревоги
      if (!gasAlertSent) {
        if (bot.sendMessage(chatId, "⚠️ Внимание! Обнаружена загазованность в помещении!", "")) {
          gasAlertSent = true; // Блокируем повторную отправку до очищения воздуха
        }
      }
    }
    
    // Логика обработки команды OK_GAS
    else if (message == "OK_GAS") {
      // Отправляем сообщение об успешной очистке, только если до этого была тревога
      if (gasAlertSent) {
        if (bot.sendMessage(chatId, "✅ Воздух пришел в норму. Опасность миновала.", "")) {
          gasAlertSent = false; // Сбрасываем флаг, система снова готова к алармам
        }
      }
    }
  }
}
