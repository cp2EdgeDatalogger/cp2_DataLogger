#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

// -------- CONFIGURAÇÕES DE HARDWARE ----------
#define SERIAL_OPTION 1
#define UTC_OFFSET -3

#define DHT11_PIN 2
#define DHTTYPE DHT22  // corrigido, pois você usa DHT22

const int ledG = 3;
const int ledY = 4;
const int ledR = 5;
const int buzzer = A1;
const int ldrPin = A0;

// variáveis globais de média
int nloops = 0;
float somaLdr = 0;
float somaTemp = 0;
float somaHumi = 0;
float mediaLdr = 0;
float mediaTemp = 0;
float mediaHumi = 0;

// flags
bool flagTempAlta = false;
bool flagTempBaixa = false;
bool flagUmidAlta = false;
bool flagUmidBaixa = false;
bool flagLumAlta = false;

// LCD 20x4 no endereço 0x27
LiquidCrystal_I2C lcd(0x27, 20, 4);

// RTC
RTC_DS1307 RTC;

// DHT
DHT dht11(DHT11_PIN, DHTTYPE);

// Teclado 4x4 (pinos 13..6)
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13, 12, 11, 10};
byte colPins[COLS] = {9, 8, 7, 6};
Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned long lastSensorMillis = 0;
const unsigned long sensorInterval = 1000; // atualizar a cada 1 segundo

enum Modo { MENU_PRINCIPAL, ESTATISTICAS, MARCADOR, RELOGIO };
Modo modoAtual = MENU_PRINCIPAL;

void mostrarMenuPrincipal() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Selecione uma opcao:"));
  lcd.setCursor(0, 1);
  lcd.print(F("1 - Estatisticas"));
  lcd.setCursor(0, 2);
  lcd.print(F("2 - Marcador"));
  lcd.setCursor(0, 3);
  lcd.print(F("3 - Relogio"));
}

void setup() {
  if (SERIAL_OPTION) Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  dht11.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print(F("Sistema iniciado"));
  delay(800);

  pinMode(ledG, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  for (int i = 6; i <= 13; i++) pinMode(i, INPUT_PULLUP);

  mostrarMenuPrincipal();

  // Banner ASCII otimizado pra flash
  Serial.println(F(" __      ___       _          _      _                                  _ _       "));
  Serial.println(F(" \\ \\    / (_)     | |        (_)    (_)           /\\                   | | |      "));
  Serial.println(F("  \\ \\  / / _ _ __ | |__   ___ _ _ __ _  __ _     /  \\   __ _ _ __   ___| | | ___  "));
  Serial.println(F("   \\ \\/ / | | '_ \\| '_ \\ / _ \\ | '__| |/ _` |   / /\\ \\ / _` | '_ \\ / _ \\ | |/ _ \\ "));
  Serial.println(F("    \\  /  | | | | | | | |  __/ | |  | | (_| |  / ____ \\ (_| | | | |  __/ | | (_) |"));
  Serial.println(F("     \\/   |_|_| |_|_| |_|\\___|_|_|  |_|\\__,_| /_/    \\_\\__, |_| |_|\\___|_|_|\\___/ "));
  Serial.println(F("                                                        __/ |                     "));
  Serial.println(F("                                                       |___/                      "));
  Serial.println(F("Iniciando gravacao de dados..."));
}

void loop() {
  char tecla = teclado.getKey();

  if (tecla) {
    digitalWrite(buzzer, HIGH);
    delay(80);
    digitalWrite(buzzer, LOW);
  }

  if (tecla == '1') {
    modoAtual = ESTATISTICAS;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("ESTATISTICAS:"));
  }
  else if (tecla == '2') {
    modoAtual = MARCADOR;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Clique 2 p/ marcar"));
    lcd.setCursor(0, 3);
    lcd.print(F("4 - Voltar"));
  }
  else if (tecla == '3') {
    modoAtual = RELOGIO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("RELOGIO ATUAL"));
    lcd.setCursor(0, 3);
    lcd.print(F("4 - Voltar"));
  }
  else if (tecla == '4') {
    modoAtual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }

  unsigned long agora = millis();

  // --- MODO ESTATISTICAS ---
  if (modoAtual == ESTATISTICAS) {
    if (agora - lastSensorMillis >= sensorInterval) {
      lastSensorMillis = agora;

      float humi  = dht11.readHumidity();
      float tempC = dht11.readTemperature();
      int ldrValue = analogRead(ldrPin);
      int lumPercent = map(ldrValue, 0, 1023, 100, 0);
      lumPercent = constrain(lumPercent, 0, 100);

      lcd.setCursor(0,1);
      if (isnan(humi) || isnan(tempC)) {
        lcd.print(F("Erro no DHT      "));
      } else {
        lcd.print(F("T:"));
        lcd.print(tempC, 1);
        lcd.print(F("C U:"));
        lcd.print(humi, 0);
        lcd.print(F("% "));
      }

      lcd.setCursor(0,2);
      lcd.print(F("L:"));
      lcd.print(lumPercent);
      lcd.print(F("%         "));
      lcd.setCursor(0,3);
      lcd.print(F("4-Voltar"));

      if (!isnan(humi) && !isnan(tempC)) {
        flagTempAlta = (tempC >= 30);
        flagTempBaixa = (tempC <= 0);
        flagUmidAlta = (humi >= 90);
        flagUmidBaixa = (humi <= 20);
        flagLumAlta = (lumPercent >= 90);

        if (flagTempAlta || flagTempBaixa || flagUmidAlta || flagUmidBaixa || flagLumAlta) {
          Serial.print(F("[CRITICO] "));
          if (flagUmidBaixa) Serial.print(F("Umidade muito baixa! "));
          if (flagUmidAlta)  Serial.print(F("Umidade muito alta! "));
          if (flagLumAlta)   Serial.print(F("Luminosidade excessiva! "));
          if (flagTempAlta)  Serial.print(F("Temperatura alta! "));
          if (flagTempBaixa) Serial.print(F("Temperatura muito baixa! "));
          Serial.println();

          digitalWrite(ledR, HIGH);
          tone(buzzer, 4000, 300);
          delay(100);
          digitalWrite(ledR, LOW);
        }
        else if (humi < 60 || humi > 80 || lumPercent > 60 || tempC > 20 || tempC < 10) {
          Serial.print(F("Warning: "));
          if (humi < 60) Serial.print(F("Umidade um pouco baixa! "));
          if (humi > 80) Serial.print(F("Umidade um pouco alta! "));
          if (lumPercent > 60) Serial.print(F("Luminosidade acima do toleravel! "));
          if (tempC > 20) Serial.print(F("Temperatura acima do ideal! "));
          if (tempC < 10) Serial.print(F("Temperatura abaixo do ideal! "));
          Serial.println();

          digitalWrite(ledY, HIGH);
          delay(100);
          digitalWrite(ledY, LOW);
        } else {
          digitalWrite(ledG, HIGH);
          delay(100);
          digitalWrite(ledG, LOW);
        }
      }
    }
  }

  // --- MODO MARCADOR ---
  else if (modoAtual == MARCADOR && tecla == '2') {
    float humi  = dht11.readHumidity();
    float tempC = dht11.readTemperature();
    int ldrValue = analogRead(ldrPin);
    int lumPercent = map(ldrValue, 0, 1023, 100, 0);
    lumPercent = constrain(lumPercent, 0, 100);

    if (!isnan(humi) && !isnan(tempC)) {
      somaLdr += lumPercent;
      somaHumi += humi;
      somaTemp += tempC;
      nloops++;

      if (nloops >= 10) {
        mediaLdr = somaLdr / nloops;
        mediaHumi = somaHumi / nloops;
        mediaTemp = somaTemp / nloops;

        DateTime now = RTC.now();
        DateTime adjustedTime = DateTime(now.unixtime() + (UTC_OFFSET * 3600));

        Serial.println(F("=== MARCADOR SALVO ==="));
        Serial.print(F("Data: "));
        Serial.print(adjustedTime.day()); Serial.print('/');
        Serial.print(adjustedTime.month()); Serial.print('/');
        Serial.println(adjustedTime.year());
        Serial.print(F("Hora: "));
        Serial.print(adjustedTime.hour()); Serial.print(':');
        Serial.print(adjustedTime.minute()); Serial.print(':');
        Serial.println(adjustedTime.second());
        Serial.print(F("T: ")); Serial.print(mediaTemp); Serial.println(F("C"));
        Serial.print(F("U: ")); Serial.print(mediaHumi); Serial.println(F("%"));
        Serial.print(F("L: ")); Serial.print(mediaLdr); Serial.println(F("%"));
        Serial.println(F("======================="));

        lcd.setCursor(0, 2);
        lcd.print(F("Marcador salvo!  "));

        nloops = 0;
        somaLdr = 0;
        somaHumi = 0;
        somaTemp = 0;
      }
    } else {
      lcd.setCursor(0,2);
      lcd.print(F("Erro na leitura! "));
    }
  }

  // --- MODO RELOGIO ---
  else if (modoAtual == RELOGIO) {
    if (agora - lastSensorMillis >= 1000) {
      lastSensorMillis = agora;
      DateTime now = RTC.now();
      DateTime adjustedTime = DateTime(now.unixtime() + (UTC_OFFSET * 3600));

      lcd.setCursor(0,1);
      lcd.print(F("Data: "));
      if (adjustedTime.day() < 10) lcd.print('0');
      lcd.print(adjustedTime.day());
      lcd.print('/');
      if (adjustedTime.month() < 10) lcd.print('0');
      lcd.print(adjustedTime.month());
      lcd.print('/');
      lcd.print(adjustedTime.year());

      lcd.setCursor(0,2);
      lcd.print(F("Hora: "));
      if (adjustedTime.hour() < 10) lcd.print('0');
      lcd.print(adjustedTime.hour());
      lcd.print(':');
      if (adjustedTime.minute() < 10) lcd.print('0');
      lcd.print(adjustedTime.minute());
      lcd.print(':');
      if (adjustedTime.second() < 10) lcd.print('0');
      lcd.print(adjustedTime.second());
      lcd.print(F("   "));
    }
  }
}