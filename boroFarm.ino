//BoroFarm v0.2 alpha  Ilya Borodin(c)
//0.001  - сбор данных с датчиков
//0.01 - добавлен вывод данных на экран
//0.02 - фиксы
//0.10  - подключен модуль реле, без алгоритма мониторинга
//0.11 - фиксы
//0.12 - рефакторинг, чистка легаси кода
//0.13 - оптимизация кода
//0.14 - загрузочный экран
//0.15 - переход c delay() на millis для оптимального управления системой
//0.16 - управление подсветкой lcd экрана с кнопки 
//0.17 - подключена звуковая индикация
//0.17.1 - анимация заставки со звуком
//0.20 - алгоритм мониторинга для полива
//0.21 - подключение матрицы кнопок 

#include <DHT.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>


// пины
const int lightIndicatorPin = 0;  //A0  фоторезистор
const int soilMoisturePin = 1;    //A1  датчик влажности почвы
#define DHTPIN 2                  //D2  Датчик DHT11
const int speaker = 3;            //D3  Пищалка
#define DHTTYPE DHT11             //    Используемый тип датчика DHT11
const int pumpPin = 47;           //D47 насос  (реле 1)
const int fanPin = 48;            //D48 вентилятор  (реле 2)
const int lightPin = 49;          //D49 лампы  (реле 3)
#define _LCD_TYPE 1               //D20-D21   для работы с I2C дисплеями
const int button = 22;            //D22 кнопка для управления экраном


// значения с датчиков
int lightIndicatorValue;  // значение с фоторезистора
int illuminationPercent;  // процент освещенности
int soilMoistureValue;    // значение с датчика влажности почвы
int soilMoisturePercent;  // процент влажности почвы
float humidityPercent;    // влажность воздуха
float temperature;        // температура воздуха


// config
bool debugMode = false;  //debug mode - true выводит данные с всех датчиков и переменные в порт в читаемом виде
int cycleTime = 1;       //время цикла в секундах

int wateringDuration = 8;   //длительность полива (сек)
int wateringDelay = 36000;  //задержка полива (сек)   *если полив нужен, но не прошло необходимое время - полив не будет осуществлён  (1 час - 3600 сек, сутки - 86400 сек)

int ventilationDuration = 600;  //длительность проветривания (сек)
int ventilationDelay = 86400;   //задержка проветривания (сек)   *если проветривание нужно, но не прошло необходимое время - проветривание не будет осуществлёно ()

int lcdLightDuration = 10; //длительность подсветки lcd экрана после нажатия кнопки (сек)


// trasholds 
int soilMoisturePercentMin = 40; //минимальный процент влажности почвы
int illuminationPercentMin = 50; //минимальный процент освещенности
int humidityPercentMax = 95; //максимальный процент влажности воздуха
int temperatureMax = 29; //максимальная температура


// служебные переменные
bool needWatering;  //необходимость полива
bool watering;      //полив

bool needVentilation;  //необходимость проветривания
bool ventilation;      //проветривание
bool lcdLight;         //подсветка экрана 

int illuminationPercentOld;  //старые значения датчиков
int soilMoisturePercentOld;
float humidityPercentOld;
float temperatureOld;

//таймеры
unsigned long sensorsTimer; // таймер для датчиков
unsigned long lightTimer;   // таймер для подсветки экрана

DHT dht(DHTPIN, DHTTYPE);
Servo myservo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

//======================================================

void setup() {
  Serial.begin(9600);
  dht.begin();
  lcd.init();
  pinMode(speaker, OUTPUT);
  pinMode(fanPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(lightPin, OUTPUT);
  pinMode(button, INPUT);
  digitalWrite(fanPin, HIGH);
  digitalWrite(pumpPin, HIGH);
  digitalWrite(lightPin, HIGH);

  
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Boro");
  tone(speaker, 261); 
  delay(100);
  lcd.print("Farm ");
  tone(speaker, 329); 
  delay(100);
  lcd.print("v0.17.1");
  tone(speaker, 392); 
  delay(100);
  tone(speaker, 523); 
  delay(100);
  noTone(speaker);
  delay(1200);
  lcd.setCursor(0, 1);
  lcd.print("Ilya Borodin (c)");
  tone(speaker, 1046); 
  delay(200);
  noTone(speaker);
  delay(3000);
  lcd.noBacklight();
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");


  // выключаем все реле (мало ли)
  digitalWrite(fanPin, HIGH);
  digitalWrite(pumpPin, HIGH);
  digitalWrite(lightPin, HIGH);

  sensorsTimer = millis();
  lightTimer = millis();
}

//======================================================

void loop() {


  if (millis() - sensorsTimer > cycleTime * 1000) {

    //освещенность
    lightIndicatorValue = analogRead(lightIndicatorPin);
    illuminationPercent = lightIndicatorValue / 10;
    //обновление данных на экране
    if (illuminationPercentOld != illuminationPercent) {
      illuminationPercentOld = illuminationPercent;
      lcd.setCursor(6, 1);
      lcd.print("     ");
      lcd.setCursor(6, 1);
      lcd.print("i ");
      lcd.print(illuminationPercent);
      lcd.print("%");
    }

    //датчик влажночсти почвы
    soilMoistureValue = analogRead(soilMoisturePin);
    soilMoisturePercent = (-soilMoistureValue + 480) * 100 / 300;  // 462 - показание датчика на воздухе / 260 - политая земля /
    //обновление данных на экране
    if (soilMoisturePercentOld != soilMoisturePercent) {
      soilMoisturePercentOld = soilMoisturePercent;
      lcd.setCursor(6, 0);
      lcd.print("     ");
      lcd.setCursor(6, 0);
      lcd.print("S ");
      lcd.print(soilMoisturePercent);
      lcd.print("%");
    }

    //влажность воздуха
    humidityPercent = dht.readHumidity();
    if (humidityPercentOld != humidityPercent) {
      humidityPercentOld = humidityPercent;
      lcd.setCursor(0, 1);
      lcd.print("     ");
      lcd.setCursor(0, 1);
      lcd.print("H ");
      lcd.print(round(humidityPercent));
      lcd.print("%");
    }

    //температура
    temperature = dht.readTemperature();
    if (temperatureOld != temperature) {
      temperatureOld = temperature;
      lcd.setCursor(0, 0);
      lcd.print("     ");
      lcd.setCursor(0, 0);
      lcd.print("t ");
      lcd.print(round(temperature));
      lcd.print("\337");
    }
    //вывод данных в порт
    if (debugMode) {
      Serial.print(illuminationPercent);
      Serial.print(", ");
      Serial.print(soilMoisturePercent);
      Serial.print(", ");
      Serial.print(humidityPercent);
      Serial.print(", ");
      Serial.println(temperature);
    }  

    sensorsTimer = millis();
  }


  // подсветка экрана 
  if (digitalRead(button) == 1){
    lcdLight = true;
    lcd.backlight();
    lightTimer = millis();
  };

  if (lcdLight && millis() - lightTimer >= lcdLightDuration * 1000) {
    lcd.noBacklight();
    lcdLight = false;
  }

  
}
