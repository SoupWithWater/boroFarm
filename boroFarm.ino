#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// === Пины ===
#define LIGHT_INDICATOR_PIN A0  // фоторезистор
#define SOIL_MOISTURE_PIN A1    // датчик влажности почвы
#define DHT_PIN 2               // DHT11
#define SPEAKER_PIN 4           // пищалка
#define PUMP_PIN 47             // насос (реле 1)
#define FAN_PIN 48              // вентилятор (реле 2)
#define LIGHT_PIN 49            // лампы (реле 3)
#define BUTTON_1 22             // кнопка для управления экраном
#define BUTTON_2 23             // кнопка для управления поливом
#define BUTTON_3 24             // кнопка для управления проветриванием
#define BUTTON_4 25             // кнопка для управления освещением

// === Настройки ===
bool debugMode = true;
int cycleTime = 1000;  // время цикла в миллисекундах
int soilMoisturePercentMin = 40;
int illuminationPercentMin = 50;
float humidityPercentMax = 95.0;
float temperatureMax = 29.0;

// === Переменные ===
unsigned long lastCycleTime = 0;
bool lcdBacklight = false;

DHT dht(DHT_PIN, DHT11);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// === Классы ===
class Moisture {
public:
    Moisture(uint8_t pin) : _pin(pin) {
        pinMode(_pin, INPUT);
    }

    int getSoilMoisturePercent() {
        int rawValue = analogRead(_pin);
        return constrain(map(rawValue, 450, 260, 0, 100), 0, 100);
    }

private:
    uint8_t _pin;
};

class DisplayManager {
public:
    void init() {
        lcd.init();
        lcd.backlight();
        welcomeMessage();
    }

    void updateValue(int col, int row, const char* label, float value, const char* unit = "") {
        lcd.setCursor(col, row);
        lcd.print(label);
        lcd.print(value);
        lcd.print(unit);
        lcd.print("   ");  // Очистка остатка
    }

    void toggleBacklight() {
        lcdBacklight = !lcdBacklight;
        lcdBacklight ? lcd.backlight() : lcd.noBacklight();
    }

private:
    void welcomeMessage() {
        lcd.setCursor(0, 0);
        lcd.print("Boro Farm v0.19.1");
        delay(2000);
        lcd.clear();
    }
};

// === Объекты ===
Moisture soilMoistureSensor(SOIL_MOISTURE_PIN);
DisplayManager display;

// === Функции ===
void setup() {
    Serial.begin(9600);
    dht.begin();
    display.init();

    // Настройка реле
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(FAN_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);

    // Отключение реле
    digitalWrite(PUMP_PIN, HIGH);
    digitalWrite(FAN_PIN, HIGH);
    digitalWrite(LIGHT_PIN, HIGH);
}

void loop() {
    unsigned long currentMillis = millis();

    // Цикл считывания данных
    if (currentMillis - lastCycleTime >= cycleTime) {
        // Чтение датчиков
        int illuminationPercent = analogRead(LIGHT_INDICATOR_PIN) / 10;
        int soilMoisturePercent = soilMoistureSensor.getSoilMoisturePercent();
        float humidityPercent = dht.readHumidity();
        float temperature = dht.readTemperature();

        // Обновление экрана
        display.updateValue(0, 0, "T:", temperature, "C");
        display.updateValue(0, 1, "H:", humidityPercent, "%");
        display.updateValue(8, 0, "L:", illuminationPercent, "%");
        display.updateValue(8, 1, "M:", soilMoisturePercent, "%");

        // Вывод в порт
        if (debugMode) {
            Serial.print("Illumination: ");
            Serial.print(illuminationPercent);
            Serial.print("%, Soil Moisture: ");
            Serial.print(soilMoisturePercent);
            Serial.print("%, Humidity: ");
            Serial.print(humidityPercent);
            Serial.print("%, Temp: ");
            Serial.print(temperature);
            Serial.println("C");
        }

        lastCycleTime = currentMillis;
    }

    // Обработка кнопок
    if (digitalRead(BUTTON_1) == HIGH) {
        display.toggleBacklight();
    }
}
