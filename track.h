#ifndef CGAJAM_TRACK_H
#define CGAJAM_TRACK_H

#define MAX_TRACK_POINTS (100)
#define MAX_COLLIDE_SEGS (3000)

#include "raylib.h"

struct TrackPoint
{
	Vector3 pos;	
};

struct CollisionSegment 
{
	Vector2 a;
	Vector2 b;	

	// DBG
	float cooldown;

};

class Track
{
public:
	TrackPoint point[MAX_TRACK_POINTS];
	int nTrackPoints;

	CollisionSegment collideSeg[MAX_COLLIDE_SEGS];
	int nCollideSeg;

	Track();

	void genRandom();

	void addTrackPoint( float x, float z );
	void addTrackPoint( TrackPoint tp );

	void addCollideSeg( Vector3 a, Vector3 b );

	void drawTrack( Shader &shader );

	void drawTrackEditMode();
	void drawCollideSegs();

	bool checkCollide( Vector3 pA, Vector3 pB, Vector3 *isectPos, Vector3 *isectNorm );
	bool checkCollideSeg( Vector2 p1, Vector2 p2, CollisionSegment *seg, Vector3 *isectPos );

	void buildTrackMesh();
	
	float trackParametricLength();
	
	//float trackWorldLength ();


	Vector3 evalTrackCurve( float pval );

	inline Vector3 evalCatmullSplineValue(const Vector3 & a, 
											 const Vector3 & b, 
											 const Vector3 & c, 
											 const Vector3 & d, 
											 const float time);

	Mesh trackMesh;
	Model trackModel;
	bool meshBuilt;
};

#endif