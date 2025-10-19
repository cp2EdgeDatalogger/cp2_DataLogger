#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// -------- CONFIGURAÇÕES DE HARDWARE ----------
#define SERIAL_OPTION 1
#define UTC_OFFSET -3

#define DHT22_PIN 2
#define DHTTYPE DHT22

const int ledG = 3;
const int ledY = 4;
const int ledR = 5;
const int buzzer = A1;
const int ldrPin = A0;

// LCD 20x4 no endereço 0x27
LiquidCrystal_I2C lcd(0x27, 20, 4);

// RTC
RTC_DS1307 RTC;

// DHT
DHT dht22(DHT22_PIN, DHTTYPE);

// Teclado 4x4 (pinos 13..6)
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
// Defina claramente os pinos físicos do Arduino que estão ligados ao teclado:
byte rowPins[ROWS] = {13, 12, 11, 10}; // L1, L2, L3, L4
byte colPins[COLS] = {9, 8, 7, 6};     // C1, C2, C3, C4

Keypad teclado = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// temporizadores
unsigned long lastSensorMillis = 0;
const unsigned long sensorInterval = 2000; // leitura a cada 2s

void setup() {
  // inicializações físicas
  if (SERIAL_OPTION) Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  dht22.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Sistema iniciado");
  delay(700);
  lcd.clear();

  // pinos
  pinMode(ledG, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  digitalWrite(ledG, LOW);
  digitalWrite(ledY, LOW);
  digitalWrite(ledR, LOW);

  // ativa pull-ups nas linhas/colunas do teclado pra reduzir ruido
  for (int i = 6; i <= 13; i++) {
    pinMode(i, INPUT_PULLUP);
  }
}

void loop() {
  // ---------- 1) Checar tecla -----------
  char tecla = teclado.getKey();
  if (tecla =='1') {
    // Mostrar tecla no LCD e Serial
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("MENU DE ESTATISTICAS");

    // bip curto no buzzer ao apertar
    digitalWrite(buzzer, HIGH);
    delay(120);
    digitalWrite(buzzer, LOW);

    delay(200); // anti-bounce simples

    //VAI MOSTRAR AS ESSTATÍSTICAS EM TELA!
    unsigned long nowMillis = millis();
  if (nowMillis - lastSensorMillis >= sensorInterval) {
    lastSensorMillis = nowMillis;

    // DHT11/22
    float humi  = dht22.readHumidity();
    float tempC = dht22.readTemperature(); // Celsius

    // LDR
    int ldrValue = analogRead(ldrPin);
    // normaliza brilho de 0..1023 para 0..100 (apenas para exibição)
    int lumPercent = map(ldrValue, 0, 1023, 100, 0);
    if (lumPercent < 0) lumPercent = 0;
    if (lumPercent > 100) lumPercent = 100;

    // Hora do RTC ajustada
    DateTime now = RTC.now();
    uint32_t timestamp = now.unixtime() + (UTC_OFFSET * 3600);
    DateTime adjustedTime = DateTime(timestamp);

    // Mostrar no LCD (linhas 0..3)
    lcd.setCursor(0,1);
    lcd.print(adjustedTime.day());
    lcd.print("/");
    lcd.print(adjustedTime.month());
    lcd.print("/");
    lcd.print(adjustedTime.year());
    lcd.print(" ");
    // hora
    if (adjustedTime.hour() < 10) lcd.print('0');
    lcd.print(adjustedTime.hour());
    lcd.print(':');
    if (adjustedTime.minute() < 10) lcd.print('0');
    lcd.print(adjustedTime.minute());

    lcd.setCursor(0,2);
    if (isnan(humi) || isnan(tempC)) {
      lcd.print("DHT erro");
    } else {
      lcd.print("T:");
      lcd.print(tempC, 1);
      lcd.print("C U:");
      lcd.print(humi, 0);
      lcd.print("%");
    }

    lcd.setCursor(0,3);
    lcd.print("Luz:");
    lcd.print(lumPercent);
    lcd.print("%   ");

    // lógica de leds e buzzer (mesma que você queria, porém sem bloquear longos delays)
    if (isnan(humi) || isnan(tempC)) {
      // erro leitura -> pisca amarelo
      digitalWrite(ledY, HIGH);
      delay(200);
      digitalWrite(ledY, LOW);
    } else {
      if (humi <= 20 || humi >= 90 || lumPercent >= 90 || tempC >= 20 || tempC <= 0) {
        // CRITICAL
        if (SERIAL_OPTION) Serial.println("critical");
        digitalWrite(ledR, HIGH);
        tone(buzzer, 5000, 500); // bip longo
        delay(600);
        noTone(buzzer);
        digitalWrite(ledR, LOW);
      } else if (humi < 60 || humi > 80 || lumPercent > 60 || tempC > 16 || tempC < 10) {
        // WARNING
        if (SERIAL_OPTION) Serial.println("warning");
        digitalWrite(ledY, HIGH);
        tone(buzzer, 1000, 300);
        delay(350);
        noTone(buzzer);
        digitalWrite(ledY, LOW);
      } else {
        // OK
        digitalWrite(ledG, HIGH);
        delay(300);
        digitalWrite(ledG, LOW);
      }
    }}

  }
  else if(tecla == '2'){
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("marcar");
  }

}   
