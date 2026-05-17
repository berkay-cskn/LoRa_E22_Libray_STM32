# LoRa E22 STM32 Library

An interrupt-driven LoRa E22 driver for STM32 microcontrollers, built on top of a ring-buffer UART abstraction layer. Supports multiple simultaneous modules, fixed-point transmission, RSSI reporting, and a clean state-machine packet parser.

> [!WARNING]
> This library is currently under active development.
---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [File Structure](#file-structure)
- [Dependencies](#dependencies)
- [Quick Start](#quick-start)
- [API Reference](#api-reference)
  - [RingBuffer](#ringbuffer)
  - [UartHandler](#uarthandler)
  - [LoRa E22](#lora-e22)
- [Packet Format](#packet-format)
- [Configuration Reference](#configuration-reference)
- [Operating Modes](#operating-modes)
- [Parser State Machine](#parser-state-machine)
- [Multi-Module Setup](#multi-module-setup)
- [Interrupt Handlers](#interrupt-handlers)
- [Known Limitations](#known-limitations)

---

## Overview

This library provides a full driver stack for the **EBYTE E22** LoRa module series on STM32 (HAL-based) platforms. It is structured in three independent layers:

```
Application
    │
    ▼
LoRa_E22        ← packet framing, config, register I/O
    │
    ▼
UartHandler     ← interrupt-driven TX/RX, txCplt flag
    │
    ▼
RingBuffer      ← circular byte buffer (heap-allocated)
```

Each layer can be used independently if needed.

---

## Architecture

### Data Flow — Receiving

```
[E22 Module]
     │  UART RX interrupt fires per byte
     ▼
HAL_UART_RxCpltCallback()
     │  enqueues byte
     ▼
RingBuffer_Enqueue()
     │
     ▼  (called from main loop)
Lora_Process()
     │  dequeues & feeds state machine
     ▼
Lora_ProcessNormalData()   ← normal data mode
     │  or
Lora_ProcessCommandByte()  ← command response mode (blocking)
     │
     ▼
lora->rxBuffer + lora->dataReady = 1
```

### Data Flow — Transmitting

```
Application calls Lora_Write()
     │  builds [ADDH][ADDL][CH][PREFIX][LEN][DATA][SUFFIX]
     ▼
Uart_Write()
     │  HAL_UART_Transmit_IT()
     ▼
HAL_UART_TxCpltCallback()  → txCplt = 1
```

---

## File Structure

```
├── RingBuffer.h / RingBuffer.c     # Circular buffer
├── uart.h / uart.c                 # UART abstraction layer
├── LoRa_E22.h / LoRa_E22.c        # LoRa E22 driver
└── main.c                          # Example application
```

---

## Dependencies

- STM32 HAL (`stm32f4xx_hal.h` or equivalent for your series)
- `string.h` (for `memcpy`, `memset`)
- `stdlib.h` (for `malloc` inside `RingBuffer_Init`)

> Tested on **STM32F4** series. Porting to other STM32 families requires only changing the HAL include path.

---

## Quick Start

### 1. Initialize the UART Layer

```c
RingBuffer_t rb;
UartHandler_t uartHandler;

uartHandler.ringBuffer = &rb;
uartHandler.uart       = &huart4;   // your HAL UART handle

Uart_Init(&uartHandler);
```

### 2. Configure the LoRa Module

```c
Lora_Init_t loraInit = {
    .uart        = &uartHandler,
    .baudRate    = 9600,
    .bufferSize  = 240,
    .prefix      = 0xAE,
    .suffix      = 0xDD,
    .m0Pin       = E22_M0_Pin,
    .m0Port      = E22_M0_GPIO_Port,
    .m1Pin       = E22_M1_Pin,
    .m1Port      = E22_M1_GPIO_Port,
    .auxPin      = E22_AUX_Pin,
    .auxPort     = E22_AUX_GPIO_Port,
};

Lora_t lora;
Lora_Init(&lora, &loraInit, &uartHandler);
```

`Lora_Init` automatically:
1. Switches the module to **Configuration mode**
2. Reads the current register configuration (`Lora_GetConfig`)
3. Returns to **Normal mode**

### 3. Main Loop

```c
uint8_t rxBuf[240];

while (1) {
    Lora_Process(&lora);   // feed the parser

    if (Lora_IsDataReady(&lora)) {
        uint8_t len = Lora_Read(&lora, rxBuf, sizeof(rxBuf));
        // process rxBuf[0..len-1]
    }

    // send data every 200 ms
    if (HAL_GetTick() - lastTx > 200) {
        lastTx = HAL_GetTick();
        Lora_Write(&lora, 0x0065, 18, txBuf, txLen);
    }
}
```

---

## API Reference

### RingBuffer

| Function | Description |
|---|---|
| `RingBuffer_Init(rb, size)` | Allocates `size` bytes on the heap and initialises the buffer |
| `RingBuffer_Enqueue(rb, data, len)` | Writes `len` bytes; silently drops if space is insufficient |
| `RingBuffer_Dequeue(rb, buf, len)` | Reads and removes `len` bytes; silently skips if not enough data |
| `RingBuffer_Peek(rb, buf, len)` | Reads `len` bytes without advancing the tail pointer |

**Structure fields:**

```c
typedef struct {
    uint8_t * const buffer;   // heap-allocated storage
    uint16_t head;            // write pointer
    uint16_t tail;            // read pointer
    uint16_t size;            // total capacity
    uint16_t bytesToRead;     // bytes currently available
} RingBuffer_t;
```

---

### UartHandler

| Function | Description |
|---|---|
| `Uart_Init(handler)` | Initialises the ring buffer, registers the handler, starts `Receive_IT` |
| `Uart_Register(handler)` | Adds handler to the global lookup table used by callbacks |
| `Uart_ReadByte(handler, data)` | Pops one byte from the ring buffer; returns `1` on success |
| `Uart_ReadPacket(handler, buf, len)` | Pops `len` bytes atomically; returns `1` on success |
| `Uart_Write(handler, data, size)` | Sends `size` bytes via `HAL_UART_Transmit_IT`; returns `0` if previous TX is not complete |

**`txCplt` flag:** set to `0` before transmit, restored to `1` inside `HAL_UART_TxCpltCallback`. Always check the return value of `Uart_Write` to detect a busy condition.

> **Important:** `MAX_UART_COUNT` (default `3`) limits the number of simultaneously registered handlers. Increase this define if you need more.

---

### LoRa E22

#### Initialisation

```c
Lora_Status_t Lora_Init(Lora_t *lora, Lora_Init_t *initConfig, UartHandler_t *uartHandler);
```

#### Mode Control

```c
Lora_Status_t Lora_SetMode(Lora_t *lora, Lora_Mode_t newMode);
```

| Mode | M1 | M0 | Description |
|---|---|---|---|
| `LORA_MODE_NORMAL` | 0 | 0 | Regular TX/RX |
| `LORA_MODE_WOR` | 0 | 1 | Wake-on-Radio |
| `LORA_MODE_CONFIGURATION` | 1 | 0 | Register read/write via UART |
| `LORA_MODE_DEEP_SLEEP` | 1 | 1 | Lowest power state |

#### Sending Data

```c
Lora_Status_t Lora_Write(Lora_t *lora,
                          uint16_t targetAddress,
                          uint8_t  targetChannel,
                          uint8_t *data,
                          uint8_t  length);
```

- Requires `LORA_MODE_NORMAL` or `LORA_MODE_WOR`
- Module must be ready (AUX pin HIGH)
- `length` must be between 1 and `LORA_MAX_BUFFER_SIZE` (240)

#### Receiving Data

```c
// Call every main loop iteration
void Lora_Process(Lora_t *lora);

// Check if a complete packet is available
uint8_t Lora_IsDataReady(Lora_t *lora);

// Copy parsed payload into your buffer; returns actual byte count
uint8_t Lora_Read(Lora_t *lora, uint8_t *data, uint8_t length);
```

#### Configuration

```c
// Read register config into lora->config
Lora_Status_t Lora_GetConfig(Lora_t *lora);

// Write a Lora_Config_t to the module (must be in CONFIGURATION mode)
Lora_Status_t Lora_SetConfig(Lora_t *lora, Lora_Config_t *config);
```

#### Low-Level Register Access

```c
Lora_Status_t Lora_WriteRegister(Lora_t *lora,
                                  uint8_t registerAddress,
                                  uint8_t length,
                                  uint8_t *parameter);

Lora_Status_t Lora_ReadRegister(Lora_t *lora,
                                 uint8_t registerAddress,
                                 uint8_t length,
                                 uint8_t *outBuffer,
                                 uint8_t outLen);
```

Both functions are **blocking** and will return `LORA_STATUS_TIMEOUT` if no response is received within `LORA_COMMAND_TIMEOUT_MS` (default 1000 ms).

#### Status Helpers

```c
uint8_t Lora_IsModuleReady(Lora_t *lora);          // AUX pin == HIGH
uint8_t Lora_IsDataReady(Lora_t *lora);             // complete packet received
uint8_t Lora_IsCommandResponseReady(Lora_t *lora);  // command echo received
```

#### Return Codes

| Code | Meaning |
|---|---|
| `LORA_STATUS_SUCCESS` | Operation completed successfully |
| `LORA_STATUS_FAIL` | Generic failure (NULL pointer, UART error, etc.) |
| `LORA_STATUS_TIMEOUT` | No response within `LORA_COMMAND_TIMEOUT_MS` |
| `LORA_STATUS_WRONG_FORMAT` | Response header was malformed |
| `LORA_STATUS_INVALID_MODE` | Operation not allowed in current mode |
| `LORA_STATUS_INVALID_CONFIG` | Configuration parameter out of range |

---

## Packet Format

### Outgoing (Fixed-Point Transmission)

```
┌──────┬──────┬─────────┬────────┬────────┬──────────────┬────────┐
│ ADDH │ ADDL │ CHANNEL │ PREFIX │ LENGTH │   PAYLOAD    │ SUFFIX │
│ 1 B  │ 1 B  │  1 B    │  1 B   │  1 B   │  1–240 B     │  1 B   │
└──────┴──────┴─────────┴────────┴────────┴──────────────┴────────┘
```

### Incoming (with RSSI enabled)

```
┌────────┬────────┬──────────────┬────────┬──────┐
│ PREFIX │ LENGTH │   PAYLOAD    │ SUFFIX │ RSSI │
│  1 B   │  1 B   │  1–240 B     │  1 B   │ 1 B  │
└────────┴────────┴──────────────┴────────┴──────┘
```

If `RSSIEnabled == 0` the trailing RSSI byte is not expected and the parser skips directly from SUFFIX to complete state.

The parsed RSSI value is stored in `lora->RSSI` and can be read at any time after `Lora_IsDataReady()` returns true.

---

## Configuration Reference

```c
typedef struct {
    uint8_t  ADDH;                    // Address high byte
    uint8_t  ADDL;                    // Address low byte
    uint8_t  NETID;                   // Network ID
    uint8_t  AirDataRate    : 3;      // See LORA_AIR_DATA_RATE_* defines
    uint8_t  ParityBit      : 2;      // UART parity
    uint8_t  BaudRate       : 3;      // See LORA_UART_BAUD_RATE_* defines
    uint8_t  TransmittingPower : 2;   // See LORA_TRANSMITTING_POWER_* defines
    uint8_t  Reserved       : 3;
    uint8_t  AmbientRSSI    : 1;      // Enable ambient RSSI
    uint8_t  SubPacketSize  : 2;      // See LORA_SUB_PACKET_SIZE_* defines
    uint8_t  Channel;                 // Frequency channel
    uint8_t  WORCycle       : 3;      // WOR listen interval
    uint8_t  WORTransceiverControl : 1;
    uint8_t  LBTEnabled     : 1;      // Listen Before Talk
    uint8_t  RepeaterEnabled : 1;
    uint8_t  FixedPointTransmission : 1;  // Must be 1 for Lora_Write addressing
    uint8_t  RSSIEnabled    : 1;      // Append RSSI byte to received packets
    uint16_t KEY;                     // Encryption key
} Lora_Config_t;
```

### Baud Rate Options

| Define | Rate |
|---|---|
| `LORA_UART_BAUD_RATE_1200` | 1200 bps |
| `LORA_UART_BAUD_RATE_2400` | 2400 bps |
| `LORA_UART_BAUD_RATE_4800` | 4800 bps |
| `LORA_UART_BAUD_RATE_9600` | 9600 bps |
| `LORA_UART_BAUD_RATE_19200` | 19200 bps |
| `LORA_UART_BAUD_RATE_38400` | 38400 bps |
| `LORA_UART_BAUD_RATE_57600` | 57600 bps |
| `LORA_UART_BAUD_RATE_115200` | 115200 bps |

### Air Data Rate Options

| Define | Rate |
|---|---|
| `LORA_AIR_DATA_RATE_0_3` | 0.3 kbps |
| `LORA_AIR_DATA_RATE_1_2` | 1.2 kbps |
| `LORA_AIR_DATA_RATE_2_4` | 2.4 kbps |
| `LORA_AIR_DATA_RATE_4_8` | 4.8 kbps |
| `LORA_AIR_DATA_RATE_9_6` | 9.6 kbps |
| `LORA_AIR_DATA_RATE_19_2` | 19.2 kbps |
| `LORA_AIR_DATA_RATE_38_4` | 38.4 kbps |
| `LORA_AIR_DATA_RATE_62_5` | 62.5 kbps |

### Transmitting Power Options

| Define | Power |
|---|---|
| `LORA_TRANSMITTING_POWER_30` | 30 dBm |
| `LORA_TRANSMITTING_POWER_27` | 27 dBm |
| `LORA_TRANSMITTING_POWER_24` | 24 dBm |
| `LORA_TRANSMITTING_POWER_21` | 21 dBm |

### Sub-Packet Size Options

| Define | Size |
|---|---|
| `LORA_SUB_PACKET_SIZE_240` | 240 bytes |
| `LORA_SUB_PACKET_SIZE_128` | 128 bytes |
| `LORA_SUB_PACKET_SIZE_64` | 64 bytes |
| `LORA_SUB_PACKET_SIZE_32` | 32 bytes |

---

## Operating Modes

### Normal Mode (M1=0, M0=0)

Default operating mode. Full-speed TX and RX. Use `Lora_Write` and `Lora_Process` here.

### WOR Mode (M1=0, M0=1)

Wake-on-Radio. The module listens periodically. `Lora_Write` is still permitted. The `WORCycle` and `WORTransceiverControl` config fields control the listen interval and whether the device acts as a transmitter or receiver in this mode.

### Configuration Mode (M1=1, M0=0)

Required for `Lora_SetConfig`, `Lora_GetConfig`, `Lora_WriteRegister`, and `Lora_ReadRegister`. The module communicates at **9600 bps** regardless of the configured baud rate while in this mode.

> **Note:** The baud rate switching between configuration mode and normal mode is currently commented out in `Lora_SetMode`. If your module's normal operating baud rate differs from 9600, you will need to implement baud rate switching around mode changes.

### Deep Sleep Mode (M1=1, M0=1)

Lowest power consumption. Configuration can be changed in this mode on some E22 variants. The module is unresponsive to UART communication.

---

## Parser State Machine

`Lora_Process` drives a two-mode state machine.

### Normal Data Mode

```
WAITING_PREFIX
      │  byte == prefix
      ▼
WAITING_LENGTH
      │  0 < length <= 240
      ▼
RECEIVING_DATA
      │  dataIndex >= expectedLength
      ▼
WAITING_SUFFIX
      │  byte == suffix
      ├─── RSSIEnabled == 0 ──► copy to rxBuffer, dataReady = 1 ──► WAITING_PREFIX
      └─── RSSIEnabled == 1 ──►
WAITING_RSSI
      │  any byte (RSSI value)
      ▼
copy to rxBuffer, dataReady = 1 ──► WAITING_PREFIX
```

Any unexpected byte at any stage resets the machine to `WAITING_PREFIX`.

### Command Response Mode

Entered automatically by `Lora_RegisterIO` before sending a register command. The parser counts incoming bytes until `cmdExpectedBytes` is reached, then sets `cmdResponseReady = 1`. This mode is **blocking** — `Lora_WaitCommandResponse` polls in a tight loop with a timeout guard.

---

## Multi-Module Setup

Up to `LORA_DEVICE_COUNT` (default `2`) modules can run simultaneously. Each requires its own `Lora_t`, `UartHandler_t`, `RingBuffer_t`, and hardware UART peripheral.

```c
Lora_t lora1, lora2;
UartHandler_t uart1, uart2;
RingBuffer_t  rb1,   rb2;

// --- Module 1 ---
uart1.ringBuffer = &rb1;
uart1.uart       = &huart4;
Uart_Init(&uart1);

Lora_Init_t init1 = { /* ... pins for module 1 ... */ };
Lora_Init(&lora1, &init1, &uart1);

// --- Module 2 ---
uart2.ringBuffer = &rb2;
uart2.uart       = &huart3;
Uart_Init(&uart2);

Lora_Init_t init2 = { /* ... pins for module 2 ... */ };
Lora_Init(&lora2, &init2, &uart2);
```

Call `Lora_Process` for every registered module in the main loop.

---

## Interrupt Handlers

The following HAL callbacks must be reachable — they are defined in `uart.c` and `LoRa_E22.c`.

### `HAL_UART_RxCpltCallback`
Defined in `uart.c`. Enqueues the received byte into the matching handler's ring buffer and re-arms `Receive_IT`. Do **not** define this callback elsewhere.

### `HAL_UART_TxCpltCallback`
Defined in `uart.c`. Restores the `txCplt` flag so the next `Uart_Write` can proceed.

### `Lora_AUX_IRQHandler`
Call this from your GPIO EXTI handler, passing the triggering pin number:

```c
// In stm32f4xx_it.c
void EXTI9_5_IRQHandler(void) {
    HAL_GPIO_EXTI_IRQHandler(E22_AUX_Pin);
    HAL_GPIO_EXTI_IRQHandler(E22_2_AUX_Pin);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    Lora_AUX_IRQHandler(GPIO_Pin);
}
```

`Lora_AUX_IRQHandler` iterates the registered device list and calls `Lora_UpdateModuleStatus` on the matching device.

---

## Known Limitations

- **No baud rate switching on mode change.** The commented-out baud rate code in `Lora_SetMode` means the STM32 UART peripheral must be initialised at 9600 bps for configuration commands to work correctly, regardless of the module's normal operating speed.
- **Blocking register I/O.** `Lora_WriteRegister` / `Lora_ReadRegister` block the calling task for up to `LORA_COMMAND_TIMEOUT_MS`. This is unsuitable for RTOS environments without modification.
- **No overflow recovery in RingBuffer.** If the ring buffer fills up before `Lora_Process` is called, incoming bytes are silently dropped. Increase `RING_BUFFER_SIZE` (default 256) if high-throughput scenarios are expected.
- **Static TX payload buffer.** The `payload` array inside `Lora_Write` is declared `static`, making the function non-reentrant. This is safe in a single-threaded bare-metal context.
- **`Lora_UpdateModuleStatus` not implemented.** The function is declared but its body is absent from the provided source; the `moduleReady` flag is therefore only updated via `Lora_IsModuleReady` (live AUX pin read).
