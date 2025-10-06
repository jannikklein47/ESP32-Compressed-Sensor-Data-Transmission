Efficient Lossless Compression and Transmission of Sensor Data on Resource-Limited IoT-Systems Using the ESP32

This PlatformIO project is the ESP32 implementation for my WAB of the second semester in Algorithms & Data Structures.

It features three compression algorithms and an http sender.
The program works by collecting x amount of sensor readings, compressing them using the by the user specified compression algorithm, and sending them over the network via http to a custom endpoint.

The program is meant to be flashed onto an ESP32-WROOM-32E.
Sensor readings are collected from a to the ESP via GPIO connected MPU6050 sensor using third-party libraries.
