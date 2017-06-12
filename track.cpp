#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"
#include "track.h"
#include "util.h"

Track::Track() :
	nTrackPoints(0)
{

}

void Track::genRandom()
{
	nTrackPoints = 0;

	static bool usePreset = true;

	if (usePreset) {
		addTrackPoint( 0.072024, 0.603336 );
	    addTrackPoint( 1.048974, -46.208435 );
	    addTrackPoint( -102.838829, -135.614182 );
	    addTrackPoint( -55.275116, -187.607513 );
	    addTrackPoint( 76.841240, 15.792542 );
	    addTrackPoint( 164.402313, -202.469330 );
	    addTrackPoint( 190.208954, -64.451317 );
	    addTrackPoint( 143.463242, -48.453033 );
	    addTrackPoint( 72.900795, 137.979340 );
	    addTrackPoint( -165.415070, 100.499458 );
	} else {

		// Generate random track for editing
		for (int i=0; i < 10; i++) {
			TrackPoint tp;
			tp.pos.x = RandUniformRange( -200.0f, 200.0f );
			tp.pos.y = 0.0f;
			tp.pos.z = RandUniformRange( -200.0f, 200.0f );
			addTrackPoint( tp );
		}	
	}	
}

void Track::addTrackPoint( TrackPoint tp )
{
	assert( nTrackPoints < MAX_TRACK_POINTS );
	point[nTrackPoints++] = tp;
}

void Track::addTrackPoint( float x, float z )
{
	TrackPoint tp;
	tp.pos = Vector3Make( x, 0.0, z );
	addTrackPoint( tp );
}

void Track::drawTrackEditMode()
{
	for (int i=0; i < nTrackPoints; i++) {

		Color col =  (Color)LIME;
		if (i==0) {
			col = (Color)SKYBLUE;
		} else if (i==1) {
			col = (Color)ORANGE;
		}

		DrawCube( point[i].pos, 3.0f, 3.0f, 3.0f, col );
		DrawLine3D( point[i].pos, point[(i+1)%nTrackPoints].pos,
				 (Color)MAGENTA );

	}


	float trackLen = trackParametricLength();
	float t = 0.0;	
	Vector3 lastP = {0};
	while ( t < trackLen) {

		Vector3 p = evalTrackCurve( t );
		if ( t > 0.0) {
			DrawLine3D( lastP, p, (Color)YELLOW );
		}
		lastP = p;
		t += 0.01f;
				
	}
	
}

void Track::buildTrackMesh()
{
  	
}

float Track::trackParametricLength()
{
	return (float)nTrackPoints;
}

Vector3 Track::evalTrackCurve( float pval )
{
	float fp = floor(pval);
	int pndx = (int)fp;
	int ndxA = (pndx-1) % nTrackPoints;
	int ndxB = (pndx+0) % nTrackPoints;
	int ndxC = (pndx+1) % nTrackPoints;
	int ndxD = (pndx+2) % nTrackPoints;

	//printf("pval %3.2f : A %d B %d C %d D %d\n", pval,
	//			ndxA, ndxB, ndxC, ndxD ); 

	//float t = 0.25 + ((pval - fp) * 0.5);
	float t = pval - fp;
	return evalCatmullSplineValue( 
		point[ndxA].pos,
		point[ndxB].pos,
		point[ndxC].pos,
		point[ndxD].pos,
		t );
}


inline Vector3 Track::evalCatmullSplineValue(const Vector3 & a, 
											 const Vector3 & b, 
											 const Vector3 & c, 
											 const Vector3 & d, 
											 const float time)
{
	float time2 = time * time;
	float time3 = time * time2;

	//T a5 = a * static_cast<S>(-0.5);
	Vector3 a5 = Vector3MultScalar( a, -0.5 );

	//T d5 = d * static_cast<S>(0.5);
	Vector3 d5 = Vector3MultScalar( d, 0.5 );

	Vector3 p1 = a5;
	Vector3 p2 = Vector3MultScalar( b, 1.5 );
	Vector3 p3 = Vector3MultScalar( c, -1.5 );
	Vector3 p4 = d5;

	Vector3 t3 = Vector3MultScalar( VectorAdd( VectorAdd( p1, p2 ), VectorAdd( p3, p4) ), time3 );

	p1 = a;
	p2 = Vector3MultScalar( b, -2.5 );
	p3 = Vector3MultScalar( c, 2.0 );
	p4 = Vector3MultScalar( d5, -1.0 );

	Vector3 t2 = Vector3MultScalar( VectorAdd( VectorAdd( p1, p2 ), VectorAdd( p3, p4) ), time2 );

	p1 = a5;
	p3 = Vector3MultScalar( c, 0.5 );

	Vector3 t1 = Vector3MultScalar( VectorAdd( p1, p3 ), time );


	return VectorAdd( VectorAdd( t1, VectorAdd( t2, t3 ) ), b );

	//return static_cast<T>(time3 * (a5 + 1.5*b - 1.5*c + d5) +
	//                      time2 * (a - 2.5*b + 2*c - d5) + 
	//                      time * (a5 + 0.5*c) + 
	//						        b);
}











