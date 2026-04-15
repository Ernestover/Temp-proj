
# Raspberry Pi Pico: Real-Time Environment Monitor
A high-precision C-based embedded system that interfaces a Raspberry Pi Pico with a DHT11 sensor and a 20x4 I2C LCD to provide live temperature and humidity telemetry.

# Overview
This project demonstrates low-level hardware communication using the Raspberry Pi Pico C/C++ SDK. It features a custom implementation of the DHT11 "One-Wire" protocol and I2C communication to drive a 20x4 character display via a PCF8574 GPIO expander.

## Key Features
- Custom DHT11 Driver: Implements precise microsecond-level timing to decode 40-bit sensor data.
-  I2C LCD Integration: Drives a 20x4 display using 4-bit initialization and custom command/data functions.
- Real-Time Processing: Continuous data polling with a 2-second sampling rate to ensure sensor stability.
- Error Handling: Checksum verification and hardware "GPIO OK" self-test during boot.

# Hardware Requirements 
- Microcontroller: Raspberry Pi Pico (standard)
- Sensor: DHT11 Temperature & Humidity Sensor (3-pin module)
- Display: 20x4 LCD with I2C Backpack (PCF8574)
- Environment: C/C++ SDK, CMAKE, and ARM GCC Toolchain

# Pinout Configuration

# Software Implementation 
The DHT11 uses a proprietary single-bus protocol that is highly timing-sensitive. This project handles the manual handshake:

1. Start Signal: Pulling the bus low for >18ms.

2. State Switching: Rapidly flipping GPIO direction from OUTPUT to INPUT.

3. Pulse Timing: Measuring high-pulse durations (24µs for '0', 70µs for '1') to reconstruct 5 bytes of data.

4. Checksum: Validating the data integrity using the 8-bit sum of the humidity and temperature bytes.

## Documentation Style 
https://github.com/Ernestover/Temp-proj/blob/d8f4e93dd18370ff22872c55550aaa901e07c712/main.c#L242-L255

# Setup & Build 
1. Ensure the Pico C/C++ Sdk is installed 
2. Clone this repositry and create a build directory 
3. Run Cmake:
    - mkdir build && cd build
    - cmake ..
    - make 
4. Flash the resulting .uf2 file to your Pico 


