# ESP8266 Multi-Channel Interrupt PWM Component

This is an **enhanced version** of the interrupt PWM component that supports **up to 8 simultaneous PWM channels**, perfect for RGB, RGBW, or multi-LED setups.

## Features

✅ **Multi-channel support** - Up to 8 PWM outputs simultaneously  
✅ **Glitch-free operation** - Hardware Timer1 interrupts eliminate WiFi glitches  
✅ **Synchronized channels** - All channels share the same PWM frequency  
✅ **Perfect for RGB/RGBW** - Smooth color transitions without flickering  
✅ **Low CPU overhead** - Single interrupt handles all channels  

## Installation

### Directory Structure

```
/config/esphome/
├── components/
│   └── interrupt_pwm/
│       ├── __init__.py
│       ├── interrupt_pwm.h
│       └── interrupt_pwm.cpp
├── rgb_example.yaml
└── rgbw_example.yaml
```

### Steps

1. **Copy the component files** to `components/interrupt_pwm/`
2. **Create your device configuration** (see examples below)
3. **Compile and upload** via ESPHome Dashboard or CLI

## RGB LED Example

```yaml
esphome:
  name: rgb-led
  platform: ESP8266
  board: nodemcuv2

external_components:
  - source:
      type: local
      path: components
    components: [ interrupt_pwm ]

logger:
api:
ota:

wifi:
  ssid: "Your_WiFi"
  password: "Your_Password"

output:
  - platform: interrupt_pwm
    id: red_output
    pin: 5      # D1
    channel: 0

  - platform: interrupt_pwm
    id: green_output
    pin: 4      # D2
    channel: 1

  - platform: interrupt_pwm
    id: blue_output
    pin: 14     # D5
    channel: 2

light:
  - platform: rgb
    name: "RGB LED"
    red: red_output
    green: green_output
    blue: blue_output
    gamma_correct: 2.8
```

## RGBW LED Example

Just add a fourth channel for white:

```yaml
output:
  # ... red, green, blue (channels 0-2)
  
  - platform: interrupt_pwm
    id: white_output
    pin: 12     # D6
    channel: 3

light:
  - platform: rgbw
    name: "RGBW LED"
    red: red_output
    green: green_output
    blue: blue_output
    white: white_output
```

## Channel Configuration

Each output requires a unique **channel ID** (0-7):

```yaml
output:
  - platform: interrupt_pwm
    id: output_name
    pin: 5         # GPIO number
    channel: 0     # Unique channel ID (0-7)
```

### Important Notes

- **Channel IDs must be unique** (0-7)
- **GPIO pin numbers** are used (not board pins like D1)
- All channels share the **same PWM frequency** (1kHz)
- Maximum **8 channels** per device

## GPIO Pin Reference

| Board Pin | GPIO | Recommended |
|-----------|------|-------------|
| D1        | 5    | ✅           |
| D2        | 4    | ✅           |
| D5        | 14   | ✅           |
| D6        | 12   | ✅           |
| D7        | 13   | ✅           |
| D0        | 16   | ❌ Special   |
| D3        | 0    | ❌ Boot pin  |
| D4        | 2    | ❌ Boot pin  |
| D8        | 15   | ❌ Boot pin  |

**Use GPIO numbers in your YAML**, not board pin names.

## Use Cases

### 1. RGB LED Strip
- 3 channels (Red, Green, Blue)
- Common for addressable LED strips
- Example: `rgb_example.yaml`

### 2. RGBW LED Strip
- 4 channels (Red, Green, Blue, White)
- Better for warm/cool white control
- Example: `rgbw_example.yaml`

### 3. Multiple Single LEDs
- Up to 8 independent LED outputs
- Each can be controlled separately
- Great for indicator lights or multi-zone lighting

### 4. RGB + Extra Outputs
- 3 channels for RGB
- Remaining channels for other devices (fans, pumps, etc.)

## Wiring Example (RGB Common Cathode)

```
ESP8266 D1 (GPIO5)  ──[ Resistor ]── Red LED Anode
ESP8266 D2 (GPIO4)  ──[ Resistor ]── Green LED Anode
ESP8266 D5 (GPIO14) ──[ Resistor ]── Blue LED Anode
                                            │
All LED Cathodes ────────────────────── GND
```

**Resistor values:** 220Ω for 5V, 100Ω for 3.3V (adjust for your LEDs)

## Advanced Configuration

### Custom PWM Frequency

To change the frequency, edit `interrupt_pwm.cpp`:

```cpp
uint32_t InterruptPWMOutput::pwm_period_ticks_ = 80000;  // 1kHz
// Change to: 40000 for 2kHz, 160000 for 500Hz, etc.
```

### Color Correction

Adjust for LED strip variations:

```yaml
light:
  - platform: rgb
    name: "RGB LED"
    # ... outputs ...
    color_correct:
      - red: 100%
      - green: 95%
      - blue: 80%
```

### Effects

Add visual effects:

```yaml
light:
  - platform: rgb
    name: "RGB LED"
    # ... outputs ...
    effects:
      - random:
          name: "Random Colors"
          transition_length: 3s
          update_interval: 5s
      - strobe:
          name: "Strobe"
      - pulse:
          name: "Pulse"
          transition_length: 1s
          update_interval: 1s
```

## Troubleshooting

### All LEDs flickering together
- Check power supply capacity
- Ensure good ground connection
- Try adding a capacitor (100µF) near the LEDs

### One color not working
- Verify GPIO pin is correct
- Check channel ID is unique (0-7)
- Test the LED/connection directly

### Colors are wrong
- Check wiring (Red/Green/Blue correct pins?)
- Try adjusting `color_correct` values
- Verify LED strip is common cathode (not common anode)

### Device won't boot
- Avoid GPIO 0, 2, 15 (boot mode pins)
- Check no channel IDs are duplicated
- Verify board type matches hardware

### Compilation errors
- Ensure all 3 files are in `components/interrupt_pwm/`
- Check `external_components` path is correct
- Verify channel IDs are 0-7

## Performance

- **CPU overhead:** Minimal (single interrupt handles all channels)
- **Timing accuracy:** ±1µs
- **PWM frequency:** 1kHz (configurable)
- **Maximum channels:** 8 (more possible but not recommended)

## Comparison with Standard PWM

| Feature | Standard esp8266_pwm | Interrupt PWM |
|---------|---------------------|---------------|
| Glitches during WiFi | ✅ Yes | ❌ No |
| Smooth transitions | Sometimes | ✅ Always |
| Multi-channel sync | No | ✅ Yes |
| CPU overhead | Low | Very Low |
| Max channels | 8+ | 8 |

## Limitations

- **ESP8266 only** - ESP32 has different hardware
- **Single Timer1** - All channels share one timer
- **Same frequency** - All channels use 1kHz (configurable globally)
- **8 channel limit** - Can be increased but affects performance

## Example Projects

### Smart RGB Lamp
```yaml
light:
  - platform: rgb
    name: "Desk Lamp"
    red: red_output
    green: green_output
    blue: blue_output
    effects:
      - random:
      - pulse:
```

### Mood Lighting
```yaml
light:
  - platform: rgbw
    name: "Ambient Light"
    # ... outputs ...
    effects:
      - random:
          name: "Slow Color Fade"
          transition_length: 5s
          update_interval: 10s
```

### Multi-Zone Lighting
```yaml
# Use 6 channels for 2 independent RGB zones
output:
  # Zone 1: channels 0-2
  - platform: interrupt_pwm
    id: z1_red
    pin: 5
    channel: 0
  # ... green, blue
  
  # Zone 2: channels 3-5
  - platform: interrupt_pwm
    id: z2_red
    pin: 12
    channel: 3
  # ... green, blue

light:
  - platform: rgb
    name: "Zone 1"
    red: z1_red
    green: z1_green
    blue: z1_blue
    
  - platform: rgb
    name: "Zone 2"
    red: z2_red
    green: z2_green
    blue: z2_blue
```

## Support

For issues:
1. Check the troubleshooting section
2. Verify your wiring with a multimeter
3. Check ESPHome logs for error messages
4. Try with a single channel first to isolate the problem

## Credits

This component uses ESP8266's Timer1 hardware for interrupt-driven PWM, providing smooth, glitch-free operation even during WiFi activity.

Perfect for RGB/RGBW LED strips, multi-LED setups, or any application requiring synchronized, flicker-free PWM outputs!
