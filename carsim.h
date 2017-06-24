#include <raylib.h>

#define MAX_HISTORY (200)
struct PhysicsGraph
{
	int numHistory;
	float history[MAX_HISTORY];
	float maxVal;
	float minVal;
};

void InitPhysicsGraph( PhysicsGraph *g, float minGraphVal, float maxGraphVal );
void UpdatePhysicsGraph( PhysicsGraph *g, float val );
void DrawPhysicsGraph( PhysicsGraph *g, Rectangle rect );

// Dummy car sim for prototype .. based on Marco Monster's 
// Car Sim for games, 
struct CarModel 
{
	Vector2 _pos;
    Vector2 _vel;

    float _angle;           // angle of car body orientation (in rads)
    float _angularvelocity;

    // player controls to the sim
    float _steerangle;      // angle of steering (input)
    float _throttle;        // amount of throttle (input)
    float _brake;           // amount of braking (input)

	// settings
	bool _front_slip, _rear_slip;

	// For visual feedback not pare of the sim
	float _tireAngle;
	Vector3 _carPos;
	float _speedMph;


	float _raceTime;
	float _lapTime;

	CarModel();	
	
	void Update( float dt, float throttle, float steer, bool brake, bool timerActive );

	PhysicsGraph _graphSpeed;
	PhysicsGraph _graphAngle;
	PhysicsGraph _graphAngVel;
};

