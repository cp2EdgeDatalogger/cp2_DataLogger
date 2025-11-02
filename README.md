# Sistema de Monitoramento Ambiental com EEPROM e RTC – Edge Computing

## Contexto Acadêmico
Este projeto foi desenvolvido no contexto da disciplina de **Edge Computing**, com o objetivo de criar um sistema embarcado capaz de **coletar, processar e registrar dados ambientais** diretamente no microcontrolador.  
A proposta reflete os princípios da computação de borda — **analisar dados localmente, reduzir dependência de nuvem** e otimizar o tempo de resposta em aplicações IoT.

---

## Contexto de Aplicação – Vinheria Agnello
Inspirado no ambiente da **Vinheria Agnello**, o projeto simula um sistema de monitoramento inteligente que observa **temperatura, umidade e luminosidade** do espaço de armazenamento dos vinhos.  
O controle preciso dessas variáveis é essencial para preservar aroma, sabor e qualidade, mantendo a tradição da vinheria agora apoiada pela tecnologia da **APAP Systems**.

---

## 🔎 Sobre o Projeto
O sistema foi desenvolvido em **C++ para Arduino UNO**, e reúne múltiplos sensores e periféricos integrados:

- **DHT22** — sensor de temperatura e umidade.  
- **LDR (fotoresistor)** — capta a intensidade luminosa.  
- **EEPROM interna** — armazena logs críticos de operação com data e hora.  
- **RTC DS1307** — mantém registro temporal mesmo sem alimentação.  
- **Keypad 4x4** — permite navegação entre menus e funções.  
- **Display LCD 20x4 (I²C)** — exibe informações em tempo real e menus interativos.  
- **LEDs** — indicam status ambiental (seguro, atenção, crítico).  
- **Buzzer** — emite alertas sonoros proporcionais ao nível de risco.  

O software identifica condições de alerta e, quando necessário, **registra automaticamente o evento na EEPROM**, com timestamp do RTC — funcionando como um **data logger autônomo em Edge Computing**.

---

## Funcionamento Geral

1. O sistema lê periodicamente:
   - Temperatura (°C)
   - Umidade (%)
   - Luminosidade (%)

2. As leituras são processadas e classificadas:
   - 🟢 Faixa ideal  
   - 🟡 Alerta  
   - 🔴 Crítico  

3. Quando um parâmetro ultrapassa o limite crítico:
   - Os **LEDs** e o **buzzer** são acionados.  
   - O evento é **registrado na EEPROM** com:
     - Data e hora (RTC)
     - Temperatura, Umidade, Luminosidade

4. É possível navegar pelos modos via teclado:
   - `A` → Estatísticas (valores médios em tempo real)  
   - `B` → Marcadores e Logs (visualiza registros salvos)  
   - `C` → Relógio (exibe data e hora atual)  
   - `#` → Limpar EEPROM  
   - `*` → Voltar ao menu principal  

---

## Estrutura de Registro na EEPROM
Cada evento ocupa **8 bytes**
Os campos são gravados na seguinte sequência:

| Campo         | Tipo           | Bytes | Descrição |
|----------------|----------------|--------|------------|
| `epoch`        | unsigned long  | 4      | Timestamp UNIX |
| `tempC`        | int            | 2      | Temperatura em °C |
| `humi`         | byte           | 1      | Umidade (%) |
| `lumPercent`   | byte           | 1      | Luminosidade (%) |

A EEPROM armazena até **100 registros**, que podem ser visualizados diretamente no display LCD ou via **Serial Monitor**.

---

## Componentes Utilizados

| Quantidade | Componente | Função |
|-------------|-------------|--------|
| 1 | Arduino UNO | Unidade de controle principal |
| 1 | Sensor DHT22 | Medição de temperatura e umidade |
| 1 | LDR + resistor de 10kΩ | Sensor de luminosidade |
| 3 | LEDs (verde, amarelo, vermelho) | Indicadores de status |
| 3 | Resistores de 220Ω | Limitação de corrente dos LEDs |
| 1 | Buzzer piezoelétrico | Alerta sonoro |
| 1 | Display LCD 20x4 (I²C) | Exibição de dados e menus |
| 1 | RTC DS1307 | Relógio em tempo real |
| 1 | Keypad 4x4 | Interface de interação |
| 1 | Protoboard + jumpers | Montagem do circuito |

---

## Como Reproduzir o Projeto

### 1. Clonar o Repositório
```bash
git clone https://github.com/FelipeMenezes937/arduino-env-monitor.git
```

### 1. Simular no Wokwi

[Projeto no Wokwi](https://wokwi.com/projects/445423567875087361)

---
## Integrantes do Projeto

| Nome Completo                  | Função         |
|--------------------------------|----------------|
| Felipe Silva Santos Menezes    | Desenvolvedor  |
| Gabriel Ardito Manes           | Desenvolvedor  |
| João Pedro Gonzales            | Desenvolvedor  |
| João Antonio Sarracine         | Desenvolvedor  |