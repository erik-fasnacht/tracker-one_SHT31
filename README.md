# Tracker One Temperature & Humidity Monitor

**Author:** Erik Fasnacht

**Date:** April 14, 2026

**Device OS:** 6.4.0 or later

A reference design built on Tracker Edge v21 that integrates an SHT31 temperature and humidity sensor on the Tracker One's M8 connector, with runtime-configurable alert thresholds managed via Particle's environment variables — no firmware reflash required.

## Table of Contents
- [Requirements](#requirements)
- [Hardware Requirements](#hardware-requirements)
- [Key Features](#key-features)
- [Environment Variables](#environment-variables)
- [Getting Started](#getting-started)
- [Data Published](#data-published)
- [Extending via Cloud API](#extending-via-cloud-api)
- [Additional Resources](#additional-resources)
- [License](#license)
- [Support](#support)

---

## Requirements

> **⚠️ Device OS 6.4.0 or later is required.** Environment variables are not available in earlier Device OS versions. Verify your device's firmware version before flashing.

> **⚠️ Tracker Edge v21 or later is required.** This application is built on the Tracker Edge reference firmware. The Tracker Edge libraries and configuration framework must be present for the firmware to compile and operate correctly.

---

## Hardware Requirements

| Component | Details |
|---|---|
| [Tracker One](https://store.particle.io/products/tracker-one) | Particle asset tracking device |
| SHT31 sensor breakout | Compatible with any SHT3x sensor at I2C address `0x44` |
| M8 4-pin connector cable | Provides 5V power and I2C (SDA/SCL) from the M8 port |

The SHT31 is wired to I2C bus 3 (Wire3) on the M8 connector. The firmware enables 5V on the M8 connector automatically at startup via `CAN_PWR`.

---

## Key Features

✅ **Sensor integration**
SHT31 temperature and humidity readings appended to every Tracker location publish as `sh31_temp` and `sh31_humid`

✅ **Alert events**
Publishes `high temp event` to the Particle Cloud when the high threshold is crossed; no event published during normal operation

✅ **Runtime configuration**
`TEMP_HIGH`, `TIME_ZONE`, and `TEMP_PERIOD` controlled from the Particle Console without reflashing — changes take effect on the next temperature check cycle

✅ **Safe defaults**
Firmware boots with a sensible threshold (86 °F) and loads Console values as soon as they are available; previously cached values are used if the device is offline at boot

✅ **Periodic monitoring**
Temperature checked on a configurable interval (default 60 seconds), independent of the location publish cadence

---

## Environment Variables

Environment variables are lightweight name-value pairs configured in the [Particle Console](https://console.particle.io) that are pushed to devices over the air. They allow runtime tuning of firmware behavior without reflashing — ideal for managing fleets where thresholds may vary by deployment location or use case.

> **⚠️ Requires Device OS 6.4.0 or later.**

| Variable | Description | Default | Unit |
|---|---|---|---|
| `TEMP_HIGH` | High temperature alert threshold | `86.0` | °F |
| `TIME_ZONE` | UTC offset for local time in location publishes | `-7.0` | hours |
| `TEMP_PERIOD` | Time between temperature checks | `60` | seconds |

### Setting Variables in the Console

1. Log in to [console.particle.io](https://console.particle.io) and navigate to your product
2. Go to **Configuration → Environment**
3. Enter the variable name and value, then click **Next** to roll out the change
4. Devices receive the update over cellular and apply it on the next check cycle — no reboot or reflash needed

If a variable is not set in the Console, the firmware default is used. If the device is offline at boot, previously cached values from the last successful cloud sync are applied.

### Further Reading

- [Getting started with environment variables](https://docs.particle.io/getting-started/configuration/environment/)
- [Device OS API reference — System.getEnv()](https://docs.particle.io/reference/device-os/api/system-calls/enviroment-variables-system/)

---

## Getting Started

### Prerequisites

- [Tracker One](https://store.particle.io/products/tracker-one)
- Particle account (sign up at [particle.io](https://www.particle.io))
- Device OS 6.4.0 or later
- [Particle Workbench](https://docs.particle.io/getting-started/developer-tools/workbench/) (VS Code) or Particle CLI

### Installation

1. **Clone this repository:**
   ```bash
   git clone <repository-url>
   cd tracker-one_temp-humidity
   git submodule update --init --recursive
   ```

2. **Open in Particle Workbench:**
   - Open the folder in VS Code
   - Run `Particle: Import Project`
   - Run `Particle: Configure Workspace for Device`, select Device OS **6.4.0+** and the `tracker` platform

3. **Compile and flash:**

   **Using Particle Workbench:**
   - Click `Particle: Cloud Flash` or `Particle: Local Flash`

   **Using Particle CLI:**
   ```bash
   particle compile tracker --target 6.4.0
   particle flash <device-name> firmware.bin
   ```

4. **Monitor operation:**
   ```bash
   particle serial monitor --follow
   ```

---

## Data Published

### Location Publish

On every location fix, the Tracker Edge framework publishes a `loc` event. This firmware appends two additional fields to that payload:

```json
{
  "sh31_temp": 72.41,
  "sh31_humid": 58.3,
  "local_time": "12:27:01 PM"
}
```

| Field | Type | Description |
|---|---|---|
| `sh31_temp` | float | Temperature in °F |
| `sh31_humid` | float | Relative humidity in % |
| `local_time` | string | Local time at publish, formatted HH:MM:SS AM/PM |

### Alert Events

When a temperature reading crosses a threshold, a separate cloud event is published:

| Event Name | Trigger | Payload |
|---|---|---|
| `high temp event` | `temp > TEMP_HIGH` | `{"temperature": 98.6}` |

No alert event is published while temperature is within the normal range — those readings are captured by the location publish.

---

## Extending via Cloud API

Environment variables can be managed programmatically using the [Particle Cloud REST API](https://docs.particle.io/reference/cloud-apis/api/#environment), making it straightforward to integrate threshold management into external systems.

Use cases include:

- **Fleet management** — push different thresholds to different device groups or individual devices
- **Integrations** — update `TEMP_HIGH` or `TEMP_LOW` automatically from an external system based on season, location, or operational mode
- **Automation** — trigger threshold changes in response to alerts using Logic Functions or webhooks

The API supports listing current environment variables, creating or updating values, and rolling out changes to devices — all without touching firmware.

---

## Additional Resources

- **Particle Tracker Edge Firmware:** https://docs.particle.io/firmware/tracker-edge/tracker-edge-firmware/
- **Environment Variables — Getting Started:** https://docs.particle.io/getting-started/configuration/environment/
- **Environment Variables — Device OS API:** https://docs.particle.io/reference/device-os/api/system-calls/enviroment-variables-system/
- **Environment Variables — Cloud API:** https://docs.particle.io/reference/cloud-apis/api/#environment
- **Particle Community:** https://community.particle.io

---

## License

Unless stated elsewhere, file headers or otherwise, all files herein are licensed under an Apache License, Version 2.0. For more information, please read the [LICENSE](LICENSE) file.

If you have questions about software licensing, please contact Particle [support](https://support.particle.io/).

---

## Support

For support or feedback:
- **Particle Community:** https://community.particle.io/c/tracking-system
- **Particle Support:** https://support.particle.io/
