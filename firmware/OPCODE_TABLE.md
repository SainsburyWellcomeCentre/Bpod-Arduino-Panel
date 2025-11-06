# Opration Code Table

## ADIO.ino

The max voltage levels for all the I/O are configured by the on-board switches. Please refer to the hardware manual for more details.

## Digital Input Events

| Opcode     | Description                      |
| ---------- | -------------------------------- |
| 0x71, 0x72 | D2 Input Event High/Low          |
| 0x73, 0x74 | D3 Input Event High/Low          |
| 0x75, 0x76 | D4 Input Event High/Low          |
| ...        | (other opcodes remain unchanged) |
| 0x97, 0x98 | D21 Input Event High/Low         |

## Digital Input Enable/Disable

| Opcode | Description                      | Byte 1  | Byte 0   |
| ------ | -------------------------------- | ------- | -------- |
| 0x45   | Enable/Disable the digital input | channel | isEnable |

## Digital Output

| Opcode | Description                      | Byte 0 |
| ------ | -------------------------------- | ------ |
| 0x1E   | Set D30 Output High or Low       | logic  |
| 0x1F   | Set D31 Output High or Low       | logic  |
| 0x20   | Set D32 Output High or Low       | logic  |
| ...    | (other opcodes remain unchanged) | ...    |
| 0x35   | Set D53 Output High or Low       | logic  |

## Analog Input reading (16-bits resolution )

| Opcode | Description                      | Byte [1:0] |
| ------ | -------------------------------- | ---------- |
| 0x36   | read A0 voltage                  | ADC [1:0]  |
| 0x37   | read A1 voltage                  | ADC [1:0]  |
| ...    | (other opcodes remain unchanged) | ...        |
| 0x41   | read A11 voltage                 | ADC [1:0]  |

## Analog Output setting (16-bits resolution)

| Opcode | Description      | Byte [1:0] |
| ------ | ---------------- | ---------- |
| 0x42   | Set DAC0 voltage | DAC [1:0]  |
| 0x43   | Set DAC1 voltage | DAC [1:0]  |

## ModuleInfo

| Opcode | Description     |
| ------ | --------------- |
| 0xFF   | Get Module Info |
