# 🔥 Estufa de Filamentos - Filament Dryer Controller

Sistema inteligente para secar filamentos de impressão 3D com **controle PID de temperatura**, monitoramento de umidade e múltiplas camadas de segurança.

## 📋 Índice

- [Características](#-características-do-projeto)
- [Arquitetura](#-arquitetura-modular)
- [Hardware](#-hardware)
- [Pinout](#-pinout-completo)
- [Compilação](#-como-compilar)
- [Funcionalidades](#-funcionalidades)
- [Segurança](#-sistema-de-segurança-multicamadas)
- [Controle PID](#-controle-pid)
- [Estrutura de Arquivos](#-estrutura-de-arquivos)

---

## 🏗️ **Características do Projeto**

### Hardware:
- **Microcontrolador:** Raspberry Pi Pico (RP2040)
- **Display:** GMT020-02-7P TFT 240x320 pixels (Driver ST7789)
- **Sensor:** DHT22 (temperatura ±0.5°C, umidade ±2-5% RH)
- **Aquecimento:** Hotend 12V controlado por MOSFET IRLZ44N
- **Monitoramento de energia:** Sensor ACS712 (opcional)
- **Controle:** Botão para ajuste de temperatura (40-70°C)

### Software:
- **Linguagem:** C (C11 standard)
- **SDK:** Raspberry Pi Pico SDK 2.2.0
- **Build System:** CMake + Ninja
- **IDE:** Visual Studio Code + Raspberry Pi Pico Extension
- **Controle:** PID com PWM de 5kHz

---

## 🧩 **Arquitetura Modular**

O projeto está organizado em módulos independentes e reutilizáveis:

### **Módulos de Controle** (`src/controls/`)
- **`pid_controller`** - Controlador PID completo com anti-windup
- **`hardware_control`** - Controle PWM do heater e LED de status
- **`button_controller`** - Gerenciamento de botão com debounce

### **Módulos de Sensores** (`src/sensors/`)
- **`sensor_manager`** - Orquestrador central de todos os sensores
- **`dht22`** - Driver completo do sensor DHT22
- **`acs712`** - Monitor de consumo de energia (opcional)

### **Módulos de Interface** (`src/display/`)
- **`st7789_display`** - Driver de baixo nível do display TFT
- **`display_interface`** - Interface de alto nível com atualização inteligente

### **Utilitários** (`src/utils/`)
- **`logger.h`** - Sistema de logs categorizados (DEBUG, INFO, WARN, ERROR)

---

## 💻 **Hardware**

### Componentes Principais:

| Componente | Modelo/Especificação | Função |
|-----------|---------------------|---------|
| MCU | Raspberry Pi Pico (RP2040) | Processamento central |
| Display | GMT020-02-7P (ST7789) | Interface visual 240x320px |
| Sensor T/H | DHT22 (AM2302) | Temperatura e umidade |
| MOSFET | IRLZ44N | Controle do hotend (PWM 5kHz) |
| Sensor Corrente | ACS712 (5A/20A/30A) | Monitor de energia (opcional) |
| Hotend | 12V (tipo impressora 3D) | Elemento de aquecimento |
| Ventoinha | 12V | Circulação de ar |
| Botão | Push button | Ajuste de temperatura |

### Alimentação:
- **Pico:** USB 5V ou VSYS
- **Hotend/Fan:** Fonte 12V externa
- **Display:** 3.3V do Pico

---

## 🔌 **Pinout Completo**

### Sensor DHT22 (CRÍTICO):
```
DHT22 Pin 1 (VCC)  → Pico 3.3V ou 5V
DHT22 Pin 2 (DATA) → GPIO 22 + Resistor 10kΩ pull-up para VCC
DHT22 Pin 4 (GND)  → GND
```

### Display ST7789 (SPI):
```
Display → Pico
VCC     → 3.3V (Pin 36)
GND     → GND (Pin 38)
SCK     → GPIO 18 (SPI0 SCK)
MOSI    → GPIO 19 (SPI0 TX)
CS      → GPIO 17
DC      → GPIO 20 (Data/Command)
RST     → GPIO 21 (Reset)
BL      → 3.3V (backlight sempre ligado)
```

### Controles e Sensores:
```
Heater (PWM)      → GPIO 27 (via MOSFET IRLZ44N)
Button            → GPIO 16 + Pull-up interno
Energy Sensor     → GPIO 26 (ADC0)
LED Onboard       → GPIO 25 (Pico) ou CYW43 (Pico W)
```

### Circuito MOSFET (Heater):
```
GPIO 27 → Resistor 330-470Ω → IRLZ44N Gate
                              IRLZ44N Source → GND
                              IRLZ44N Drain  → Hotend (-)
                              Hotend (+)     → 12V
```

---

## 🔧 **Como Compilar**

### Pré-requisitos:
- Raspberry Pi Pico SDK 2.2.0+
- CMake 3.13+
- Ninja build system
- ARM GCC toolchain

### Usando VS Code (Recomendado):

1. **Abrir projeto:**
   ```bash
   code filament_dryer/
   ```

2. **Compilar:**
   - Pressione `Ctrl+Shift+B`
   - Ou use a tarefa: `Compile Project`

3. **Flash no Pico:**
   - Segure BOOTSEL no Pico e conecte USB
   - Arraste `build/filament_dryer.uf2` para o drive RPI-RP2

### Linha de Comando:

```bash
# 1. Configurar build
mkdir build
cd build
cmake -G Ninja ..

# 2. Compilar
ninja

# 3. Arquivo gerado: build/filament_dryer.uf2
```

---

## 🚀 **Funcionalidades**

### Controle de Temperatura:
- **Setpoint ajustável:** 40-70°C (ajuste via botão)
- **Controle PID:** Resposta suave e precisa
- **PWM:** 5kHz, duty cycle 0-100%
- **Proteção de overshoot:** Desliga se temp > setpoint + 4°C

### Monitoramento em Tempo Real:
- **Temperatura atual** (DHT22, ±0.5°C)
- **Umidade relativa** (DHT22, ±2-5% RH)
- **Consumo de energia** (ACS712, opcional)
- **PWM atual** (0-100%)
- **Uptime** do sistema
- **Estatísticas** de falhas

### Interface Visual (Display TFT):
- **Temperatura:** Grande, com setpoint
- **Umidade:** Percentual
- **Energia:** Watts e Wh acumulado
- **Status:** AQUECENDO / STANDBY
- **PWM:** Percentual de potência
- **Estatísticas:** Falhas de sensor, eventos unsafe

### LED de Status:
- **Pisca lento (1s):** Standby (PWM < 5%)
- **Pisca médio (250ms):** Aquecendo (PWM ≥ 5%)
- **Pisca rápido (100ms):** ⚠️ Erro crítico (sensor falhou)

### Logs Serial (USB):
```
[INFO ] Main: T:45.2°C H:35.0% E:48.50W Target:45°C Heater:ON(67%) [SAFE]
[WARN ] TempCtrl: SAFETY MODE: Heater disabled - Sensor failed
[ERROR] Main: CRITICAL OVERSHOOT: Temp 49.5°C > Target 45°C + 4°C!
```

---

## 🛡️ **Sistema de Segurança Multicamadas**

### Camada 1: Sensor Crítico DHT22
```c
if (!sensor_safe) {
    heater → PWM 0%
    pid_reset()
    Tela de erro crítico
}
```
- Sensor falha → **Heater desliga imediatamente**
- Sistema tenta recuperação automática
- Operação só retoma com sensor funcional

### Camada 2: Proteção de Overshoot
```c
if (temperatura > setpoint + 4°C) {
    heater → PWM 0%
    pid_reset()
    Log de erro crítico
}
```
- Previne superaquecimento
- Temperatura só pode ultrapassar 4°C do alvo

### Camada 3: Detecção de Falha do Heater
```c
if (PWM > 30% && corrente < 0.3A) {
    heater_failure = true
    Alerta no display
}
```
- Detecta heater queimado ou desconectado
- Sistema continua funcionando (só alerta)

### Camada 4: Anti-windup do PID
```c
integral_max = (output_max - output_min) * 2.0
// Limita acúmulo do termo integral
```
- Evita saturação do controlador
- Previne overshoot excessivo

---

## 🎛️ **Controle PID**

### Configuração Atual:
```c
Kp = 10.0   // Ganho proporcional
Ki = 0.5    // Ganho integral
Kd = 1.0    // Ganho derivativo

Sample time: 1 segundo
Output: 0-100% (PWM)
Frequency: 5kHz
```

### Características:
- **Derivativo sobre PV:** Evita "derivative kick" ao mudar setpoint
- **Anti-windup:** Limite de 2× o range de saída
- **Reset ao mudar setpoint:** Evita transientes (configurável)

### Tunning (Ajuste Fino):

Para ajustar os ganhos, edite `filament_dryer.c`:
```c
#define PID_KP 10.0f   // Aumentar → resposta mais rápida (pode oscilar)
#define PID_KI 0.5f    // Aumentar → elimina erro residual
#define PID_KD 1.0f    // Aumentar → reduz overshoot
```

**Método de ajuste:**
1. Comece com Ki=0, Kd=0
2. Aumente Kp até oscilar, depois reduza 50%
3. Aumente Ki até eliminar erro residual
4. Aumente Kd para reduzir overshoot (se necessário)

---

## 📁 **Estrutura de Arquivos**

```
filament_dryer/
├── CMakeLists.txt                 # Configuração de build
├── pico_sdk_import.cmake          # Import do SDK
├── README.md                      # Este arquivo
│
├── src/
│   ├── main/
│   │   └── filament_dryer.c       # Loop principal e orquestração
│   │
│   ├── controls/
│   │   ├── pid_controller.c/h     # Controlador PID completo
│   │   ├── hardware_control.c/h   # Controle PWM e LED
│   │   └── button_controller.c/h  # Gerenciamento de botão
│   │
│   ├── sensors/
│   │   ├── sensor_manager.c/h     # Orquestrador de sensores
│   │   ├── dht22.c/h              # Driver DHT22
│   │   └── acs712.c/h             # Monitor de energia
│   │
│   ├── display/
│   │   ├── st7789_display.c/h     # Driver low-level do display
│   │   └── display_interface.c/h  # Interface de alto nível
│   │
│   └── utils/
│       └── logger.h               # Sistema de logs
│
├── docs/
│   └── DHT22_README.md            # Documentação do DHT22
│
└── build/
    ├── filament_dryer.elf         # Executável
    ├── filament_dryer.uf2         # Firmware para flash
    └── compile_commands.json      # Para IntelliSense
```

---

## 🔄 **Fluxo de Operação**

```
Inicialização
     ↓
DHT22 leitura → sensor_safe?
     ↓              ↓ NO
    YES         PWM = 0%
     ↓          Tela erro
Temp atual         ↓
     ↓          Retry...
PID compute
     ↓
Overshoot > 4°C?
     ↓ NO
PWM output (0-100%)
     ↓
MOSFET → Heater
     ↓
Display update
     ↓
Loop (5s)
```

---

## 📊 **Dados Técnicos**

### Performance:
- **Update rate:** 5 segundos (limitado pelo DHT22)
- **PID sample time:** 1 segundo
- **PWM frequency:** 5kHz (período 200µs)
- **Display refresh:** Somente campos alterados (eficiente)

### Consumo Estimado:
- **Pico + Display:** ~150mA @ 5V
- **Hotend (100%):** ~3-5A @ 12V (36-60W)
- **Fan:** ~100mA @ 12V

### Temperatura Típica de Secagem:
- **PLA:** 45-50°C
- **PETG:** 50-55°C
- **ABS:** 60-65°C
- **Nylon:** 65-70°C

---

## 📚 **Documentação Adicional**

- [`docs/DHT22_README.md`](docs/DHT22_README.md) - Guia completo do sensor DHT22

---

## 🏆 **Versão Atual: v2.0**

Sistema completo com controle PID, proteções multicamadas e interface visual para secagem segura e eficiente de filamentos de impressão 3D.

### Histórico de Versões:
- **v2.0** (Nov 2025) - Controle PID implementado, proteção de overshoot, modularização completa
- **v1.0** (Nov 2025) - Sistema básico com histerese e sensores

---

**Autor:** Andre  
**Instituição:** Maua  
**Data:** Novembro 2025  
**Licença:** MIT (uso livre)