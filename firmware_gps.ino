#include <WiFi.h>
#include <HTTPClient.h>

// ---------- CONFIG WIFI ----------
const char* WIFI_SSID = "EL_TEU_SSID";
const char* WIFI_PASS = "EL_TEU_PASSWORD";

// ---------- CONFIG SENTILO ----------
const char* SENTILO_API_BASE = "http://147.83.83.21:8081";

// Identificador del vostre provider
const char* PROVIDER_ID = "gps_tracker_groupXY"; //MODIFICAR

const char* SPEED_SENSOR_ID   = "gps_speed"; //MODIFICAR
const char* POSITION_SENSOR_ID = "gps_position"; //MODIFICAR

// Token del provider (IDENTITY_KEY a Sentilo)
const char* IDENTITY_KEY = "EL_TEU_TOKEN"; //MODIFICAR

const unsigned long SEND_PERIOD_MS = 2000; //5s
unsigned long lastSendMillis = 0;

void connectWifi();
bool sendSpeedToSentilo(float speed);
bool sendPositionToSentilo(float lat, float lon);

void setup() {
	Serial.begin(115200);
	delay(1000);
	Serial.println("\nInicialitzant firmware GPS + Sentilo...");
	connectWifi();
}

void loop() {
  unsigned long now = millis();

  if (now - lastSendMillis >= SEND_PERIOD_MS) {
	lastSendMillis = now;

	//DADES SIMULADES
	float fakeSpeed = 12.3;
	float fakeLat   = 41.3890;
	float fakeLon   = 2.1590;
	//AQUI OMPLIREM UNA ESTRUCTURA QUE SERA GpsData

	Serial.println("Enviant dades a Sentilo...");

	bool ok1 = sendSpeedToSentilo(fakeSpeed);
	bool ok2 = sendPositionToSentilo(fakeLat, fakeLon);

	/*
	GpsData d = getGpsData();
	ool ok1 = sendSpeedToSentilo(d.speed);
	ool ok2 = sendPositionToSentilo(d.lat, d.lon);
	*/

	if (ok1 && ok2) {
		Serial.println("   -> Enviament OK");
	} else {
		Serial.println("   -> ERROR en l'enviament");
	}
  }
}

// Funció per connectar-se al Wifi
void connectWifi() {
	Serial.print("Connectant a la WiFi ");
	Serial.println(WIFI_SSID);

	WiFi.begin(WIFI_SSID, WIFI_PASS);

	int retries = 0;
	const int maxRetries = 20;

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
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi desconnectada. Reintentant connectar...");
		connectWifi();
		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("No s'ha pogut reconnectar a la WiFi.");
			return false;
		}
	}
	HTTPClient http;

	String url = String(SENTILO_API_BASE) +
				   "/data/" + PROVIDER_ID +
				   "/" + SPEED_SENSOR_ID +
				   "/" + String(speed, 2);

	Serial.print("URL speed: ");
	Serial.println(url);

	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);

	int httpCode = http.PUT("");

	Serial.print("Resposta HTTP speed: ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}

// Envia la posició com una observació amb JSON
bool sendPositionToSentilo(float lat, float lon) {
	if (WiFi.status() != WL_CONNECTED) {
		Serial.println("WiFi desconnectada. Reintentant connectar...");
		connectWifi();
		if (WiFi.status() != WL_CONNECTED) {
			Serial.println("No s'ha pogut reconnectar a la WiFi.");
			return false;
		}
	}
	HTTPClient http;

	String url = String(SENTILO_API_BASE) +
				   "/data/" + PROVIDER_ID +
				   "/" + POSITION_SENSOR_ID;

	Serial.print("URL position: ");
	Serial.println(url);

	http.begin(url);
	http.addHeader("IDENTITY_KEY", IDENTITY_KEY);
	http.addHeader("Content-Type", "application/json");

	// De moment posem un value dummy, el que importa és la location
	String body = "{ \"observations\": [{"
					"\"value\": \"1\", "
					"\"location\": \"" + String(lat, 6) + " " + String(lon, 6) + "\""
					"}] }";

	Serial.print("Body position: ");
	Serial.println(body);

	int httpCode = http.PUT(body);

	Serial.print("Resposta HTTP position: ");
	Serial.println(httpCode);
	http.end();
	return (httpCode == 200 || httpCode == 201);
}
