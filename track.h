#ifndef CGAJAM_TRACK_H
#define CGAJAM_TRACK_H

#define MAX_TRACK_POINTS (100)

#include "raylib.h"

struct TrackPoint
{
	Vector3 pos;	
};

class Track
{
public:
	TrackPoint point[MAX_TRACK_POINTS];
	int nTrackPoints;
	Track();

	void genRandom();
	void addTrackPoint( TrackPoint tp );

	void drawTrackEditMode();

	float trackParametricLength();
	
	//float trackWorldLength ();


	Vector3 evalTrackCurve( float pval );

	inline Vector3 evalCatmullSplineValue(const Vector3 & a, 
											 const Vector3 & b, 
											 const Vector3 & c, 
											 const Vector3 & d, 
											 const float time);
};

#endif