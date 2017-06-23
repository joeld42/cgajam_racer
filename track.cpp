#include <assert.h>
#include <stdio.h>
#include <math.h>

#include "raylib.h"
#include "track.h"
#include "util.h"

#define SEG_COOLDOWN_TIME (1.5)

Track::Track() :
	nTrackPoints(0),
	meshBuilt(false)
{

}

void Track::genRandom()
{
	nTrackPoints = 0;
	nCollideSeg = 0;

	static bool usePreset = true;

	if (usePreset) {
		// addTrackPoint( 0.072024, 0.603336 );
	 //    addTrackPoint( 1.048974, -46.208435 );
	 //    addTrackPoint( -102.838829, -135.614182 );
	 //    addTrackPoint( -55.275116, -187.607513 );
	 //    addTrackPoint( 76.841240, 15.792542 );
	 //    addTrackPoint( 164.402313, -202.469330 );
	 //    addTrackPoint( 190.208954, -64.451317 );
	 //    addTrackPoint( 143.463242, -48.453033 );
	 //    addTrackPoint( 72.900795, 137.979340 );
	 //    addTrackPoint( -165.415070, 100.499458 );

	addTrackPoint( -42.733921, 7.118019 );
    addTrackPoint( -24.298538, -36.245270 );
    addTrackPoint( -102.838829, -135.614182 );
    addTrackPoint( -55.275116, -187.607513 );
    addTrackPoint( 58.704330, -13.472721 );
    addTrackPoint( 164.402313, -202.469330 );
    addTrackPoint( 212.666992, -19.637169 );
    addTrackPoint( 159.511002, -29.298012 );
    addTrackPoint( 45.775219, 149.611786 );
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

void Track::addCollideSeg( Vector3 a, Vector3 b )
{
	CollisionSegment *seg = collideSeg + nCollideSeg++;
	seg->a = Vector2Make( a.x, a.z );
	seg->b = Vector2Make( b.x, b.z );
	seg->cooldown = 0.0;
}

bool Track::checkCollide( Vector3 pA, Vector3 pB, Vector3 *isectPos, Vector3 *isectNorm )
{
	// TODO: Bucket these or something..
	Vector2 pA2 = Vector2Make( pA.x, pA.z );
	Vector2 pB2 = Vector2Make( pB.x, pB.z );

	for (int i=0; i < nCollideSeg; i++) {
		CollisionSegment *seg = collideSeg + i;
		if (checkCollideSeg( pA2, pB2, seg, isectPos )) {
			seg->cooldown = SEG_COOLDOWN_TIME;

			if (isectNorm) {
				Vector3 wallVec = Vector3Make( seg->b.x - seg->a.x, 0.0, seg->b.y - seg->a.y );
				VectorNormalize( &wallVec );
				*isectNorm = VectorCrossProduct( wallVec, (Vector3){ 0.0, -1.0, 0.0} );
				VectorNormalize( isectNorm );
			}

			return true;
		}
	}
	return false;
}

bool Track::checkCollideSeg( Vector2 p1, Vector2 p2, CollisionSegment *seg, Vector3 *isectPos )
{
	float Ax,Bx,Cx,Ay,By,Cy,d,e,f,num/*,offset*/;
	float x1lo,x1hi,y1lo,y1hi;

	Vector2 p3 = seg->a;
	Vector2 p4 = seg->b;

	Ax = p2.x-p1.x;
	Bx = p3.x-p4.x;

	// X bound box test/
	if(Ax<0) {
		x1lo=p2.x; x1hi=p1.x;
	} else {
		x1hi=p2.x; x1lo=p1.x;		
	}
	if(Bx>0) {
		if(x1hi < p4.x || p3.x < x1lo) return false;
	} else {
		if(x1hi < p3.x || p4.x < x1lo) return false;
	}

	Ay = p2.y-p1.y;
	By = p3.y-p4.y;

	// Y bound box test//
	if(Ay<0) {                  
		y1lo=p2.y; y1hi=p1.y;
	} else {
		y1hi=p2.y; y1lo=p1.y;
	}

	if(By>0) {
		if(y1hi < p4.y || p3.y < y1lo) return false;
	} else {
		if(y1hi < p3.y || p4.y < y1lo) return false;
	}

	Cx = p1.x-p3.x;
	Cy = p1.y-p3.y;
	d = By*Cx - Bx*Cy;  // alpha numerator//
	f = Ay*Bx - Ax*By;  // both denominator//

	// alpha tests//
	if(f>0) {
		if(d<0 || d>f) return false;
	} else {
		if(d>0 || d<f) return false;
	}

	e = Ax*Cy - Ay*Cx;  // beta numerator//
	// beta tests //
	if(f>0) {                          
		if(e<0 || e>f) return false;
	} else {
		if(e>0 || e<f) return false;
	}

	// check if they are parallel
	if(f==0) return false;

	// compute intersection coordinates //
	num = d*Ax; // numerator //
	num = d*Ay;

	if (isectPos) {
		isectPos->x = p1.x + num / f;
		isectPos->y = p1.y + num / f;
	}
	
	return true;

}

void Track::drawTrack( Shader &shader )
{
	if (meshBuilt) {
		trackModel.material.shader = shader;
		DrawModel( trackModel, (Vector3){ 0.0, 0.0, 0.0}, 1.0, (Color)WHITE );
	}
}

void Track::drawCollideSegs()
{
	for (int i=0; i < nCollideSeg; i++) {
		CollisionSegment *seg = collideSeg + i;
		Vector3 a = Vector3Make( seg->a.x, 0.5, seg->a.y );
		Vector3 b = Vector3Make( seg->b.x, 0.5, seg->b.y );

		//DrawLine3D( a, b, ColorLerp( (Color)GOLD, (Color)BLUE, seg->cooldown ) );
		DrawLine3D( a, b, ColorLerp( (Color)GOLD, (Color)BLUE, (seg->cooldown) / SEG_COOLDOWN_TIME ) );

		if (seg->cooldown > 0.0) {
			seg->cooldown = saturatef( seg->cooldown - 1.5*(1.0/60.0) );
		}
	}
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
			DrawLine3D( lastP, p, (Color)ORANGE );
		}
		lastP = p;
		t += 0.01f;
				
	}
	
}

void Track::buildTrackMesh()
{
	printf("Build Track Mesh ------------------------------\n" );
  	//Mesh LoadMeshEx(int numVertex, float *vData, float *vtData, float *vnData, Color *cData);
    //Model LoadModelFromMesh(Mesh data, bool dynamic);                                       

    nCollideSeg = 0;

	int nPoints = 1500;
    Vector3 *vert = (Vector3*)malloc( sizeof(Vector3) * nPoints * 6 );
    Vector3 *nrm = (Vector3*)malloc( sizeof(Vector3) * nPoints * 6 );
    Vector2 *st = (Vector2*)malloc( sizeof(Vector2) * nPoints * 6 );    

    // FIXME: adaptive segments
	float totalLen = trackParametricLength();
	float step = totalLen / 1000.0f;

	float t = 0.0;
	float prevT;
	int ndx = 0;
	float texScale = -0.05;
	float trackLength = 0.0; 
	float prevTrackLength = 0.0; 
	Vector3 prevLeft, prevRight, prevP;
	while (t < totalLen) {

		Vector3 p = evalTrackCurve(  t );
		Vector3 p2 = evalTrackCurve( t + 0.001 );
		Vector3 dir = VectorSubtract( p2, p );
		VectorNormalize( &dir );

		dir = VectorCrossProduct( dir, (Vector3){ 0.0, 1.0, 0.0 } );
		VectorNormalize( &dir );

		dir = Vector3MultScalar( dir, 10.0f );
		Vector3 right = VectorAdd( p, dir );
		Vector3 left = VectorSubtract( p, dir );		

		if (t > 0.0) {

			float d = VectorDistance( prevP, p );
			trackLength += d;

			vert[ndx+0] = prevLeft;
			st[ndx+0] = Vector2Make( 0.0, prevTrackLength * texScale );			
			vert[ndx+1] = prevRight;
			st[ndx+1] = Vector2Make( 1.0, prevTrackLength * texScale);
			vert[ndx+2] = left;
			st[ndx+2] = Vector2Make( 0.0, trackLength * texScale);

			vert[ndx+5] = prevRight;
			st[ndx+5] = Vector2Make( 1.0, prevTrackLength * texScale);
			vert[ndx+4] = left;
			st[ndx+4] = Vector2Make( 0.0, trackLength * texScale);
			vert[ndx+3] = right;
			st[ndx+3] = Vector2Make( 1.0, trackLength * texScale );

			addCollideSeg( prevLeft, left );
			addCollideSeg( prevRight, right );

			for (int j=0; j < 6; j++) {
				nrm[ndx+j] = (Vector3){0.0, 1.0, 0.0};
			}

			ndx += 6;
		}
		prevP = p;
		prevLeft = left;
		prevRight = right;
		prevT = t;
		prevTrackLength = trackLength;

		t += step;
	}

	trackMesh = LoadMeshEx(ndx, (float*)vert, (float*)st, (float*)nrm, NULL /*Color *cData*/ );
	trackModel = LoadModelFromMesh( trackMesh, false );

	Texture2D trackTexture = LoadTexture("track1.png");
    trackModel.material.texDiffuse = trackTexture; 

	meshBuilt = true;

	free(vert);
	free(st);
}

float Track::trackParametricLength()
{
	return (float)nTrackPoints;
}

Vector3 Track::evalTrackCurve( float pval )
{
	float fp = floor(pval);
	int pndx = (int)fp;
	int ndxA = (pndx+nTrackPoints-1) % nTrackPoints;
	int ndxB = (pndx+0) % nTrackPoints;
	int ndxC = (pndx+1) % nTrackPoints;
	int ndxD = (pndx+2) % nTrackPoints;

	// printf("pval %3.2f : A %d B %d C %d D %d\n", pval,
	// 			ndxA, ndxB, ndxC, ndxD ); 

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











