#include <Arduino.h>
#include <HardwareSerial.h>
#include "gps_data.h"

HardwareSerial gpsSerial(1);   // UART1: GPIO 16 (RX), GPIO 17 (TX)

String nmeaBuffer = "";
GpsData currentData;

// Funció per dividir cadenes en camps
void splitNMEA(const String &s, char delimiter, String *out, int maxParts, int &outCount) {
    outCount = 0;
    int start = 0;

    for (int i = 0; i < s.length() && outCount < maxParts; i++) {
        if (s[i] == delimiter) {
            out[outCount++] = s.substring(start, i);
            start = i + 1;
        }
    }
    if (outCount < maxParts && start <= s.length()) {
        out[outCount++] = s.substring(start);
    }
}
  

static float convertToDecimal(const String &value, const String &direction) {
    if (value.length() < 3) {
        return 0.0f;
    }

    float raw = value.toFloat();
    int deg = int(raw / 100.0f);
    float minutes = raw - deg * 100.0f;

    float dec = deg + minutes / 60.0f;
    if (direction == "S" || direction == "W") {
        dec = -dec;
    }

    return dec;
}

//Funcions per fer el parsing en el format que volem
static void parseGGA(const String &s) {
    const int MAX_PARTS = 20;
    String parts[MAX_PARTS];
    int count = 0;

    splitNMEA(s, ',', parts, MAX_PARTS, count);
    if (count < 10) {
        return;
    }

    currentData.lat = convertToDecimal(parts[2], parts[3]);
    currentData.lon = convertToDecimal(parts[4], parts[5]);
    currentData.alt = parts[9].toFloat();	// metres
}

static void parseRMC(const String &s) {
    const int MAX_PARTS = 20;
    String parts[MAX_PARTS];
    int count = 0;

    splitNMEA(s, ',', parts, MAX_PARTS, count);
    if (count < 8) {
        return;
    }

    currentData.valid = (parts[2] == "A");

    float speedKnots = parts[7].toFloat();
    currentData.speed = speedKnots * 1.852f;//passem a km/h
}

static void processNMEASentence(const String &s) {
	if (s.startsWith("$GPGGA") || s.startsWith("$GNGGA")) {
		parseGGA(s);
	} else if (s.startsWith("$GPRMC") || s.startsWith("$GNRMC")) {
		parseRMC(s);
	}
}

//Iniciem gps, la func es crida desde firmware
void initGps() {
    Serial.println("Inicialitzant GPS (UART1 16/17 @ 9600)...");
    gpsSerial.begin(9600, SERIAL_8N1, 16, 17);

    currentData.lat   = 0.0f;
    currentData.lon   = 0.0f;
    currentData.alt   = 0.0f;
    currentData.speed = 0.0f;
    currentData.valid = false;
    nmeaBuffer        = "";
}

//Actualitzem GPS
void updateGps() {
  while (gpsSerial.available()) {
      char c = gpsSerial.read();
      Serial.write(c);

      if (c == '\n') {
          processNMEASentence(nmeaBuffer);
          nmeaBuffer = "";
      } else if (c != '\r') {
        nmeaBuffer += c;
      }
  }
}


GpsData getGpsData() {
    return currentData;
}