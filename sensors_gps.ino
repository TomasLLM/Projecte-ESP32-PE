#include <Arduino.h>
#include "gps_data.h"

// ---------------------------------------------------------------------------
// CONFIGURACIÓ SERIAL DEL GPS
// ---------------------------------------------------------------------------

HardwareSerial gpsSerial(1);   // UART1: GPIO 16 (RX), GPIO 17 (TX)

String nmeaBuffer = "";
GpsData currentData;           // Estructura global amb l’última lectura vàlida


// ---------------------------------------------------------------------------
// EINA PER DIVIDIR CADENES
// ---------------------------------------------------------------------------

Vector<String> split(String s, char delimiter) {
    Vector<String> result;
    int start = 0;
    for (int i = 0; i < s.length(); i++) {
        if (s[i] == delimiter) {
            result.push_back(s.substring(start, i));
            start = i + 1;
        }
    }
    result.push_back(s.substring(start));
    return result;
}


// ---------------------------------------------------------------------------
// CONVERSIÓ DE COORDENADES NMEA A DECIMAL
// Ex: "4807.038", "N" → 48.1173
// ---------------------------------------------------------------------------

float convertToDecimal(String value, String direction) {
    if (value.length() < 3) return 0.0;

    float raw = value.toFloat();
    int deg = int(raw / 100);
    float minutes = raw - deg * 100;

    float dec = deg + minutes / 60.0;
    if (direction == "S" || direction == "W") dec = -dec;

    return dec;
}


// ---------------------------------------------------------------------------
// PARSEIG DE GGA: LAT, LON, ALT
// ---------------------------------------------------------------------------

void parseGGA(const String &s) {
    auto parts = split(s, ',');
    if (parts.size() < 10) return;

    currentData.lat = convertToDecimal(parts[2], parts[3]);
    currentData.lon = convertToDecimal(parts[4], parts[5]);
    currentData.alt = parts[9].toFloat();
}


// ---------------------------------------------------------------------------
// PARSEIG DE RMC: VALIDACIÓ + VELOCITAT (knots)
// ---------------------------------------------------------------------------

void parseRMC(const String &s) {
    auto parts = split(s, ',');
    if (parts.size() < 8) return;

    currentData.valid = (parts[2] == "A");
    currentData.speed = parts[7].toFloat();  // en nusos
}


// ---------------------------------------------------------------------------
// GESTOR DE FRASES NMEA
// ---------------------------------------------------------------------------

void processNMEASentence(const String &s) {
    if (s.startsWith("$GPGGA")) {
        parseGGA(s);
    } else if (s.startsWith("$GPRMC")) {
        parseRMC(s);
    }
}


// ---------------------------------------------------------------------------
// SETUP
// ---------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

    Serial.println("Sensor GPS inicialitzat");
    currentData = {0, 0, 0, 0, false};
}


// ---------------------------------------------------------------------------
// LOOP PRINCIPAL: LLEGIR NMEA I PROCESSAR-LES
// ---------------------------------------------------------------------------

void loop() {
    while (gpsSerial.available()) {
        char c = gpsSerial.read();

        if (c == '\n') {
            processNMEASentence(nmeaBuffer);
            nmeaBuffer = "";
        } else if (c != '\r') {
            nmeaBuffer += c;
        }
    }
}


// ---------------------------------------------------------------------------
// FUNCIÓ EXPOSADA AL FIRMWARE: RETORNA LES DADES DEL GPS
// ---------------------------------------------------------------------------

GpsData getGpsData() {
    return currentData;
}
