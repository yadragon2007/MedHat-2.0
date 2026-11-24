# ESP32 (30-pin) Connection Guide (HumanESP32)

Project: **2 servos, 4 LEDs, 4 push buttons, 3 potentiometers**

> **Code pin** is the GPIO number you write in code, for example `pinMode(16, INPUT_PULLUP)`.
> **Physical pin** is the position (1 to 30) on the board diagram. You never use it in code.

---

## 1. Pin summary

| Part | Qty | Code pins |
|---|---|---|
| Servos (signal) | 2 | **18, 19** |
| Potentiometers (middle leg) | 3 | **32, 34, 35** |
| Push buttons | 4 | **2, 17, 4, 16** |
| LEDs (through resistors) | 4 | **23, 25, 26, 27** |

---

## 2. Full connection table

| Part | Name | Code pin | Physical pin | Side of board | Type |
|---|---|---|---|---|---|
| Servo 1 | signal | **18** | 24 | Right | PWM output |
| Servo 2 | signal | **19** | 25 | Right | PWM output |
| Pot 1 | middle leg | **32** | 6 | Left | ADC1 input |
| Pot 2 | middle leg | **34** | 4 | Left | ADC1, input only |
| Pot 3 | middle leg | **35** | 5 | Left | ADC1, input only |
| Button 1 | one leg | **16** | 21 | Right | Digital input, pull-up |
| Button 2 | one leg | **17** | 22 | Right | Digital input, pull-up |
| Button 3 | one leg | **4** | 20 | Right | Digital input, pull-up |
| Button 4 | one leg | **15** | 18 | Right | Digital input, pull-up |
| LED 1 | resistor side | **23** | 30 | Right | Digital output |
| LED 2 | resistor side | **25** | 8 | Left | Digital output |
| LED 3 | resistor side | **26** | 9 | Left | Digital output |
| LED 4 | resistor side | **27** | 10 | Left | Digital output |

### Power pins

| Pin | Physical pin | Use |
|---|---|---|
| **3V3** | 16 | Power for the pots |
| **5V (VIN)** | 15 | Power for the servos (or use an external 5V supply) |
| **GND** | 14 (left), 17 (right) | Common ground for everything |

---

## 3. Wiring details

### Servos (x2)

```
Servo wire   Connect to
----------   ----------------------------------
Signal       GPIO18 (servo 1) / GPIO19 (servo 2)
Red (+)      5V (external supply recommended)
Brown (-)    GND (shared with the ESP32 GND)
```

- Two servos moving at the same time can draw enough current to **reset the ESP32**. If you see random reboots, use an **external 5V supply** for the servos.
- **Connect the external supply's GND to the ESP32 GND**, otherwise the signal will not work.
- Use the `ESP32Servo` library.

### Potentiometers (x3)

```
Pot leg      Connect to
----------   ----------------------
Left leg     GND
Middle leg   GPIO32 / GPIO34 / GPIO35
Right leg    3V3  (never 5V)
```

- The ADC reads **0 to 3.3V** as **0 to 4095**. More than 3.3V can damage the pin.
- All three pins are **ADC1**, so they work with **WiFi on or off**.

### Push buttons (x4)

```
Button leg A  ->  GPIO (16 / 17 / 4 / 15)
Button leg B  ->  GND
```

- No external resistor needed. Use the internal pull-up: `pinMode(pin, INPUT_PULLUP)`.
- Released reads **HIGH**, pressed reads **LOW**.
- Do not hold the GPIO15 button while the ESP32 boots. It only silences the boot log, which is harmless.

### LEDs (x4)

```
GPIO -> 220 to 330 ohm resistor -> LED long leg (+) -> LED short leg (-) -> GND
```

- Always use a resistor, otherwise you can burn the LED or the pin.

---

## 4. Wiring diagram (text)

```
                 ESP32 30-pin
                +-------------+
  Pot1 -------- | 32      23  | -------- LED1
  Pot2 -------- | 34      19  | -------- Servo2 signal
  Pot3 -------- | 35      18  | -------- Servo1 signal
  LED2 -------- | 25      17  | -------- Button2
  LED3 -------- | 26      16  | -------- Button1
  LED4 -------- | 27       4  | -------- Button3
                |         15  | -------- Button4
  3V3 --------- | 3V3    GND  | -------- common GND
  5V  --------- | 5V/VIN      |
                +-------------+
```

This is a simplified layout. Check the real pin positions against your pinout diagram.


---

## 6. Free (unused) pins

| Code pin | Digital read | Digital write | Analog read | PWM | Notes |
|---|---|---|---|---|---|
| **33** | Yes | Yes | ADC1 (WiFi on or off) | Yes | Best spare analog pin |
| **36** | Yes | No | ADC1 (WiFi on or off) | No | Input only, no pull-up |
| **39** | Yes | No | ADC1 (WiFi on or off) | No | Input only, no pull-up |
| **13** | Yes | Yes | ADC2 (WiFi off only) | Yes | Best spare digital pin |
| **14** | Yes | Yes | ADC2 (WiFi off only) | Yes | Outputs PWM at boot |
| **21** | Yes | Yes | No | Yes | I2C SDA, keep for sensors or OLED |
| **22** | Yes | Yes | No | Yes | I2C SCL, keep for sensors or OLED |
| **5** | Yes | Yes | No | Yes | Strapping pin, PWM at boot |
| **12** | Yes | Yes | ADC2 (WiFi off only) | Yes | Strapping pin, must be LOW at boot |
| **2** | Yes | Yes | ADC2 (WiFi off only) | Yes | Strapping pin, onboard LED |
| **1** | Yes | Yes | No | Yes | TX0, USB serial, avoid |
| **3** | Yes | Yes | No | Yes | RX0, USB serial, avoid |

---

## 7. Important notes

- **ADC2 pins (2, 4, 12, 13, 14, 15, 25, 26, 27) cannot do `analogRead()` while WiFi is on.** This plan avoids that problem because the pots are on ADC1.
- **GPIO34, 35, 36, 39 are input only.** Never use them for LEDs or servos.
- **Strapping pins (2, 5, 12, 15)** are read at boot. Buttons with `INPUT_PULLUP` are safe on them. Avoid outputs that pull them to the wrong level at boot.
- **GPIO1 and GPIO3** are used by USB serial. Avoid them if you use `Serial` or upload code.
- **Never feed more than 3.3V** into any GPIO.
- **Always share one common GND** between the ESP32, servos, pots and any external supply.
