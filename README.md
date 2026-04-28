

![AirSense_Reloaded](https://raw.githubusercontent.com/smartboxchannel/AirSense_Reloaded/refs/heads/main/IMAGES/Bez-nazvania.jpg)

# DIYRUZ AirSens Reloaded

Zigbee 3.0 air quality monitoring sensor based on SenseAir S8 (CO2), BME280 (temperature, humidity, pressure) and optionally ZE08K-CH2O (formaldehyde).

Support the author: https://yoomoney.ru/fundraise/P2XLTgJsB6k.231012

**This project is a fork/rework** of the [original AirSense](https://github.com/diyruz/AirSense)

---

## Table of Contents

1. [Differences from the original AirSense](#differences-from-the-original-airsense)
2. [Sensors](#sensors)
3. [Technical specifications](#technical-specifications)
4. [Schematic](#schematic)
5. [Enclosure](#enclosure)
6. [Connecting to the Zigbee network](#connecting-to-the-zigbee-network)
7. [Supported platforms](#supported-platforms)
8. [Device identification](#device-identification)
9. [Warning](#warning)

---

## Differences from the original AirSense

| Aspect | Original AirSense | AirSens Reloaded |
|--------|-------------------|------------------|
| PCB | Original | Redesigned layout |
| Enclosure | DIY / custom | Factory-made (AP07B-2) |
| SenseAir S8 polling | Unstable | Stable |
| Pressure compensation for CO2 calculation | No | Yes |
| RGB control | Discrete | PWM |
| T/H error due to self-heating | Present | Eliminated |
| Formaldehyde sensor | No | Optional (ZE08K-CH2O) |
| Device identification | No | Yes |

Backward compatibility with old hardware is preserved.

---

## Sensors

| Sensor | Measured values | Interface | Documentation |
|--------|----------------|-----------|---------------|
| SenseAir S8 | CO2 (400-5000 ppm) | UART | [PSP107.pdf](DOC/PSP107.pdf) |
| Bosch BME280 | Temperature, humidity, pressure | I2C | [bst-bme280-ds002.pdf](DOC/bst-bme280-ds002.pdf) |
| Winsen ZE08K-CH2O | Formaldehyde (HCHO) | DAC | [ze08k-ch2o.pdf](DOC/ze08k-ch2o.pdf) |

---

## Technical specifications

| Parameter | Value |
|-----------|-------|
| Model | DIYRUZ AirSens Reloaded |
| Protocol | Zigbee 3.0 |
| Radio module | EBYTE E18-MS1PA1-IPEX (20 dBm) |
| Indicator | RGB LED 5050 (PWM controlled) |
| Enclosure dimensions | ⌀ 9 cm × 2 cm |
| Power supply | USB Type C (supports fast charging protocols) |

---

## Schematic

![Schematic](IMAGES/14.png)

---

## Enclosure

Compatible enclosure: **AP07B-2**

| AP07B-2 | PCB |
|---------|-----|
| ![AP07B-2](IMAGES/1763651856367810.png) | ![PCB](IMAGES/image(1).png) |

Formaldehyde sensor module mounting — using M2 standoffs.

| Formaldehyde sensor installation |
|----------------------------------|
| ![Installation](IMAGES/Bez-imeni-6.png) |
| ![Installation](IMAGES/Bez-imeni-66.png) |

---

## Connecting to the Zigbee network

### Joining the network
1. Enable pairing mode on your Zigbee coordinator
2. Press and hold the button on the sensor (next to the USB port)
3. Wait for the system LED to turn on
4. The LED will turn off after 5-7 seconds — an open network has been found
5. If the LED turns off after 15 seconds — retry the procedure

### Factory reset and leaving the network
- Press and hold the button for **10 seconds**
- The system LED will start blinking at 1 Hz
- Once the blinking stops — release the button
- All settings stored in the device memory will be erased

Alternative method: delete the device through your coordinator's interface.

### Troubleshooting pairing issues
During pairing, place the device within 1-3 meters of the coordinator or a router with good signal strength.

---

## Supported platforms

- Zigbee2MQTT
- ZHA (Home Assistant)
- Sprut Hub
- HOMEd

| Platform | Screenshot |
|----------|------------|
| Zigbee2MQTT | ![Zigbee2MQTT](IMAGES/image(2).png) |
| ZHA | ![ZHA](IMAGES/image(4).png) |
| Sprut Hub | ![Sprut Hub](IMAGES/image(5).png) |
| HOMEd | ![HOMEd](IMAGES/image(6).png) |

---

## Device identification

![Identification](IMAGES/doc-2026-04-28-18-04-07.gif)

---

## Warning

> **This sensor is NOT intended for use in safety systems.**
>
> Do not use it:
> - in emergency ventilation systems
> - in fire alarm systems
> - for measuring absolute toxic concentrations
>
> **Intended use:** air quality monitoring for non-critical applications.

---

## Links

- [GitHub project](https://github.com/smartboxchannel/AirSense_Reloaded)
- [DIY DEV Telegram group](https://t.me/diy_devices)
- [DIY Flea market](https://t.me/diydevmart)
- [EFEKTA device catalog](http://efektalab.com/Efekta_devices)

