# nRF52833 Beacon Example with Button-Controlled PHY and TX Power

This project is a customized beacon application built on the **nRF5 SDK 17.1.0** for the **nRF52833 DK (PCA10100)**. It demonstrates:

- BLE advertising as a non-connectable beacon
- Advertising a **byte stream**
- **Automatic incrementing of the last byte** of the advertised data, with **wrap-around to 0 when it overflows**
- Dynamic PHY switching (1 Mbps ↔ Coded PHY) via **Button 1**
- Dynamic TX power switching (4 dBm ↔ 8 dBm) via **Button 2**
- LED indicators for current PHY and TX power modes

## 📍 Project Location in SDK

This project should be placed in:
`nRF5_SDK_17.1.0_ddde560/examples/ble_peripheral/`

## 🧠 Features

- Non-connectable extended advertising using a byte stream
- Periodic payload updates every 2 seconds
- Uses:
  - `bsp_board_led_on()` for PHY and TX power indication
  - `app_button` for handling button presses

## 🛠️ Setup

* **SDK Version:** nRF5 SDK 17.1.0  
* **SoftDevice:** S140  
* **Toolchain:** SEGGER Embedded Studio or GCC  
  ➤ Tested with **SEGGER Embedded Studio for ARM v5.42a**  

### 📁 To run this project:

1. Place the folder under:  
   `nRF5_SDK_17.1.0_ddde560\examples\ble_peripheral\`

2. Open and run the project file `ble_app_beacon_pca10100_s140_Release.emProject` located in:  
   `pca10100\s140\ses\`

3. Compile and flash the application to the nRF52833 DK.

## 🧪 Button Functions

| Button | Function                         |
|--------|----------------------------------|
|   1    | Toggle PHY (Coded ↔ 1 Mbps)      |
|   2    | Toggle TX Power (4 dBm ↔ 8 dBm)  |

## 💡 LED Indicators

- **LED 2**: ON when using Coded PHY, OFF for 1 Mbps
- **LED 3**: ON for 4 dBm, OFF for 8 dBm

## 📜 License

Nordic Semiconductor License (from nRF5 SDK). See license terms in the `main.c` file.

## 📂 Related

- [nRF5 SDK](https://www.nordicsemi.com/Products/Development-software/nRF5-SDK)
- [nRF52833](https://www.nordicsemi.com/Products/nRF52833)


