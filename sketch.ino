#include <Wire.h>
#include <RTClib.h>
#include "DHT.h"

#define SERIAL_OPTION 1
#define UTC_OFFSET -3

#define DHT22_PIN 2
int ledG = 3;
int ledY = 4;
int ledR = 5;
int buzzer = A1;
int ldr = A0;

int nloops = 0;
float somaLdr = 0;
float somaTemp = 0;
float somaHumi = 0;
float mediaLdr = 0;
float mediaTemp = 0;
float mediaHumi = 0;

DHT dht22(DHT22_PIN, DHT22);
RTC_DS1307 RTC;

void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ldr, INPUT);

  if (SERIAL_OPTION) Serial.begin(9600);

  // Desenho do nome
Serial.println(F(" __      ___       _          _      _                                  _ _       "));
Serial.println(F(" \\ \\    / (_)     | |        (_)    (_)           /\\                   | | |      "));
Serial.println(F("  \\ \\  / / _ _ __ | |__   ___ _ _ __ _  __ _     /  \\   __ _ _ __   ___| | | ___  "));
Serial.println(F("   \\ \\/ / | | '_ \\| '_ \\ / _ \\ | '__| |/ _` |   / /\\ \\ / _` | '_ \\ / _ \\ | |/ _ \\ "));
Serial.println(F("    \\  /  | | | | | | | |  __/ | |  | | (_| |  / ____ \\ (_| | | | |  __/ | | (_) |"));
Serial.println(F("     \\/   |_|_| |_|_| |_|\\___|_|_|  |_|\\__,_| /_/    \\_\\__, |_| |_|\\___|_|_|\\___/ "));
Serial.println(F("                                                        __/ |                     "));
Serial.println(F("                                                       |___/                      "));
Serial.println(F("Iniciando gravacao de dados..."));


  Wire.begin();
  RTC.begin();
  dht22.begin();

  // RTC.adjust(DateTime(2025, 10, 9, 16, 34, 0)); // só na primeira vez
}

void loop() {
  // Leituras
  int ldrValue = analogRead(ldr);
  float humi = dht22.readHumidity();
  float tempC = dht22.readTemperature();
  float temp = tempC;

  // Incremento para médias
  somaLdr += ldrValue;
  somaHumi += humi;
  somaTemp += temp;
  nloops++;

  if (nloops == 10) {
    // Cálculo das médias
    mediaLdr = somaLdr / nloops;
    mediaHumi = somaHumi / nloops;
    mediaTemp = somaTemp / nloops;

    float lum = map(mediaLdr, 0, 1023, 100, 0); // brilho em %

    // Hora atual com fuso
    DateTime now = RTC.now();
    uint32_t timestamp = now.unixtime() + (UTC_OFFSET * 3600);
    DateTime adjustedTime = DateTime(timestamp);

    if (SERIAL_OPTION) {
      if (isnan(mediaHumi) || isnan(mediaTemp)) {
        Serial.println("Erro ao ler sensores!");
      } else {
        if (mediaHumi <= 20 || mediaHumi >= 90 || lum >= 90 || mediaTemp >= 20 || mediaTemp <= 0) {
  Serial.print("[CRITICAL]: ");
  
  if (mediaHumi <= 20) Serial.print(F("Umidade muito baixa! "));
if (mediaHumi >= 90) Serial.print(F("Umidade muito alta! "));
if (lum >= 90) Serial.print(F("Luminosidade excessiva! "));
if (mediaTemp >= 20) Serial.print(F("Temperatura alta! "));
if (mediaTemp <= 0) Serial.print(F("Temperatura muito baixa! "));
Serial.println();// colocando junto pq tem chance do compilador deixar no mesmo buffer, ai dá cagada
  
  digitalWrite(ledR, HIGH);
  tone(buzzer, 5000);
  delay(3000);
  noTone(buzzer);
  digitalWrite(ledR, LOW);

} else if (mediaHumi < 60 || mediaHumi > 80 || lum > 60 || mediaTemp > 16 || mediaTemp < 10) {
  Serial.print("warning: ");

  if (mediaHumi < 60) Serial.print(F("Umidade um pouco baixa! "));
  if (mediaHumi > 80) Serial.print(F("Umidade um pouco alta! "));
  if (lum > 60) Serial.print(F("luminosidade acima do toleravel! "));
  if (mediaTemp > 16) Serial.print(F("Temperatura acima do ideal! "));
  if (mediaTemp < 10) Serial.print(F("Temperatura abaixo do ideal! "));

  Serial.println();
  
  digitalWrite(ledY, HIGH);
  tone(buzzer, 1000);
  delay(2000);
  noTone(buzzer);
  digitalWrite(ledY, LOW);

} else {
  digitalWrite(ledG, HIGH);
  delay(3000);
  digitalWrite(ledG, LOW);
}

      }

      Serial.print(adjustedTime.day());
      Serial.print("/");
      Serial.print(adjustedTime.month());
      Serial.print("/");
      Serial.print(adjustedTime.year());
      Serial.print(" | ");
      Serial.print(adjustedTime.hour());
      Serial.print(":");
      if (adjustedTime.minute() < 10) Serial.print("0");
      Serial.print(adjustedTime.minute());
      Serial.print(" | ");

      Serial.print("U=");
      Serial.print(mediaHumi, 0);
      Serial.print("% | ");

      Serial.print("L=");
      Serial.print(lum, 0);
      Serial.print("% | ");

      Serial.print("T=");
      Serial.print(mediaTemp, 0);
      Serial.println("C");
    }

    // Resetando vars para próxima leitura
    nloops = 0;
    somaLdr = 0;
    somaHumi = 0;
    somaTemp = 0;
  }

  delay(250);
}
