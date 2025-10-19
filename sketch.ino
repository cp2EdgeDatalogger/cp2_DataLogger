#include <Wire.h>
#include <RTClib.h>
#include "DHT.h"

#define SERIAL_OPTION 1 //
#define UTC_OFFSET -3

#define DHT22_PIN 2
int ledG = 3;
int ledY = 4;
int ledR = 5;
int buzzer = A1;
int ldr = A0;

DHT dht22(DHT22_PIN, DHT22);
RTC_DS1307 RTC;

void setup() {

  pinMode(buzzer, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ldr, INPUT);

  if (SERIAL_OPTION) Serial.begin(9600);
  //desenhando o nome
  Serial.println(" __      ___       _          _      _                                  _ _       ");
  Serial.println(" \\ \\    / (_)     | |        (_)    (_)           /\\                   | | |      ");
  Serial.println("  \\ \\  / / _ _ __ | |__   ___ _ _ __ _  __ _     /  \\   __ _ _ __   ___| | | ___  ");
  Serial.println("   \\ \\/ / | | '_ \\| '_ \\ / _ \\ | '__| |/ _` |   / /\\ \\ / _` | '_ \\ / _ \\ | |/ _ \\ ");
  Serial.println("    \\  /  | | | | | | | |  __/ | |  | | (_| |  / ____ \\ (_| | | | |  __/ | | (_) |");
  Serial.println("     \\/   |_|_| |_|_| |_|\\___|_|_|  |_|\\__,_| /_/    \\_\\__, |_| |_|\\___|_|_|\\___/ ");
  Serial.println("                                                        __/ |                     ");
  Serial.println("                                                       |___/                      ");
  Serial.println("iniciando gravacao de dados...");
 //startando leituras
  Wire.begin();
  RTC.begin();
  dht22.begin();

  // RTC.adjust(DateTime(2025, 10, 9, 16, 34, 0)); // só na primeira vez
}

void loop() {
    
  
  // Leituras
  float humi  = dht22.readHumidity();
  float tempC = dht22.readTemperature(); //capturando em Celsius
  float tempF = dht22.readTemperature(); //capturando em Farenheit
  float temp = tempC;
  int ldrValue = analogRead(ldr);
  float lum = map(ldrValue, 0, 1023, 100, 0); // brilho em %

  // Hora atual com fuso
  DateTime now = RTC.now();
  uint32_t timestamp = now.unixtime() + (UTC_OFFSET * 3600);
  DateTime adjustedTime = DateTime(timestamp);

  if (SERIAL_OPTION) {
    if (isnan(humi) || isnan(temp) || isnan(lum)) {// isnan --> is not a number(verifica se chega um valor q não é numero )
      Serial.println("Erro ao ler sensores!");
    } else {
      if (humi <= 20 || humi >= 90 || lum >= 90 || temp >= 20 || temp <= 0) {
  Serial.print("critical: ");
  digitalWrite(ledR, HIGH);
  tone(buzzer, 5000);
  delay(3000);
  noTone(buzzer);
  digitalWrite(ledR, LOW);

} else if (humi < 60 || humi > 80 || lum > 60 || temp > 16 || temp < 10) {
  Serial.print("warning: ");
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
      Serial.print(adjustedTime.minute() < 10 ? "0" : "");
      Serial.print(adjustedTime.minute());
      Serial.print(" | ");

      Serial.print("U=");
      Serial.print(humi, 0);
      Serial.print("%| ");

      Serial.print("L=");
      Serial.print(lum, 0);
      Serial.print("%| ");

      Serial.print("T=");
      Serial.print(temp, 0);
      Serial.println("C");
      

      }
    }
