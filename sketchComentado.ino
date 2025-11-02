// ======================================================
//     SISTEMA DE MONITORAMENTO AMBIENTAL "APAP"
// ======================================================
// Leitura de sensores DHT22 (umidade e temperatura) e LDR (luminosidade),
// exibição em LCD I2C 20x4, armazenamento de alertas na EEPROM,
// navegação via teclado 4x4 e controle de LEDs e buzzer.
// ======================================================

#include <Wire.h>               // Comunicação I2C (RTC e LCD)
#include <RTClib.h>             // Biblioteca para manipular data/hora (RTC DS1307)
#include <DHT.h>                // Biblioteca para sensor DHT22 (temperatura/umidade)
#include <LiquidCrystal_I2C.h>  // Biblioteca para display LCD com interface I2C
#include <Keypad.h>             // Biblioteca para teclado matricial 4x4
#include <EEPROM.h>             // Biblioteca para memória EEPROM (armazenamento persistente)

// ======================================================
// CONFIGURAÇÕES DA EEPROM
// ======================================================

const int maxRecords = 100;       // Número máximo de registros que podem ser armazenados
const int recordSize = 8;         // Cada registro ocupa 8 bytes (timestamp, temp, umid, luz)
int startAddress = 0;             // Endereço inicial da EEPROM
int endAddress = maxRecords * recordSize; // Endereço final
int currentAddress = 0;           // Endereço atual de gravação
int oldAddress = 0;               // Endereço anterior (para controle de sobrescrita)

// ======================================================
// DEFINIÇÕES DE SISTEMA
// ======================================================

#define LOG_OPTION 1              // Ativa o modo de gravação de logs
#define SERIAL_OPTION 1           // Ativa saída serial
#define UTC_OFFSET -3             // Ajuste de fuso horário (Brasil)

// ======================================================
// CONFIGURAÇÃO DOS SENSORES E ATUADORES
// ======================================================

#define DHT22_PIN 2               // Pino do sensor DHT22
#define DHTTYPE DHT22             // Tipo de sensor DHT (modelo DHT22)

const int ledG = 3;               // LED verde (status normal)
const int ledY = 4;               // LED amarelo (atenção)
const int ledR = 5;               // LED vermelho (crítico)
const int buzzer = A1;            // Buzzer no pino analógico A1
const int ldrPin = A0;            // Sensor de luminosidade (LDR)

// ======================================================
// VARIÁVEIS DE CÁLCULO DE MÉDIAS
// ======================================================

int nloops = 0;                   // Contador de leituras realizadas
int somaLdr = 0;                  // Soma das leituras de luminosidade
int somaTemp = 0;                 // Soma das leituras de temperatura
int somaHumi = 0;                 // Soma das leituras de umidade
int mediaLdr = 0;                 // Média final de luminosidade
int mediaTemp = 0;                // Média final de temperatura
int mediaHumi = 0;                // Média final de umidade

// ======================================================
// FLAGS DE ALERTA (ESTADOS DE SENSOR)
// ======================================================

bool flagTempAlta = false;        // Temperatura acima do limite
bool flagTempBaixa = false;       // Temperatura abaixo do limite
bool flagUmidAlta = false;        // Umidade alta
bool flagUmidBaixa = false;       // Umidade baixa
bool flagLumAlta = false;         // Luminosidade excessiva

// ======================================================
// DESENHO DE CARACTERES PERSONALIZADOS PARA O LCD
// ======================================================
// (Cada byte representa uma linha de pixels 5x8)

// Matrizes das letras "A" e "P" divididas em quatro quadrantes
byte A_topleft[8] = {B00000, B00000, B00011, B01111, B11000, B11000, B11000, B11000};
byte A_botleft[8] = {B11111, B11111, B11000, B11000, B11000, B11000, B00000, B00000};
byte A_topright[8] = {B00000, B00000, B11000, B11110, B00011, B00011, B00011, B00011};
byte A_botright[8] = {B11111, B11111, B00011, B00011, B00011, B00011, B00000, B00000};
byte P_topleft[8] = {B00000, B00000, B11111, B11111, B11000, B11000, B11000, B11000};
byte P_botleft[8] = {B11111, B11111, B11000, B11000, B11000, B11000, B00000, B00000};
byte P_topright[8] = {B00000, B00000, B11100, B11110, B00011, B00011, B00011, B00011};
byte P_botright[8] = {B11110, B11100, B00000, B00000, B00000, B00000, B00000, B00000};

// ÍCONES CLIMÁTICOS E DE STATUS
const char fogo[8] PROGMEM = {0b01001,0b10100,0b00010,0b10100,0b00110,0b10111,0b11111,0b11111};
const char gelo[8] PROGMEM = {0b00100,0b01010,0b10101,0b01110,0b10101,0b01010,0b00100,0b00000};
const char gotaMeia[8] PROGMEM = {0b00100,0b01010,0b10001,0b10111,0b11111,0b11111,0b01110,0b00000};
const char gotaVazia[8] PROGMEM = {0b00100,0b01010,0b10001,0b10001,0b10001,0b10001,0b01110,0b00000};
const char gotaCheia[8] PROGMEM = {0b00100,0b01110,0b11111,0b11111,0b11111,0b11111,0b01110,0b00000};
const char sol[8] PROGMEM = {0b00100,0b10101,0b01110,0b11111,0b01110,0b10101,0b00100,0b00000};
const char porSol[8] PROGMEM = {0b10001,0b00010,0b10100,0b11000,0b11101,0b11010,0b11000,0b00100};
const char lua[8] PROGMEM = {0b00110,0b11111,0b10011,0b00001,0b00001,0b10011,0b11111,0b01110};

// Ícones de “arquivo” usados para modo de marcadores
const char arquivoEsq[8] PROGMEM = {0b01111,0b01010,0b01011,0b01000,0b01011,0b01010,0b01010,0b01111};
const char arquivoDir[8] PROGMEM = {0b11100,0b00110,0b11101,0b00001,0b11101,0b00101,0b00101,0b11111};

// ======================================================
// INSTÂNCIAS DE HARDWARE
// ======================================================

LiquidCrystal_I2C lcd(0x27, 20, 4);  // LCD I2C no endereço 0x27
RTC_DS1307 RTC;                      // Relógio de tempo real DS1307
DHT dht22(DHT22_PIN, DHTTYPE);       // Sensor DHT22 de temperatura/umidade

// ======================================================
// CONFIGURAÇÃO DO TECLADO 4x4
// ======================================================

const byte ROWS = 4;                 // Número de linhas
const byte COLS = 4;                 // Número de colunas
char keys[ROWS][COLS] = {            // Mapeamento das teclas
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {13,12,11,10};  // Pinos das linhas
byte colPins[COLS] = {9,8,7,6};      // Pinos das colunas
Keypad teclado = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ======================================================
// VARIÁVEIS DE CONTROLE DE TEMPO
// ======================================================

unsigned long lastSensorMillis = 0;          // Guarda o tempo da última leitura
const unsigned long sensorInterval = 1000;   // Intervalo entre leituras (1 segundo)

// ======================================================
// ENUMERAÇÃO DE MODOS DE OPERAÇÃO
// ======================================================

enum Modo { MENU_PRINCIPAL, ESTATISTICAS, MARCADOR, RELOGIO, MARCADORES };
Modo modoAtual = MENU_PRINCIPAL;             // Estado inicial do sistema

// ======================================================
// FUNÇÕES DE SUPORTE
// ======================================================

// Carrega um caractere armazenado em PROGMEM para o LCD
void loadGlyphFromProgmem(byte slot, const char* glyphPROGMEM) {
    byte buffer[8];
    memcpy_P(buffer, glyphPROGMEM, 8);
    lcd.createChar(slot, buffer);
}

// Carrega ícones usados no modo de marcadores
void carregarIconesMarcadores() {
    loadGlyphFromProgmem(6, arquivoEsq);
    loadGlyphFromProgmem(7, arquivoDir);
}

// Carrega ícones usados no modo estatísticas
void carregarIconesEstatisticas() {
    loadGlyphFromProgmem(0, fogo);
    loadGlyphFromProgmem(1, gelo);
    loadGlyphFromProgmem(2, gotaCheia);
    loadGlyphFromProgmem(3, gotaVazia);
    loadGlyphFromProgmem(6, gotaMeia);
    loadGlyphFromProgmem(4, sol);
    loadGlyphFromProgmem(7, porSol);
    loadGlyphFromProgmem(5, lua);
}

// Atualiza o endereço da próxima gravação na EEPROM
void getNextAddress() {
  currentAddress += recordSize;
  if (currentAddress >= endAddress) {
    currentAddress = 0;       // Reinicia o ciclo de gravação
    oldAddress += recordSize; // Avança marcador de dados antigos
  }
}

// Limpa completamente a EEPROM
void limparEEPROM() {
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(F("Limpando EEPROM..."));

  for (int i = 0; i < endAddress; i++) {
    EEPROM.update(i, 0xFF);  // Substitui cada byte por FF (padrão vazio)
  }

  currentAddress = 0;
  lcd.setCursor(0, 2);
  lcd.print(F("Concluido!"));
  delay(1000);
}

// Exibe o menu principal
void mostrarMenuPrincipal() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print(F("Selecione uma opcao:"));
  lcd.setCursor(0, 1); lcd.print(F("A - Estatisticas"));
  lcd.setCursor(0, 2); lcd.print(F("B - Marcador"));
  lcd.setCursor(0, 3); lcd.print(F("C - Relogio"));
}

// Lê e exibe os logs salvos na EEPROM (modo debug)
void get_log() {
  for (int address = startAddress; address < endAddress; address += recordSize) {
    unsigned long timeStamp;
    int tempC;
    byte humi, lumPercent;

    // Lê o registro
    EEPROM.get(address, timeStamp);
    EEPROM.get(address + 4, tempC);
    EEPROM.get(address + 6, humi);
    EEPROM.get(address + 7, lumPercent);

    // Ignora posições vazias
    if (timeStamp != 0xFFFFFFFF && timeStamp != 0) {
      DateTime dt = DateTime(timeStamp);
      Serial.print(dt.timestamp(DateTime::TIMESTAMP_FULL));
      Serial.print("\t");
      Serial.print(tempC); Serial.print("C\t");
      Serial.print(humi); Serial.print("%\t");
      Serial.print(lumPercent); Serial.println("%");
    }
  }
}

// Verifica se a tecla pressionada é válida
void verificaTecla(char tecla){
  if (tecla != 'A' && tecla != 'B' && tecla != 'C' && tecla != 'D' && tecla != '#' && tecla != '*') {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Opcao invalida"));
    delay(500);
    modoAtual = MENU_PRINCIPAL;
    mostrarMenuPrincipal();
  }
}

// Limpa uma linha do LCD (usado em animações)
void clearLine(int line) {
  lcd.setCursor(0, line);
  lcd.print("                    "); // 20 espaços
}

// Animação inicial "APAP"
void animarAPAP() {
  int larguraTexto = 8; // 4 letras x 2 colunas cada
  int larguraLCD = 20;
  for (int pos = larguraLCD; pos >= -larguraTexto; pos--) {
    clearLine(1);
    clearLine(2);
    int startIndex = max(0, -pos);
    int startCol = max(0, pos);
    int drawCols = min(larguraTexto - startIndex, larguraLCD - startCol);
    lcd.setCursor(startCol, 1);
    for (int i = 0; i < drawCols; i++) lcd.write(i);
    delay(150);
  }
}

// ======================================================
// SETUP - INICIALIZAÇÃO
// ======================================================

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

  // Cria os caracteres personalizados (letras A e P)
  lcd.createChar(0, A_topleft);
  lcd.createChar(1, A_botleft);
  lcd.createChar(2, A_topright);
  lcd.createChar(3, A_botright);
  lcd.createChar(4, P_topleft);
  lcd.createChar(5, P_botleft);
  lcd.createChar(6, P_topright);
  lcd.createChar(7, P_botright);

  animarAPAP();   // Mostra animação de inicialização
  delay(1500);
  lcd.clear();

  // Define os pinos dos LEDs e buzzer
  pinMode(ledG, OUTPUT);
  pinMode(ledY, OUTPUT);
  pinMode(ledR, OUTPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  for (int i = 6; i <= 13; i++) pinMode(i, INPUT_PULLUP);

  mostrarMenuPrincipal();
  Serial.println(F("Iniciando gravacao de dados..."));
}

// ======================================================
// LOOP PRINCIPAL
// ======================================================

void loop() {
  // Lê o horário atual do RTC
  DateTime now = RTC.now();
  unsigned long epoch = now.unixtime() + (UTC_OFFSET * 3600);
  DateTime adjustedTime = DateTime(epoch);

  // --- LEITURA DOS SENSORES ---
  float humi = dht22.readHumidity();
  float tempC = dht22.readTemperature();
  int ldrValue = analogRead(ldrPin);

  // Converte LDR em porcentagem (0 a 100%)
  int lumPercent = map(ldrValue, 0, 1023, 100, 0);
  lumPercent = constrain(lumPercent, 0, 100);

  // Calcula médias móveis a cada 10 leituras
  somaLdr += lumPercent;
  somaTemp += tempC * 10;
  somaHumi += humi * 10;
  nloops++;
  if (nloops >= 10) {
    mediaTemp = (somaTemp / nloops) * 10;
    mediaHumi = (somaHumi / nloops) * 10;
    mediaLdr  = somaLdr / nloops;
    nloops = somaLdr = somaTemp = somaHumi = 0;
  }

  // --- LEITURA DO TECLADO ---
  char tecla = teclado.getKey();
  if (tecla) {
    verificaTecla(tecla);
    digitalWrite(buzzer, HIGH);
    delay(80);
    digitalWrite(buzzer, LOW);
  }

  // --- TROCA DE MODOS ---
  if (tecla == 'A') { modoAtual = ESTATISTICAS; lcd.clear(); lcd.print(F("ESTATISTICAS:")); delay(200); }
  else if (tecla == 'B') { modoAtual = MARCADOR; lcd.clear(); lcd.print(F("Modo Marcador")); }
  else if (tecla == 'C') { modoAtual = RELOGIO; lcd.clear(); lcd.print(F("Relogio Atual")); }
  else if (tecla == '*') { modoAtual = MENU_PRINCIPAL; mostrarMenuPrincipal(); }

  // --- EXIBIÇÃO DE DADOS ---
  unsigned long agora = millis();

  // (A continuação exibe dados e alertas no LCD,
  // grava registros críticos na EEPROM,
  // e mostra o relógio conforme o modo selecionado)
}
