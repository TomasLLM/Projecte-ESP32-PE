#include <WiFi.h>
#include <HTTPClient.h>
#include "gps_data.h"

bool gpsDataIsPlausible(const GpsData &d);
bool sendNumericToSentilo(const char* sensorId, float value, const char* labelForLog);

// ---------- CONFIG WIFI ----------
const char* WIFI_SSID = "POCO F3";
const char* WIFI_PASS = "ProjectesE";

// ---------- CONFIG SENTILO ----------
const char* SENTILO_API_BASE = "http://147.83.83.21:8081";

// Identificador del vostre provider
const char* PROVIDER_ID				= "grup_4-101@grup4_101_provider";
const char* SPEED_SENSOR_ID     = "Velocitat";
const char* LAT_SENSOR_ID       = "Latitud";
const char* LON_SENSOR_ID       = "Longitud";
const char* ALTITUDE_SENSOR_ID  = "Altitud";

// Token del provider (IDENTITY_KEY a Sentilo)
const char* IDENTITY_KEY = "83af41529fb19b634c61ca72379ccdf7dd58cb120fc75d806cabaa36565438dc";

const unsigned long SEND_PERIOD_MS = 2000; //2s
unsigned long lastSendMillis = 0;

void connectWifi();
bool sendSpeedToSentilo(float speed);
bool sendPositionToSentilo(float lat, float lon);
bool sendAltitudeToSentilo(float alt);

GpsData getGpsData() {
	GpsData d;

	d.lat	= 41.3890f;
	d.lon	= 2.1590f;
	d.alt	= 50.0f;	// metres
	d.speed	= 12.3f;	// km/h, per exemple
	d.valid	= true;		// en la versió real vindrà de si el GPS té fix, etc.

	return d;
}

void setup() {
	Serial.begin(115200);
	delay(1000);
	Serial.println("\nInicialitzant firmware GPS + Sentilo...");
	connectWifi();
}

void loop() {
	unsigned long now = millis();
	bool okSpeed;
	bool okPos;	
	bool okAlt;	

	if (now - lastSendMillis >= SEND_PERIOD_MS) {
		lastSendMillis = now;
		GpsData d = getGpsData();
		if (!d.valid || !gpsDataIsPlausible(d)) {
			Serial.println("Dades GPS no valides o poc plausibles. No s'envien.");
			return;
		}
		Serial.println("Enviant dades a Sentilo...");
		okSpeed	= sendSpeedToSentilo(d.speed);
		okPos = sendPositionToSentilo(d.lat, d.lon);
		okAlt = sendAltitudeToSentilo(d.alt);

		if (okSpeed && okPos && okAlt) {
			Serial.println("   -> Enviament OK");
		} else {
			Serial.println("   -> ERROR en l'enviament (algun sensor ha fallat)");
		}
	}
}


// Funció per connectar-se al Wifi
void connectWifi() {
	int retries = 0;
	const int maxRetries = 20;

	Serial.print("Connectant a la WiFi ");
	Serial.println(WIFI_SSID);
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
		delay(500);
		Serial.print(".");
		retries++;
	}
	if (WiFi.status() == WL_CONNECTED) {
		Serial.println("\nWiFi connectat!");
		Serial.print("IP local: ");
		Serial.println(WiFi.localIP());
	} else {
		Serial.println("\nNo s'ha pogut connectar a la WiFi.");
	}
}

//Funció que envia la velocitat
bool sendSpeedToSentilo(float speed) {
	return sendNumericToSentilo(SPEED_SENSOR_ID, speed, "speed");
}


// Funció que envia la posició com una observació amb JSON
bool sendAltitudeToSentilo(float alt) {
	return sendNumericToSentilo(ALTITUDE_SENSOR_ID, alt, "altitude");
}

// Funció que envia l'altitud com un valor simple
bool sendLatitudeToSentilo(float lat) {
	return sendNumericToSentilo(LAT_SENSOR_ID, lat, "lat");
}

bool sendLongitudeToSentilo(float lon) {
	return sendNumericToSentilo(LON_SENSOR_ID, lon, "lon");
}

bool sendPositionToSentilo(float lat, float lon) {
	bool okLat = sendLatitudeToSentilo(lat);
	bool okLon = sendLongitudeToSentilo(lon);
	return (okLat && okLon);
}

bool gpsDataIsPlausible(const GpsData &d) {
	if (d.lat < -90.0f || d.lat > 90.0f) {
		return false;
	}
	if (d.lon < -180.0f || d.lon > 180.0f) {
		return false;
	}
	if (d.speed < 0.0f) {
		return false;
	}
	return true;
}

bool sendNumericToSentilo(const char* sensorId, float value, const char* labelForLog) {
	HTTPClient http;
	String url;
	int httpCode;

	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi desconnectada. Reintentant connectar...");
		connectWifi();
		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("No s'ha pogut reconnectar a la WiFi.");
			return false;
		}
	}

	url = String(SENTILO_API_BASE) +
		"/data/" + PROVIDER_ID +
		"/" + sensorId;

	// JSON: {"observations":[{"value":"<valor>"}]}
	String body = String("{\"observations\":[{\"value\":\"") +
		String(value, 6) +
		String("\"}]}");

	Serial.print("URL ");
	Serial.print(labelForLog);
	Serial.print(": ");
	Serial.println(url);

	Serial.print("Body ");
	Serial.print(labelForLog);
	Serial.print(": ");
	Serial.println(body);

	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);
	http.addHeader("Content-Type", "application/json");

	httpCode = http.PUT(body);
	Serial.print("Resposta HTTP ");
	Serial.print(labelForLog);
	Serial.print(": ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}