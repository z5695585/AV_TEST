# Fire Alarm Audio-Visual Alert Firmware

## Target
- MCU: HC32L110C6PA (TSSOP-20, Cortex-M0+, 32KB Flash, 4KB RAM)
- Crystal: 8MHz XTH
- IDE: Keil MDK v5, ARMCC v5.06
- Programmer: J-Link (SWD)

## Pin Mapping
| Physical Pin | GPIO | Function |
|-------------|------|----------|
| 1  | P34 (28) | Buzzer PWM — TIM4_CHA |
| 2  | P35 (29) | Light DIP BIT0 |
| 3  | P36 (30) | Light DIP BIT1 |
| 5  | P01 (1)  | XTHI (8MHz) |
| 6  | P02 (2)  | XTHO (8MHz) |
| 11 | P15 (13) | UART0 RX |
| 12 | P14 (12) | UART0 TX |
| 15 | P25 (21) | LED PWM — TIM5_CHA |
| 17 | P27 (23) | SWDIO |
| 18 | P31 (25) | SWCLK |
| 19 | P32 (26) | Sound DIP BIT0 |
| 20 | P33 (27) | Sound DIP BIT1 |

## Hardware Critical Knowledge

### UART0 Baud Rate
- **Must use BT0 (Base Timer0) with TOG_EN=1** for baud rate clock
- `Uart_SetBaudRate()` only returns reload value — does NOT configure timer
- All UART TX/RX must use direct register access (not SDK Uart_SendData which busy-waits)
- DBAUD=0 (normal baud), SM01=1 (Mode1, 8-N-1)
- `Gpio_SetFunc_UART0TX_P14()` / `Gpio_SetFunc_UART0RX_P15()`
- **DO NOT call SDK `Uart_Init()` / `Uart_SendData()` / `Uart_EnableFunc()`** — they were bypassed in v0.1 for a reason

### PWM (ADT)
- Buzzer: TIM4_CHA (AdTIM4), LED: TIM5_CHA (AdTIM5)
- Mode: edge-aligned active-high, sawtooth, no prescaler
- `AdtCHxCompareLow` + `AdtCHxPeriodHigh` + `AdtCHxPortOutHigh`
- `Adt_CHxXPortConfig()` required to drive physical pin
- `Clk_SetPeripheralGate(ClkPeripheralGpio, TRUE)` BEFORE `Gpio_SetFunc_XXX`
- NO `Adt_EnableValueBuf` — causes stale compare values
- `pwm_reconfig()`: stop→clear counter→set period+compare→start (atomic)

### DIP Switches
- Active LOW (common to GND), software inverts
- 50ms debounce

## Architecture
```
main.c
  ├── alarm_sound / alarm_light  (per-channel engine wrappers)
  │     └── pattern_engine       (shared state machine)
  │           └── pattern_table  (step definitions)
  ├── test_protocol              (UART command parser)
  └── hal/hc32l110/
        ├── pwm_drv              (ADT TIM4/TIM5 register ops)
        ├── uart_drv             (UART0 + BT0 register ops)
        ├── timer_drv            (SysTick 1ms)
        └── gpio_drv             (GPIO helpers)
```

## Pattern Engine
- `cmd_mode`: when set by BUZZ/LED ON, DIP changes ignored for that channel
- `freq_ovr` / `duty_ovr`: overrides set by FREQ/DUTY commands
- OFF steps (freq_hz==0) are always respected, overrides don't apply there
- `pwm_reconfig()` used for all parameter changes to prevent first-cycle glitch
- `pattern_engine_clear_overrides()` on RESET, clears cmd_mode + overrides

## UART Protocol
- 9600-8-N-1, polled (no interrupts)
- Commands: line-based, `\r\n` delimited (terminal must append CR+LF)
- Banner sent 4s after power-on
- Reply via blocking `uart_send_str()` — short enough to not affect PWM

## Build
- Open `mdk/fire-alarm.uvprojx` in Keil MDK
- Source files in SDK: bt.c must be manually added if BT API used
  (currently bypassed with direct register access, so NOT needed)
- Output: `mdk/output/fire-alarm.axf` / `fire-alarm.hex`

## Testing
- Serial terminal: 9600-8-N-1, CR+LF enabled
- Scope: PIN1 (buzzer PWM), PIN15 (LED PWM), PIN12 (UART TX), PIN26 (heartbeat)
- `PING` → `PONG`, `DIP` → DIP positions, `STATUS` → full state
