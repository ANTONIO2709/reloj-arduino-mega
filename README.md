# Reloj Arduino Mega con TFT, SD y Sensores

Este proyecto consiste en un reloj avanzado basado en **Arduino Mega 2560** con una pantalla **TFT LCD de 2.4"**, lector de tarjetas **SD** para fondos dinámicos y sensores meteorológicos.

## Características principales

- **Arduino Mega 2560**: Adaptación específica para usar el shield TFT de 2.4" sin cables externos.
- **SD por Software (SoftSPI)**: Solución por código para mapear los pines SPI del shield (11, 12, 13) en el Mega sin puentes físicos.
- **Fondos Dinámicos**: Carga automática de imágenes `.BMP` de 24 bits desde la tarjeta SD.
- **Sensor BMP085**: Medición de Temperatura, Presión Atmosférica y Altitud.
- **Reloj DS3231**: RTC de alta precisión para Hora, Fecha y Día de la Semana.
- **Diseño 3D**: Incluye script paramétrico de OpenSCAD para imprimir la caja.

## Estructura del Proyecto

- `reloj_sd.ino`: Código principal de Arduino.
- `images/`: Pack de fotos de ejemplo para la SD.
- `3d/`: Diseño de la caja para impresión 3D.
- `BMP085.PDF`: Documentación técnica del sensor.

## Configuración y Librerías

Para que la SD funcione en el Mega con el shield pinchado directamente, se ha aplicado un parche en la librería `SD` de Arduino (modificando `Sd2Card.h` y `Sd2Card.cpp`) para activar el modo **Software SPI**.

### Pines utilizados:
- **TFT**: Estándar del shield.
- **SD**: Pines 11 (MOSI), 12 (MISO), 13 (SCK) y 10 (CS).
- **I2C**: Pines 20 (SDA) y 21 (SCL) del Mega.

## Instalación

1. Clona este repositorio o descarga el ZIP.
2. Asegúrate de tener instaladas las librerías `Adafruit_TFTLCD`, `TouchScreen`, `RTClib` y `Adafruit_BMP085`.
3. Copia las imágenes de la carpeta `images/` a la raíz de tu tarjeta SD (formateada en FAT32).
4. Sube el código a tu Arduino Mega.

## Licencia
Dominio Público - ¡Siéntete libre de mejorarlo!
