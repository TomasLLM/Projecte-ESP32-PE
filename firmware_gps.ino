#include <WiFi.h>
#include <HTTPClient.h>
#include "gps_data.h"

bool gpsDataIsPlausible(const GpsData &d);

// ---------- CONFIG WIFI ----------
const char* WIFI_SSID = "EL_TEU_SSID";
const char* WIFI_PASS = "EL_TEU_PASSWORD";

// ---------- CONFIG SENTILO ----------
const char* SENTILO_API_BASE = "http://147.83.83.21:8081";

// Identificador del vostre provider
const char* PROVIDER_ID = "gps_tracker_groupXY"; //MODIFICAR
const char* SPEED_SENSOR_ID   = "gps_speed"; //MODIFICAR
const char* POSITION_SENSOR_ID = "gps_position"; //MODIFICAR
const char* ALTITUDE_SENSOR_ID    = "gps_altitude";   //MODIFICAR

// Token del provider (IDENTITY_KEY a Sentilo)
const char* IDENTITY_KEY = "EL_TEU_TOKEN"; //MODIFICAR

const unsigned long SEND_PERIOD_MS = 2000; //2s
unsigned long lastSendMillis = 0;

void connectWifi();
bool sendSpeedToSentilo(float speed);
bool sendPositionToSentilo(float lat, float lon);
bool sendAltitudeToSentilo(float alt);

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
	String url;
	int httpCode;
	HTTPClient http;

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
				   "/" + SPEED_SENSOR_ID +
				   "/" + String(speed, 2);
	Serial.print("URL speed: ");
	Serial.println(url);
	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);
	httpCode = http.PUT("");
	Serial.print("Resposta HTTP speed: ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}

// Funció que envia la posició com una observació amb JSON
bool sendPositionToSentilo(float lat, float lon) {
	HTTPClient http;
	String url;
	String body;
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
				   "/" + POSITION_SENSOR_ID;
	Serial.print("URL position: ");
	Serial.println(url);
	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);
	http.addHeader("Content-Type", "application/json");
	// De moment posem un value dummy, el que importa és la location
	body = "{ \"observations\": [{"
					"\"value\": \"1\", "
					"\"location\": \"" + String(lat, 6) + " " + String(lon, 6) + "\""
					"}] }";
	Serial.print("Body position: ");
	Serial.println(body);
	httpCode = http.PUT(body);
	Serial.print("Resposta HTTP position: ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}


// Funció que envia l'altitud com un valor simple
bool sendAltitudeToSentilo(float alt) {
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
		"/" + ALTITUDE_SENSOR_ID +
		"/" + String(alt, 2);
	Serial.print("URL altitude: ");
	Serial.println(url);
	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);
	httpCode = http.PUT("");
	Serial.print("Resposta HTTP altitude: ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}

GpsData getGpsData() {
	GpsData d;

	d.lat	= 41.3890f;
	d.lon	= 2.1590f;
	d.alt	= 50.0f;	// metres
	d.speed	= 12.3f;	// km/h, per exemple
	d.valid	= true;		// en la versió real vindrà de si el GPS té fix, etc.

	return d;
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