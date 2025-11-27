// gps_data.h
#ifndef GPS_DATA_H
#define GPS_DATA_H

struct GpsData {
	float lat;
	float lon;
	float alt;
	float speed;
	bool valid;
};

GpsData getGpsData();	// La Part 1 l'ha d'implementar

#endif