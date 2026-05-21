#include <Wire.h> // Протокол I2C для дисплея и датчика BME280
#include <Adafruit_BME280.h> // Библиотека для метеосенсора BME280
#include <LiquidCrystal_I2C.h> // Библиотека для управления ЖК-дисплеем
#include <SoftwareSerial.h> // Библиотека для создания виртуальных портов связи

// --- ОПРЕДЕЛЕНИЕ ПОРТОВ ---
SoftwareSerial jwSerial(7, 8); // Порт для датчика качества воздуха JW01 (7 - RX, 8 - TX)
SoftwareSerial telegramSerial(6, 11); // Связь с Wemos D1 Mini (6 - RX, 11 - TX)

const int PIN_MQ2 = A0; // Аналоговый вход для датчика газа и дыма
const int PIN_BUZZER = 9; // Пин для звуковой сирены (зуммера)
const int PIN_RELAY = 2; // Пин для управления модулем реле

// Пины для RGB-светодиода (Индикация статуса)
const int PIN_RED = 3; // Красный канал (Опасность)
const int PIN_GREEN = 4; // Зеленый канал (Норма)
const int PIN_BLUE = 5; // Синий канал

// --- НАСТРОЙКА ПОРОГОВ БЕЗОПАСНОСТИ ---
const int ABSOLUTE_GAS_LIMIT = 230; // Аварийный порог для MQ-2 

// --- ОБЪЕКТЫ И ПЕРЕМЕННЫЕ ---
Adafruit_BME280 bme;
LiquidCrystal_I2C lcd(0x27, 16, 2); // Адрес дисплея 0x27, размер 16x2
int air_clean_level = 0; // Переменная для хранения фонового значения чистого воздуха
bool jwAlert = false;    // Флаг опасности от датчика JW01

// Флаг-предохранитель против спама в Telegram (чтобы слать команду только ОДИН раз)
bool alertSent = false; 

void setup() {
  // Инициализация последовательных портов
  Serial.begin(9600); // Монитор порта для отладки в ПК
  jwSerial.begin(9600); // Чтение данных с JW01
  telegramSerial.begin(9600); // Передача команд в Telegram (через Wemos)
  
  // Настройка всех исполнительных пинов на выход
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  
  // Установка начального безопасного состояния (всё выключено)
  digitalWrite(PIN_BUZZER, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_RED, LOW);
  digitalWrite(PIN_GREEN, LOW);
  
  // Запуск дисплея
  lcd.init();
  lcd.backlight();

  // Запуск метеодатчика BME280
  if (!bme.begin(0x76)) {
    Serial.println("Ошибка BME280! Проверьте подключение.");
  }
  
  // Автоматическая калибровка MQ-2 (берем среднее значение за 1 секунду)
  long sum = 0;
  for(int i = 0; i < 10; i++) {
    sum += analogRead(PIN_MQ2);
    delay(100);
  }
  air_clean_level = sum / 10; // Устанавливаем текущий уровень воздуха как "норму"
  
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM READY");
  delay(2000);
}

void loop() {
  // 1. СБОР ДАННЫХ С ДАТЧИКОВ
  float temp = bme.readTemperature(); // Получаем температуру
  float hum = bme.readHumidity(); // Получаем влажность
  float pres = bme.readPressure() * 0.00750062; // Перевод давления в мм рт.ст.
  int gasVal = analogRead(PIN_MQ2); // Считываем уровень загазованности с MQ-2

  // --- ОБРАБОТКА ДАННЫХ С ДАТЧИКА JW01 ---
  // Безопасный опрос: очищаем буфер, чтобы плата не висла, и фиксируем активность
  jwAlert = false;
  if (jwSerial.available() > 0) {
    while(jwSerial.available() > 0) {
      char c = jwSerial.read();
    }
    jwAlert = true; 
  }

  // 2. ВЫВОД КЛИМАТИЧЕСКИХ ДАННЫХ НА LCD (Верхняя строка)
  lcd.setCursor(0, 0);
  lcd.print("T:"); lcd.print(temp, 0);
  lcd.print(" H:"); lcd.print(hum, 0);
  lcd.print("% P:"); lcd.print(pres, 0);
  lcd.print(" ");

  // 3. АНАЛИЗ БЕЗОПАСНОСТИ И УПРАВЛЕНИЕ АВТОМАТИКОЙ (Нижняя строка)
  lcd.setCursor(0, 1);
  
  // КОМБИНИРОВАННОЕ УСЛОВИЕ ТРЕВОГИ
  if (gasVal > (air_clean_level + 80) || gasVal > ABSOLUTE_GAS_LIMIT || jwAlert == true) {
    
    // --- РЕЖИМ ТРЕВОГИ ---
    lcd.print("AIR: DANGER!    ");
    digitalWrite(PIN_BUZZER, HIGH); // Активируем звуковую сирену
    digitalWrite(PIN_RELAY, HIGH); // Включаем реле аварийной вентиляции
    
    // Цветовая индикация статуса: КРАСНЫЙ
    digitalWrite(PIN_RED, HIGH);
    digitalWrite(PIN_GREEN, LOW);
    
    // Защита от спама: отправляем маркер на Wemos ТОЛЬКО один раз при фиксации аварии
    if (!alertSent) {
      telegramSerial.println("ALERT_GAS"); 
      alertSent = true; 
    }
  }
  else {
    // --- ШТАТНЫЙ РЕЖИМ ---
    lcd.print("AIR: CLEAN      ");
    digitalWrite(PIN_BUZZER, LOW); // Отключаем сирену
    digitalWrite(PIN_RELAY, LOW); // Выключаем реле
    
    // Цветовая индикация статуса: ЗЕЛЕНЫЙ
    digitalWrite(PIN_RED, LOW);
    digitalWrite(PIN_GREEN, HIGH);

    // Защита от спама: отправляем сброс на Wemos ТОЛЬКО один раз, когда всё очистилось
    if (alertSent) {
      telegramSerial.println("OK_GAS"); 
      alertSent = false; 
    }
  }

  // Вывод текущих показаний в Монитор порта для отладки
  Serial.print("MQ-2: "); Serial.print(gasVal);
  Serial.print(" | JW01 Status: "); Serial.println(jwAlert ? "ACTIVE" : "IDLE");

  // Стабилизация вывода на экран
  delay(1500);
}
