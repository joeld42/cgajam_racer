#include <raylib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "util.h"
#include "carsim.h"

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

// carsim constants
// NOTE: commented out value are orig values
const float EPS = 0.0001f;
const float CAR_B = 1.0f; // m
const float CAR_C = 1.0f; // m
const float CAR_WHEELBASE = CAR_B + CAR_C;
const float CAR_H = 1.0f; // m
//const float CAR_MASS = 5000.0f; // kg
const float CAR_MASS = 3000.0f; // kg
//const float CAR_INERTIA = 5000.0f; // kg.m
const float CAR_INERTIA = 1000.0f; // kg.m
const float CAR_WIDTH = 1.5f; // m
const float CAR_LENGTH = 3.0; // m, must be > wheelbase
const float CAR_WHEELLENGTH = 0.7f;
const float CAR_WHEELWIDTH = 0.3f;

//const float DRAG        = 5.0f;     /* factor for air resistance (drag)         */
const float DRAG        = 7.0f;     /* factor for air resistance (drag)         */
//const float RESISTANCE  = 30.0f;    /* factor for rolling resistance */
const float RESISTANCE  = 30.0f;    /* factor for rolling resistance */
// const float CA_R        = -5.20f;   /* cornering stiffness */
// const float CA_F        = -5.0f;    /* cornering stiffness */
const float CA_R        = -15.20f;   /* cornering stiffness */
const float CA_F        = -15.0f;    /* cornering stiffness */
//const float MAX_GRIP    = 2.0f;     /* maximum (normalised) friction force, =diameter of friction circle */
const float MAX_GRIP    = 20.0f;     /* maximum (normalised) friction force, =diameter of friction circle */

// const float THROTTLE = (100.0f);
const float THROTTLE = (500.0f);

const float BRAKE = (400.0f);
//const float MAX_STEERANGLE = (30.0f); // degrees
const float MAX_STEERANGLE = (20.0f); // degrees

CarModel::CarModel()
{
	_pos = Vector2Make( -2.0f, 0.0f );
	_vel = Vector2Make( 0.0f, 0.0f );
	_angle = M_PI;
	_angularvelocity = 0.0f;
	_steerangle = 0.0f;
	_throttle = 0.0f;
	_brake = 0.0f;

	_front_slip = false;
	_rear_slip = false;

	InitPhysicsGraph( &_graphSpeed, 0.0, 200.0 );

	InitPhysicsGraph( &_graphAngle, -180.0, 180.0 );
	InitPhysicsGraph( &_graphAngVel, -180.0, 180.0 );
}

void InitPhysicsGraph( PhysicsGraph *g, float minGraphVal, float maxGraphVal )
{
	memset( g, sizeof(PhysicsGraph), 0 );
	g->minVal = minGraphVal;
	g->maxVal = maxGraphVal;
}

void DrawPhysicsGraph( PhysicsGraph *g, Rectangle rect )
{
	Color bg = (Color)DARKBLUE;
	//DrawRectangleRec( rect, bg );
	DrawRectangleLines( rect.x, rect.y, rect.width, rect.height, (Color)SKYBLUE );

	float extent = g->maxVal - g->minVal;
	if (extent < 0.0001) {
		return;
	}

	float step = rect.width / (float)g->numHistory;	
	for (int i=0; i < g->numHistory-1; i++) {
		float val = 1.0 - ((g->history[i] - g->minVal) / extent);
		float val2 = 1.0 - ((g->history[i+1] - g->minVal) / extent);
		DrawLine( rect.x + (i*step), rect.y + val*rect.height,
				  rect.x + ((i+1)*step), rect.y + val2*rect.height,
				  (Color)GOLD );

	}

	char buff[200];
	sprintf(buff, "%3.2f", g->history[g->numHistory-1]);
	DrawText( buff, rect.x+2, rect.y+2, 12, (Color)WHITE );

	sprintf(buff, "%3.2f", g->maxVal);
	DrawText( buff, rect.x + rect.width - 50, rect.y+2, 12, (Color)WHITE );

	sprintf(buff, "%3.2f", g->minVal);
	DrawText( buff, rect.x + rect.width - 50, rect.y + rect.height - 15, 12, (Color)WHITE );

}

void UpdatePhysicsGraph( PhysicsGraph *g, float val )
{
	if (g->numHistory > 0) {
		g->numHistory--;
		for (int i=0; i < g->numHistory; i++) {
			g->history[i] = g->history[i+1];
		}
	}
	while (g->numHistory < (MAX_HISTORY-1) ) {
		g->history[g->numHistory++] = val;
	}
}

void CarModel::Update( float dt, float throttle, float steer, bool brake )
{
	// copy control into carsim
	_steerangle = pow(steer, 3.0f) * (MAX_STEERANGLE * (M_PI/180.0f));
	_throttle = throttle*THROTTLE;
	_brake = brake?BRAKE:0.0f;	

	// update car sim -- based on code from
	// Monsterous software and bloemschneif
	// see "Car Physics for Games.doc" for more info
#if 0
	vec2f f = ctrl->GetLeftStick();
	vec2f a = f;
	g_carSim.vel += a * dt;
	g_carSim.pos += g_carSim.vel * dt;
#endif

	float sn, cs;
	sn = sin( _angle );
	cs = cos( _angle );

	// transform vel into car's frame of ref
	Vector2 vel = Vector2Make(  cs * _vel.y + sn * _vel.x,
							    -sn * _vel.y + cs * _vel.x );

	// lateral force on wheels
	float yawspeed = CAR_WHEELBASE * 0.5f * _angularvelocity;

	float rot_angle, sideslip;
	if ( fabs(vel.x) < EPS )
	{
		rot_angle = 0.0f;
		sideslip = 0.0f;
	}
	else
	{
	   rot_angle = atan( yawspeed / vel.x);
	   sideslip = atan( vel.y / vel.x);
	}
	
	// Calculate slip angles for front and rear wheels (a.k.a. alpha)
	float slipanglefront, slipanglerear;
	slipanglefront = sideslip + rot_angle - _steerangle;
	slipanglerear  = sideslip - rot_angle;

	// weight per axle = half car mass times 1G (=9.8m/s^2)
	float weight = CAR_MASS * 9.8f * 0.5f;

	// lateral force on front wheels = (Ca * slip angle) capped to friction circle * load
	Vector2 flatf;
	flatf.x = 0;
	flatf.y = CA_F * slipanglefront;
	flatf.y = MIN(MAX_GRIP, flatf.y);
	flatf.y = MAX(-MAX_GRIP, flatf.y);
	flatf.y *= weight;
	if(_front_slip)
	{
		flatf.y *= 0.5;
	}

	// lateral force on rear wheels
	Vector2 flatr;
	flatr.x = 0;
	flatr.y = CA_R * slipanglerear;
	flatr.y = MIN(MAX_GRIP, flatr.y);
	flatr.y = MAX(-MAX_GRIP, flatr.y);
	flatr.y *= weight;
	if (_rear_slip)
	{
		flatr.y *= 0.5;
	}


	// longtitudinal force on rear wheels - very simple traction model
	Vector2 ftraction;
	ftraction.x = 100*(_throttle - _brake*fsgn(vel.x));
	ftraction.y = 0;

	// TMP: disable slip
	// if(_rear_slip)
	// {
	// 	ftraction.x *= 0.5;
	// }

	// Forces and torque on body

	// drag and rolling resistance
	Vector2 resistance;
	resistance.x = -( RESISTANCE*vel.x + DRAG*vel.x*fabs(vel.x) );
	resistance.y = -( RESISTANCE*vel.y + DRAG*vel.y*fabs(vel.y) );

	// sum forces
	Vector2 force;
	force.x = ftraction.x + sin(_steerangle) * flatf.x + flatr.x + resistance.x;
	force.y = ftraction.y + cos(_steerangle) * flatf.y + flatr.y + resistance.y;

	// torque on body from lateral forces
	float torque = CAR_B * flatf.y - CAR_C * flatr.y;

	// Acceleration

	// F = m.a, therefore a = F/m
	Vector2 acceleration = Vector2MultScalar( force, 1.0f/CAR_MASS );
	float angular_acceleration;
	//acceleration = force/CAR_MASS;		
	angular_acceleration = torque / CAR_INERTIA;

	// Velocity and position

	// transform acceleration from car reference frame to world reference frame
	Vector2 acceleration_wc;
	acceleration_wc.x =  cs * acceleration.y + sn * acceleration.x;
	acceleration_wc.y = -sn * acceleration.y + cs * acceleration.x;

	// velocity is integrated acceleration  	
	_vel.x += dt * acceleration_wc.x;   
	_vel.y += dt * acceleration_wc.y;   

	// position is integrated velocity  
	_pos = Vector2Add( _pos, Vector2MultScalar( _vel, dt ) );

	// Angular velocity and heading
	
	// integrate angular acceleration to get angular velocity   
	_angularvelocity += dt * angular_acceleration;

	// HACK to keep from spinning
	// float CLAMP_ANG_VEL = -200.0f;
	// _angularvelocity = clampf( -CLAMP_ANG_VEL, CLAMP_ANG_VEL, _angularvelocity );
	
	// integrate angular velocity to get angular orientation   
	_angle += dt * _angularvelocity ;

	// get graphical pos/angle from sim
	_tireAngle = _steerangle * (180.0f / M_PI );
	_carPos = Vector3Make( _pos.x, 0.0, _pos.y );

	// press back to STOP
#if 0
	if (ctrl->IsButtonPressed( Controller::BTN_BACK))
	{
		g_carSim.vel = vec2f( 0.0f, 0.0f );
		g_carSim.angularvelocity = 0.0f;
	}

	// press start to PANIC RESET if sim gets screwed up
	if (ctrl->IsButtonPressed( Controller::BTN_START))
	{
		g_carSim.pos = vec2f( 0.0f, 0.0f );
		g_carSim.vel = vec2f( 0.0f, 0.0f );
		g_carSim.angularvelocity = 0.0f;
		g_carSim.angle = 0.0f;
	}
#endif

	// Update graphs

	// convert m/s to mph
	_speedMph = Vector2Lenght( _vel ) * 2.23694;
	UpdatePhysicsGraph( &_graphSpeed, _speedMph );

	UpdatePhysicsGraph( &_graphAngle, fmod( _angle * RAD2DEG, 180.0f ) );
	UpdatePhysicsGraph( &_graphAngVel, _angularvelocity * RAD2DEG );

}
