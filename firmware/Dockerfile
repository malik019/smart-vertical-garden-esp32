FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV CLI_HOST=downloads.arduino.cc
ENV CLI_PATH=arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz
ENV ESP_HOST=espressif.github.io
ENV ESP_PATH=arduino-esp32/package_esp32_index.json
ENV GH_HOST=github.com
ENV ASYNCTCP_GIT=ESP32Async/AsyncTCP.git

RUN apt-get update && apt-get install -y \
    curl python3 python3-pip git unzip \
    && rm -rf /var/lib/apt/lists/*

# Installeer arduino-cli in /usr/local/bin
RUN curl -fsSL "https://${CLI_HOST}/${CLI_PATH}" \
    | tar -xz -C /usr/local/bin arduino-cli

# Configureer ESP32 board manager en installeer esp32 core + tools
RUN arduino-cli config init && \
    arduino-cli config set board_manager.additional_urls "https://${ESP_HOST}/${ESP_PATH}" && \
    arduino-cli config set library.enable_unsafe_install true && \
    arduino-cli core update-index && \
    arduino-cli core install esp32:esp32

# Installeer alle bibliotheken (met matching ESP32Async/AsyncTCP voor ESP Async WebServer 3.x)
RUN arduino-cli lib install --git-url "https://${GH_HOST}/${ASYNCTCP_GIT}" && \
    arduino-cli lib install \
    "WiFiManager" \
    "ArduinoJson" \
    "ESP Async WebServer" \
    "PubSubClient" \
    "Adafruit BMP280 Library" \
    "Adafruit Unified Sensor" \
    "OneWire" \
    "DallasTemperature"

WORKDIR /firmware
