#include <Wire.h>
#include <RTClib.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <EEPROM.h>

// CONFIGS DA EEPROM
const int maxRecords = 100;
const int recordSize = 8;
int startAddress = 0;
int endAddress = maxRecords * recordSize;
int currentAddress = 0;
int oldAddress = 0;

#define LOG_OPTION 1
#define SERIAL_OPTION 1
#define UTC_OFFSET -3

#define DHT22_PIN 2
#define DHTTYPE DHT22

const int ledG = 3;
const int ledY = 4;
const int ledR = 5;
const int buzzer = A1;
const int ldrPin = A0;

// variáveis globais de média
int nloops = 0;
int somaLdr = 0;
int somaTemp = 0;
int somaHumi = 0;
int mediaLdr = 0;
int mediaTemp = 0;
int mediaHumi = 0;

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
DHT dht22(DHT22_PIN, DHTTYPE);

// Teclado 4x4
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
const unsigned long sensorInterval = 1000;

enum Modo { MENU_PRINCIPAL, ESTATISTICAS, MARCADOR, RELOGIO, MARCADORES };
Modo modoAtual = MENU_PRINCIPAL;

// ======================= FUNÇÕES =========================

void getNextAddress() {
  currentAddress += recordSize;
  if (currentAddress >= endAddress) {
    currentAddress = 0;
    oldAddress += recordSize;
  }
}
void limparEEPROM() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(F("Limpando EEPROM..."));

  for (int i = 0; i < endAddress; i++) {
    EEPROM.update(i, 0xFF); 
  }

  currentAddress = 0;

  lcd.setCursor(0, 2);
  lcd.print(F("Concluido!"));
  delay(1000);
}
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

void get_log() {
  for (int address = startAddress; address < endAddress; address += recordSize) {
    unsigned long timeStamp;
    int tempC;
    byte humi, lumPercent;

    // LER DADOS DA EEPROM
    EEPROM.get(address, timeStamp);
    EEPROM.get(address + 4, tempC);
    EEPROM.get(address + 6, humi);
    EEPROM.get(address + 7, lumPercent);

    if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
      DateTime dt = DateTime(timeStamp);
      Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
      Serial.print("\t");
      Serial.print(tempC);
      Serial.print("C\t");
      Serial.print(humi);
      Serial.print("%\t");
      Serial.print(lumPercent);
      Serial.println("%");
      
    }
  }
}

// ======================= SETUP =========================

void setup() {
  if (SERIAL_OPTION) Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  dht22.begin();

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

  Serial.println(F("Iniciando gravacao de dados..."));
}

// ======================= LOOP =========================

void loop() {
  DateTime now = RTC.now();
  unsigned long epoch = now.unixtime() + (UTC_OFFSET * 3600);
  DateTime adjustedTime = DateTime(epoch);

  // --- Leitura dos sensores ---
  float humi = dht22.readHumidity();
  float tempC = dht22.readTemperature();
  int ldrValue = analogRead(ldrPin);

 
int lumPercent = map(ldrValue, 0, 1023, 100, 0);
lumPercent = constrain(lumPercent, 0, 100);

somaLdr += (int)lumPercent;         // já em 0–100
somaTemp += (int)(tempC * 10);   // 1 casa decimal
somaHumi += (int)(humi * 10);   // 2 casas decimais
nloops++;

if (nloops >= 10) {
  mediaTemp = (somaTemp / nloops) * 10;   // escala decimal
  mediaHumi = (somaHumi / nloops) * 10;       
  mediaLdr  = somaLdr / nloops;           // % direta
         // sem multiplicar, já é %
 
  nloops = 0;
  somaTemp = 0;
  somaHumi = 0;
  somaLdr  = 0;
}
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
    lcd.print(F("Clique: A p/ marcar ou # p/ apagar"));
    lcd.setCursor(0,1);
    lcd.print("4- Ver marcadores");
    lcd.setCursor(0, 3);
    lcd.print(F("5 - Voltar"));
  }
  else if (tecla == '3') {
    modoAtual = RELOGIO;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("RELOGIO ATUAL"));
    lcd.setCursor(0, 3);
    lcd.print(F("5 - Voltar"));
  }
  else if (modoAtual == MARCADOR && tecla=='4') {
    lcd.clear();
    modoAtual = MARCADORES;
    lcd.setCursor(0,0);
    lcd.print("MARCADORES:");
    if (LOG_OPTION) get_log();
  }
  else if (modoAtual == MARCADOR && tecla == '#') {
  limparEEPROM();           // chama a função
  mostrarMenuPrincipal();   // volta ao menu principal
  modoAtual = MENU_PRINCIPAL;
}
  else if (tecla == '5') {
    modoAtual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }

  unsigned long agora = millis();

  // --- MODO ESTATISTICAS ---
  if (modoAtual == ESTATISTICAS) {
    if (agora - lastSensorMillis >= sensorInterval) {
      lastSensorMillis = agora;

      lcd.setCursor(0,1);
      lcd.print(F("T:"));
      lcd.print((mediaTemp / 100.0), 1);
      lcd.print(F("C"));
      lcd.setCursor(0,3);
      lcd.print(F("U:"));
      lcd.print((mediaHumi / 100.0), 1);
      lcd.print(F("% ")); 
      lcd.setCursor(0,2);
      lcd.print(F("L:"));
      lcd.print((int)mediaLdr, DEC);
      lcd.print(F("%         "));
      lcd.setCursor(12,3);
      lcd.print(F("5-Voltar"));

      // --- ALERTAS ---
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

        EEPROM.put(currentAddress, epoch);
        EEPROM.put(currentAddress + 4, (int)tempC);
        EEPROM.put(currentAddress + 6, (byte)humi);
        EEPROM.put(currentAddress + 7, lumPercent);
        getNextAddress();

        digitalWrite(ledR, HIGH);
        tone(buzzer, 4000, 300);
        delay(100);
        digitalWrite(ledR, LOW);
      }
      else if (humi < 60 || humi > 80 || lumPercent > 60 || tempC > 20 || tempC < 10) {
        Serial.print(F("Warning: "));
        if (humi < 60) Serial.print(F("Umidade um pouco baixa! "));
        if (humi > 80) Serial.print(F("Umidade um pouco alta! "));
        if (lumPercent > 60) Serial.print(F("Luz acima do ideal! "));
        if (tempC > 20) Serial.print(F("Temp. acima do ideal! "));
        if (tempC < 10) Serial.print(F("Temp. abaixo do ideal! "));
        Serial.println();

        digitalWrite(ledY, HIGH);
        tone(buzzer, 2000, 200);
        delay(100);
        digitalWrite(ledY, LOW);
      } else {
        digitalWrite(ledG, HIGH);
        delay(100);
        digitalWrite(ledG, LOW);
      }
    }
  }

  // --- MODO MARCADOR ---
  else if (modoAtual == MARCADOR && tecla == 'A') {
    Serial.println(F("=== MARCADOR SALVO ==="));
    Serial.print(F("Data: "));
    Serial.println(adjustedTime.timestamp(DateTime::TIMESTAMP_DATE));
    Serial.print(F("Hora: "));
    Serial.println(adjustedTime.timestamp(DateTime::TIMESTAMP_TIME));
    Serial.print(F("T: ")); Serial.print(tempC); Serial.println(F("C"));
    Serial.print(F("U: ")); Serial.print(humi); Serial.println(F("%"));
    Serial.print(F("L: ")); Serial.print(lumPercent); Serial.println(F("%"));
    Serial.println(F("======================="));

    lcd.setCursor(0, 2);
    lcd.print(F("Marcador salvo!  "));

    
  }

// --- MODO MARCADORES ---
  else if (modoAtual == MARCADORES) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("MARCADORES SALVOS"));
    delay(800);
    int marcador = 1;
    for (int address = startAddress; address < endAddress; address += recordSize) {
      unsigned long timeStamp;
      int tempC;
      byte humi, lumPercent;

      EEPROM.get(address, timeStamp);
      EEPROM.get(address + 4, tempC);
      EEPROM.get(address + 6, humi);
      EEPROM.get(address + 7, lumPercent);

      // Ignora espaços vazios
      if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
        DateTime dt = DateTime(timeStamp);

        // Mostra no LCD
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(dt.day());
        lcd.print('/');
        lcd.print(dt.month());
        lcd.print(' ');
        if (dt.hour() < 10) lcd.print('0');
        lcd.print(dt.hour());
        lcd.print(':');
        if (dt.minute() < 10) lcd.print('0');
        lcd.print(dt.minute());

        lcd.setCursor(0,1);
        lcd.print("Marcador -");
        lcd.setCursor(11,1);
        lcd.print(marcador);

        lcd.setCursor(0, 2);
        lcd.print(F("T:"));
        lcd.print(tempC);
        lcd.print(F("C  U:"));
        lcd.print(humi);
        lcd.print(F("%"));
        lcd.print(F(" L:"));
        lcd.print(lumPercent);
        lcd.print(F("%"));

        lcd.setCursor(0, 3);
        lcd.print(F("5 - Voltar"));
        marcador +=1;
        delay(2000); // Mostra cada registro por 2 segundos
      }
    }

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(F("Fim dos registros"));
    delay(1000);
    mostrarMenuPrincipal();
    modoAtual = MENU_PRINCIPAL;
  }

  // --- MODO RELOGIO ---
  else if (modoAtual == RELOGIO) {
    if (agora - lastSensorMillis >= 1000) {
      lastSensorMillis = agora;

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
    }
  }
}

