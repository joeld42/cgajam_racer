
#include <stdio.h> // for printf

#include <OpenGL/gl3.h>

#include "rlgl.h"
#include "raylib.h"
#include "carsim.h"
#include "util.h"
#include "track.h"
#include "dither.h"

// sync tracker from librocket

//#define SYNC_PLAYER
#include "sync.h"

// soloud audio
#include "soloud.h"
#include "soloud_wav.h"
#include "soloud_echofilter.h"

SoLoud::Soloud soloud;  
SoLoud::Wav soloud_music;
int hmusic; 

// Music stuff
static struct sync_device *rocket;
#if !defined(SYNC_PLAYER)
static struct sync_cb cb;
#endif
#define sizeof_array(array) (int)(sizeof(array)/sizeof(array[0]))

int audio_is_playing = 1;
int curtime_ms = 0;

static const float bpm = 135.0f; /* beats per minute */
static const int rpb = 8; /* rows per beat */
static const double row_rate = (double(bpm) / 60) * rpb;

// global sync tracks
float row_f = 0.0;
const sync_track *syncGridSize = NULL;

// ===================================================================================

#define MAX_COLUMNS 20

#define CGA_BLACK    CLITERAL{ 0, 0, 0, 255 }
#define CGA_CYAN     CLITERAL{ 0, 255, 255, 255 }
#define CGA_MAGENTA  CLITERAL{ 255, 0, 255, 255 }
#define CGA_WHITE    CLITERAL{ 255, 255, 255, 255 }

Model cycleMesh;
Texture2D cycleTextureBase;
RenderTexture2D cycleTexture;
float steerAmount = 0.0;

// World stuff
Model worldMesh;
Model tunnelMesh;
Model tunnelEntrMesh;
Model hexagonMesh;

Model torusModel;
Texture2D torusTexture;
Texture2D torusMtlmapTexture;
Vector3 torusAxisRaw = { 0.0, 1.0, 0.0 };
Vector3 torusAxis = { 0.0, 1.0, 0.0 };
float torusAngle = 0.0;

// Hud stuff
SpriteFont jupiterFont;

// ===================================================================================
//  Dither Effect
// ===================================================================================
GLuint texPalette;
GLuint texBayerDither;

Texture2D gradientMapTexture;

Texture2D ditherTestTexture;
Texture2D ditherMtlTestTexture;

// ===================================================================================
// Editor, config, display options
// ===================================================================================
bool doPixelate = true;
bool doCGAMode = true;
bool editMode = false;
bool showMultipass = true;
bool gradTest = false;
bool paused = false;

Track raceTrack;

// camera nonsense
const int numAvgFollow = 120;
Vector3 pastFollow[numAvgFollow];


// ===================================================================================
//  Sync tracker stuff
// ===================================================================================

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int row_to_ms_round(int row, float rps) 
{
    const float newtime = ((float)(row)) / rps;
    return (int)(floor(newtime * 1000.0f + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static float ms_to_row_f(int time_ms, float rps) 
{
    const float row = rps * ((float)time_ms) * 1.0f/1000.0f;
    return row;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int ms_to_row_round(int time_ms, float rps) 
{
    const float r = ms_to_row_f(time_ms, rps);
    return (int)(floor(r + 0.5f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if !defined(SYNC_PLAYER)

// --- soloud sync callbacks
static int sosync_is_playing( void *data )
{
    return soloud.getPause( hmusic )?0:1;
}

static void sosync_pause( void *data, int flag )
{    
    soloud.setPause( hmusic, flag );
}

static void sosync_set_row( void *data, int row ) 
{        
    int newtime_ms = row_to_ms_round( row, row_rate );
    float sectime = (float)newtime_ms / 1000.0f;
    soloud.seek( hmusic, sectime );
}


#endif //!SYNC_PLAYER

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int rocket_init(const char* prefix) 
{
    rocket = sync_create_device( prefix );
    if (!rocket) 
    {
        printf("Unable to create rocketDevice\n");
        return 0;
    }

#if !defined( SYNC_PLAYER )
    // cb.is_playing = xis_playing;
    // cb.pause = xpause;
    // cb.set_row = xset_row;

    cb.is_playing = sosync_is_playing;
    cb.pause = sosync_pause;
    cb.set_row = sosync_set_row;

    if (sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT)) 
    {
        printf("Rocket failed to connect\n");
        return 0;
    }
#endif

    printf("Rocket connected.\n");

    return 1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static int rocket_update()
{
    int row = 0;

#if !defined( SYNC_PLAYER )

    float currTime = soloud.getStreamTime( hmusic );
    curtime_ms = (int)(currTime * 1000.0f);
    row = ms_to_row_round( curtime_ms, row_rate );

    //row = ms_to_row_round(curtime_ms, rps);
    if (sync_update(rocket, row, &cb, 0)) 
        sync_tcp_connect(rocket, "localhost", SYNC_DEFAULT_PORT);
#endif

    return 1;
}


// ===================================================================================
// Game
// ===================================================================================


void DrawGroundSquares()
{
    float gridSz = 5.0f + (sync_get_val( syncGridSize, row_f ) * 15.0f);

    int SZ = 10;
    for (int j=-SZ; j <= SZ; j++) {
        for (int i=-SZ; i <= SZ; i++) {
            Vector3 pos = {0.0f };
            pos.x = i * 20.0f;
            pos.y = -0.5f;
            pos.z = j * 20.0f;
            Color groundColor = (Color)CGA_CYAN;
            if (editMode) {
                groundColor = (Color){ 20, 20, 40, 0xff };
            }
            DrawPlane( pos, (Vector2){ gridSz, gridSz }, groundColor );
        }
    }
}

void DrawShapes()
{
    DrawSphere((Vector3){-1.0f, 1.0f, -2.0f}, 1.0f, (Color)CGA_CYAN);
    DrawSphereWires((Vector3){-1.0f, 1.0f, -2.0f}, 1.01f, 16, 16, (Color)CGA_BLACK);
}

void DrawHUDText( char *text, float x, float y, int fontsize, Color color )
{
    DrawTextEx( jupiterFont, text, Vector2Make( x, y ),
                    jupiterFont.baseSize * fontsize, 1, color);
}

void DrawTextOutlined( char *text, float x, float y, int fontSize, Color color )
{
    DrawHUDText( text, x-1, y-1, fontSize, (Color)BLACK );
    DrawHUDText( text, x+1, y-1, fontSize, (Color)BLACK );
    DrawHUDText( text, x-1, y+1, fontSize, (Color)BLACK );
    DrawHUDText( text, x+1, y+1, fontSize, (Color)BLACK );

    DrawHUDText( text, x, y, fontSize, color );
}

void DrawHud( CarModel *carSim, Rectangle screenRect, Color color )
{
    // Speed indicator
    char buff[200];
    sprintf(buff, "%3.0f", carSim->_speedMph );
    DrawTextOutlined( buff, screenRect.x + 10, screenRect.y + screenRect.height - 40, 1, color );
}

void DrawScene( CarModel *carSim, Shader shader )
{
    //DrawGroundSquares();
    DrawShapes();

    raceTrack.drawTrack( shader );

    worldMesh.material.shader = shader;    
    DrawModel( worldMesh, (Vector3){ 0.0, 0.0, 0.0}, 1.0, (Color)WHITE );

    tunnelMesh.material.shader = shader;
    DrawModel( tunnelMesh, (Vector3){ 0.0, 0.0, 0.0}, 1.0, (Color)WHITE ); // SYNC: Tint tunnel light color here

    tunnelEntrMesh.material.shader = shader;
    DrawModel( tunnelEntrMesh, (Vector3){ 0.0, 0.0, 0.0}, 1.0, (Color)WHITE ); // SYNC: Tint tunnel light color here

    hexagonMesh.material.shader = shader;
    DrawModel( hexagonMesh, (Vector3){ 0.0, 0.0, 0.0}, 1.0, (Color)WHITE ); // SYNC: Tint tunnel light color here


    // Draw the car
    Vector3 carPos = {0};
    carPos.x = carSim->_pos.x;
    carPos.y = 0.5f;
    carPos.z = carSim->_pos.y;

    //DrawCube( carPos, 0.5f, 0.5f, 0.5f, (Color)CGA_MAGENTA );
    //DrawCubeWires(carPos, 0.51f, 0.51f, 0.51f, (Color)RED );

    // NOTES:
    // engine Pos 

    cycleMesh.material.shader = shader;

        // Tilt
    cycleMesh.transform = MatrixRotate( (Vector3){0.0f, 0.0f, 1.0f}, steerAmount * DEG2RAD * 30.0f );

#if 1
    DrawModelEx( cycleMesh, carPos, (Vector3){0.0f, 1.0f, 0.0f},
                    -RAD2DEG * (carSim->_angle), (Vector3){1.0f, 1.0f, 1.0f}, 
                    (Color)WHITE );

    // Draw flattened for shadow
    DrawModelEx( cycleMesh, carPos, (Vector3){0.0f, 1.0f, 0.0f},
                    -RAD2DEG * (carSim->_angle), (Vector3){1.0f, 0.0f, 1.0f}, 
                    (Color)BLACK );
#else

    Matrix transform;


    Matrix matRotation = MatrixRotate((Vector3){0.0f, 1.0f, 0.0f}, -carSim->_angle );
    Matrix matScale = MatrixScale(1.0f, 1.0f, 1.0f);
    Matrix matTranslation = MatrixTranslate(carPos.x, carPos.y, carPos.z);
    
    Matrix matTransform = MatrixMultiply(MatrixMultiply(matScale, matRotation), matTranslation);

    // Combine model transformation matrix (model.transform) with matrix generated by function parameters (matTransform)
    //Matrix matModel = MatrixMultiply(model.transform, matTransform);    // Transform to world-space coordinates

    transform = MatrixMultiply(cycleMesh.transform, matTransform);
    cycleMesh.material.colDiffuse = (Color)WHITE;

    rlglDrawMesh(cycleMesh.mesh, cycleMesh.material, transform);

#endif
    
    // Vector3 torusPos = VectorAdd( carPos, (Vector3){ 0.0, 0.0, 0.0});
    // torusModel.material.shader = shader;
    // DrawModelEx( torusModel, torusPos, torusAxis, torusAngle, (Vector3){3.0f, 3.0f, 3.0f}, (Color)WHITE );


}


void EnableDitherEffect( Shader &cgaShader )
{
        // directly bind palette shader
    static GLint samplerPally = -2;
    static GLint samplerDither = -2;
    static GLint samplerMask = -2;
    static GLint samplerGradientMap = -2;

    CHECKGL("start");
    if (samplerPally < -1) {
        
        glUseProgram( cgaShader.id);
        
        samplerPally = glGetUniformLocation( cgaShader.id, "pally" );
        CHECKGL("get uniform");
        printf("pixelize shader prog %d samplerId %d\n", cgaShader.id, samplerPally );
        
        samplerDither = glGetUniformLocation( cgaShader.id, "dither" );

        samplerMask = glGetUniformLocation( cgaShader.id, "mtlmask" );
        
        samplerGradientMap = glGetUniformLocation( cgaShader.id, "gradientmap" );

        //ditherStrengthParam = glGetUniformLocation( cgaShader.id, "ditherStrength" );
    }
    
    if (samplerPally >= 0) {
        glUseProgram(cgaShader.id);
        CHECKGL("use shader");
        
        glUniform1i( samplerGradientMap, 1 );
        CHECKGL("set gradientmap sampler");

        glUniform1i( samplerMask, 2 );
        CHECKGL("set mtlmask sampler");

        glUniform1i( samplerPally, 3 );
        CHECKGL("set pally sampler");
        
        glUniform1i( samplerDither, 4 );
        CHECKGL("set dither sampler");
        
        //glUniform1f( ditherStrengthParam, ditherStrength );

        
        glActiveTexture( GL_TEXTURE3 );
        glBindTexture( GL_TEXTURE_3D, texPalette );
        CHECKGL("bind pally texture");
        
        glActiveTexture( GL_TEXTURE4 );
        glBindTexture( GL_TEXTURE_2D, texBayerDither );
        CHECKGL("bind dither texture");
        
        glActiveTexture( GL_TEXTURE0 );
    }

}

void ResetToStartPos( CarModel *carSim )
{
    // DBG reset
    float trackStartP = 0.5; // start pos parametric along track
    Vector3 trackStart = raceTrack.evalTrackCurve( trackStartP );
    Vector3 trackStart2 = raceTrack.evalTrackCurve( trackStartP + 0.01 );
    Vector3 startDir = VectorSubtract( trackStart2, trackStart );

    carSim->_pos = Vector2Make( trackStart.x, trackStart.z );
    carSim->_carPos = Vector3Make( trackStart.x, 0.0, trackStart.z );
    carSim->_vel = Vector2Make( 0.0f, 0.0f );
    carSim->_angularvelocity = 0.0f;
    carSim->_angle = atan2( startDir.x, startDir.z ); 

    VectorNormalize( &startDir );
    startDir.x *= -1.0;
    startDir.z *= -1.0;
    for (int i=0; i < numAvgFollow; i++) {
        pastFollow[i] = startDir;
    }
}

int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    int screenWidth = 800;
    int screenHeight = 450;
    //int screenWidth = 1280;
    //int screenHeight = 800;
    
    int pixelWidth = 320;
    int pixelHeight = 200;

    float editCamHite = 200.0f;

    // initialize SoLoud.
    soloud.init();
    

    if (!rocket_init("data/sync")) {
        return -1;
    }

    InitWindow(screenWidth, screenHeight, "CGAJAM - Racer4c");

    RenderTexture2D pixelTarget = LoadRenderTexture(pixelWidth, pixelHeight);
    SetTextureFilter( pixelTarget.texture, FILTER_POINT );
    
    RenderTexture2D mtlModeTarget = LoadRenderTexture(pixelWidth, pixelHeight);
    SetTextureFilter( mtlModeTarget.texture, FILTER_POINT );

    //RenderTexture2D postProcTarget = LoadRenderTexture(pixelWidth, pixelHeight);
    //SetTextureFilter( postProcTarget.texture, FILTER_POINT );    

    // Define the camera to look into our 3d world (position, target, up vector)
    Camera camera = {{ 4.0f, 2.0f, 4.0f }, { 0.0f, 1.8f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 60.0f };
    
    Camera editCamera = {0};

    SetCameraMode(editCamera, CAMERA_FREE); 
    SetCameraMode(camera, CAMERA_FREE ); 


    SetTargetFPS(60);                           // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------


    Shader worldShader = LoadShader( (char *)"world.vs", (char *)"world.fs");
    Shader worldMtlShader = LoadShader( (char *)"world.vs", (char *)"world_mtl.fs");
    Shader cgaShader = LoadShader( (char *)"cga_enforce.vs", (char *)"cga_enforce.fs");


    soloud_music.load("turbo_electric_16pcm.wav"); 

    // Shorter excerpt for faster load during testing..
    //soloud_music.load("turbo_electric_16pcm_excerpt.wav"); 
        
    // SoLoud::EchoFilter echo;
    // echo.setParams( 0.5f, 0.5f );
    // soloud_music.setFilter( 0, &echo );

    hmusic = soloud.play(soloud_music);   
    printf("Play music: result %d\n", hmusic );


    syncGridSize = sync_get_track( rocket, "env:gridsz");

    // Game Stuff Init
    CarModel carSim;
    srand( 0x6BA8F );
    raceTrack.genRandom();

    raceTrack.buildTrackMesh();
    //raceTrack.trackModel.material.shader = worldShader;

    ResetToStartPos( &carSim );

    cycleMesh = LoadModel("cycle1.obj");
    cycleTextureBase = LoadTexture("cycle_col.png");
    cycleTexture = LoadRenderTexture(cycleTextureBase.width, cycleTextureBase.height );
    //SetTextureFilter( cycleTexture.texture, FILTER_POINT );

    cycleMesh.material.texSpecular = LoadTexture("cycle_maps.png");; 
    cycleMesh.material.texDiffuse = cycleTexture.texture; 

    //cycleMesh.material.shader = worldShader;

    worldMesh = LoadModel("mountain1.obj");
    worldMesh.material.texDiffuse = LoadTexture("mountain1.png");
    worldMesh.material.texSpecular = LoadTexture("mountain_mtl.png");

    tunnelMesh = LoadModel("tunnel_inside.obj");
    tunnelMesh.material.texDiffuse = LoadTexture("tunnel_inside.png");
    tunnelMesh.material.texSpecular = LoadTexture("tunnel_inside_mtl.png");

    tunnelEntrMesh = LoadModel("tunnel_entr.obj");
    tunnelEntrMesh.material.texDiffuse = LoadTexture("tunnel_entr.png");
    tunnelEntrMesh.material.texSpecular = LoadTexture("tunnel_entr_mtl.png");

    hexagonMesh = LoadModel("hexagon.obj");
    hexagonMesh.material.texDiffuse = LoadTexture("hexagon.png");
    hexagonMesh.material.texSpecular = LoadTexture("hexagon_mtl.png");


    torusModel = LoadModel("test_obj_smooth.obj");
    torusTexture = LoadTexture( "testobj_color.png");
    torusMtlmapTexture = LoadTexture( "testobj_maps.png");
    torusModel.material.texDiffuse = torusTexture;
    torusModel.material.texSpecular = torusMtlmapTexture;
    //torusModel.material.shader = worldShader;

    ditherTestTexture = LoadTexture( "gradtest.png");
    ditherMtlTestTexture = LoadTexture("gradtest_mtl.png");

    // IMPORTANT for cgamode shader
    gradientMapTexture = LoadTexture("gradientmap.png");

    int grabPointNdx = 0;
    bool grabbed = false;

    // setup HUD stuff
    jupiterFont = LoadSpriteFont("jupiter_crash.png");

    // Set up pixelate filter
    texPalette = MakePaletteTexture(64);
    texBayerDither = MakeBayerDitherTexture();
    
    //PhysicsGraph turn;

    // Main game loop
    while (!WindowShouldClose())                // Detect window close button or ESC key
    {

         // Update sync tracker
        rocket_update();
        row_f = ms_to_row_f(curtime_ms, row_rate);

        // Update
        //----------------------------------------------------------------------------------        
        float dt = 1.0/60.0;

        float throttle = 0.0f;
        float turn = 0.0f;
        bool brake = false;
        float minTurn = -1.0;
        float maxTurn = 1.0;
        if (IsKeyDown(KEY_UP)) {
            throttle += 1.0f;
        } 
        if (IsKeyDown(KEY_DOWN)) {
            brake = true;
        } 

        // TODO: make wheel continuous, and recenter
        bool isSteering = false;
        if (IsKeyDown(KEY_RIGHT)) {
            turn -= 1.0;
            isSteering = true;
        } else if (IsKeyDown(KEY_LEFT)) {
            turn += 1.0;
            isSteering = true;
        }

        if (turn > 0.1) {
            minTurn = 0.0;
        } else if (turn < -0.1) {
            maxTurn = 0.0;
        }

        if (!isSteering) {
            // return to center if no turn pushed
            steerAmount *= 0.75;
        } else {
            steerAmount += turn*5.0*dt;            
            steerAmount = clampf(  minTurn, maxTurn, steerAmount );
        }

        // DBG/Edit keys
        if (IsKeyPressed(KEY_P)) {
            paused = !paused;
        }
        if (IsKeyPressed(KEY_Z)) {
            ResetToStartPos( &carSim );
        }
        if (IsKeyPressed(KEY_X)) {
            // only reset velocity
            carSim._vel = Vector2Make( 0.0f, 0.0f );
            carSim._angularvelocity = 0.0f;
        }
        if (IsKeyPressed(KEY_D)) {
            static int screenyNum = 0;
            char buff[200];
            sprintf(buff, "screenys/cgaracer%04d.png", screenyNum++ );
            TakeScreenshot( buff );
        }
        if (IsKeyPressed(KEY_NINE)) {
            doPixelate = !doPixelate;
        }
        if (IsKeyPressed(KEY_EIGHT)) {
            showMultipass = !showMultipass;
        }
        if (IsKeyPressed(KEY_SEVEN)) {
            gradTest = !gradTest;
        }
        if (IsKeyPressed(KEY_ZERO)) {
            doCGAMode = !doCGAMode;
        }
        if (IsKeyPressed(KEY_FIVE)) {
            int seed = rand() % 0xfffff;
            srand( seed );
            printf("TRACK SEED: %05X\n", seed );

            raceTrack.genRandom();
        }
        if (IsKeyPressed(KEY_E)) {


             //= {{ 4.0f, 10.0f, 100.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 90.0f };
            editMode = !editMode;

            if (editMode) {
                // Edit mode
                editCamera.position = (Vector3){ 4.0f, editCamHite, 50.0f};
                editCamera.target = (Vector3){ 0.0f, 0.0f, 0.0f};

                Vector3 facingDir = VectorSubtract( editCamera.target, editCamera.position );
                editCamera.up = VectorCrossProduct(  (Vector3){ 1.0f, 0.0f, 0.0f}, facingDir );
                VectorNormalize( &(editCamera.up) );
                //editCamera.up = (Vector3){ 0.0f, 1.0f, 0.0f};
                editCamera.fovy = 90.0f;
                SetCameraMode(editCamera, CAMERA_FREE);

                // editCamera = camera;
            }
        }

        if (editMode) {

            int mouseWheel = GetMouseWheelMove();
            //printf("MouseWHeel: %d hite %f\n", mouseWheel, editCamHite );
            editCamHite = clampf( 10.0f, 300.0f, editCamHite + (float)mouseWheel  );

            Vector3 camCenterPos = VectorLerp( carSim._carPos, (Vector3){0.0f, 0.0f, 0.0f}, 
                                    (editCamHite - 10.0) / (300.0 - 10.0) );

            editCamera.position = VectorAdd( camCenterPos, (Vector3){ 4.0f, 0.0f, 5.0f} );
            editCamera.position.y = editCamHite;
            editCamera.target =  camCenterPos;

            if (IsKeyPressed(KEY_P)) {
                // Print Track Points
                for (int i=0; i < raceTrack.nTrackPoints; i++) {
                    printf("    addTrackPoint( %f, %f );\n", 
                        raceTrack.point[i].pos.x,
                        raceTrack.point[i].pos.z );
                }
            }
            if (IsKeyPressed(KEY_B)) {
                raceTrack.buildTrackMesh();
            }            

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            {
                Ray ray = GetMouseRay(GetMousePosition(), editCamera );                
                RayHitInfo groundHitInfo = GetCollisionRayGround(ray, 0.0f);

                printf("Ground Hit: %3.2f %3.2f %3.2f\n",
                    groundHitInfo.position.x,
                    groundHitInfo.position.y,
                    groundHitInfo.position.z );

                float bestDist = 0.0;
                int bestNdx = 0;
                for (int i=0; i < raceTrack.nTrackPoints; i++) {
                    float d = VectorDistance( raceTrack.point[i].pos, groundHitInfo.position );
                    if ((i==0) || (d < bestDist)) {
                        bestDist = d;
                        bestNdx = i;
                    }
                }

                if (bestDist < 10.0) {
                    grabbed = true;
                    grabPointNdx = bestNdx;
                    printf("Grab point %d\n", grabPointNdx );
                }

            }
            else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
                grabbed = false;
            }
            else if (grabbed) {
                Ray ray = GetMouseRay(GetMousePosition(), editCamera );                
                RayHitInfo groundHitInfo = GetCollisionRayGround(ray, 0.0f);
                raceTrack.point[grabPointNdx].pos = groundHitInfo.position;
            }
        
        }

        if (!editMode) {
            static float time = 0.0;            

            if (!paused) {
                time += dt;

                int numSubstep = 10;
                for (int substep=0; substep < numSubstep; substep++) {
                    Vector3 prevCarPos = carSim._carPos;
                    Vector2 prevCarPos2 = carSim._pos;

                    Vector3 hitPos = {0};
                    Vector3 hitNorm = {0};
                    carSim.Update( dt / (float)numSubstep, throttle, steerAmount, brake );
                    if (raceTrack.checkCollide( prevCarPos, carSim._carPos, &hitPos, &hitNorm )) {
                        Vector3 carVel = Vector3Make( carSim._vel.x, 0.0, carSim._vel.y );
                        carVel = VectorReflect( carVel, hitNorm );

                        // extra bump away from the wall
                        carVel = VectorAdd( carVel, Vector3MultScalar( hitNorm, -0.1 ));

                        carSim._carPos = prevCarPos;
                        carSim._pos = prevCarPos2;
                        float hitAbsorb = 0.8;
                        carSim._vel = Vector2Make( carVel.x*hitAbsorb, carVel.z*hitAbsorb );
                        printf("Hit rail: %3.2f %3.2f %3.2f\n",
                            hitPos.x, hitPos.y, hitPos.z );
                        printf("Prev Pos %3.2f %3.2f pos %3.2f, %3.2f\n",
                            prevCarPos.x, prevCarPos.z,
                            carSim._carPos.x, carSim._carPos.z
                            );

                    }
                }
            }

            if (1) {
                // random spin axis
                torusAxisRaw.x = sin( time );
                torusAxisRaw.y = sin( time * 1.23 );
                torusAxisRaw.z = sin( time * 1.78 );

                torusAxis = torusAxisRaw;
                VectorNormalize( &torusAxis );
            }

            torusAngle += 0.3f;            
        }

        Vector3 camTarget = Vector3Make( carSim._pos.x, 0.0f, carSim._pos.y );
        camera.target = camTarget;
        Vector3 followDir = (Vector3){0.0, 0.0, 1.0f };


        if (Vector2Lenght( carSim._vel) > 0.0f) {
            followDir = Vector3Make( -carSim._vel.x, 0.0f, -carSim._vel.y );
            VectorNormalize( &followDir );

            for (int i=1; i < numAvgFollow; i++) {
                pastFollow[i-1] = pastFollow[i];
            }
            pastFollow[numAvgFollow-1] = followDir;

            followDir = (Vector3){0};
            for (int i=0; i < numAvgFollow; i++) {
                followDir = VectorAdd( followDir, pastFollow[i]);
            }
            VectorNormalize( &followDir );

            
        }
        // printf("Vel %3.2f %3.2f LEN %3.2f FollowDir: %3.2f %3.2f %3.2f\n", 
        //     carSim._vel.x, carSim._vel.y,
        //     followDir.x, followDir.y, followDir.z );

        Vector3 cameraOffset = Vector3MultScalar( followDir, 10.0f );
        cameraOffset.y = 8.0f;
        camera.target.y += 4.0;

        camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
        camera.position = VectorAdd( camTarget, cameraOffset);
        

        //----------------------------------------------------------------------------------
        // Draw
        //----------------------------------------------------------------------------------
        CHECKGL( "aaa");

        Rectangle pixelRect = (Rectangle){ 0, 0, pixelWidth, pixelHeight };

        BeginDrawing();

            // Update cycle texture
            BeginTextureMode( cycleTexture );
            Rectangle cycleTextureRect = (Rectangle){ 0, 0, cycleTextureBase.width, cycleTextureBase.height };
            Rectangle cycleTextureRectFlipped = (Rectangle){ 0, cycleTextureBase.height, cycleTextureBase.width, -cycleTextureBase.height  };
            DrawTexturePro( cycleTextureBase, cycleTextureRectFlipped, cycleTextureRect, 
                (Vector2){ 0, 0 }, 0, (Color)WHITE);

            // Thrusters (todo, more faded, like  temperature)

            // 301, 447
            DrawCircle( 301, 512-447, 48*throttle, (Color)WHITE );
            DrawCircle( 301, 512-447, 20*throttle, (Color)CGA_MAGENTA );

            //127, 29 wh 59, 20
            if (brake) {
                DrawRectangle( 68, 512-(57+20), 60, 20, (Color)CGA_MAGENTA );
            }

            EndTextureMode();

            bool frameDoPixelate = doPixelate && (!editMode);
            bool frameDoCgaMode = doCGAMode && frameDoPixelate;            

            Camera activeCamera = camera;
            if (editMode) {
                activeCamera = editCamera;
            }            

            CHECKGL( "aaa");
            ClearBackground( (Color)BLACK );

            BeginTextureMode(mtlModeTarget);            
            Begin3dMode(activeCamera);
            //DrawSphere( (Vector3){0.0, 0.0, 0.0f }, 1.0f, (Color)LIME );
            BeginShaderMode( worldMtlShader );
            DrawScene( &carSim, worldMtlShader );            
            EndShaderMode( );

            CHECKGL( "aaa");

            End3dMode();

            // MATERIAL pass
            DrawHud( &carSim, pixelRect, (Color){ 0, 0, 0 });

            EndTextureMode();            
            
            CHECKGL( "aaa");
            if (frameDoPixelate) {
                BeginTextureMode(pixelTarget);   // Enable drawing to texture            
            }
            CHECKGL( "aaa");

            ClearBackground( (Color)BLACK);

            Begin3dMode(activeCamera);

            BeginShaderMode( worldShader );
            DrawScene( &carSim, worldShader );            
            EndShaderMode( );
            

            if (editMode) {
                raceTrack.drawCollideSegs();

                raceTrack.drawTrackEditMode();
            }            

            End3dMode();

            DrawHud( &carSim, pixelRect, (Color)WHITE );

            if (frameDoPixelate) {
                EndTextureMode();

                ClearBackground( (Color)DARKBLUE );

                Rectangle textureRect = (Rectangle){ 0, 0, pixelTarget.texture.width, -pixelTarget.texture.height };
                Rectangle screenRect = (Rectangle){ 0, 0, screenWidth, screenHeight };
                
    //            float displayScale = 2.0;
    //            textureRect.width *= displayScale;
    //            textureRect.height *= displayScale;
    //            screenRect.width *= displayScale;
    //            screenRect.height *= displayScale;

                if (showMultipass) {
                    Rectangle previewRect1 = (Rectangle){ 0, 0, screenWidth/2, screenHeight/2 };
                    Rectangle previewRect2 = (Rectangle){ 0, screenHeight/2, screenWidth/2, screenHeight/2 }; 

                    // Modify ScreenRect for debug
                    screenRect = (Rectangle){ screenWidth/2, 0, screenWidth/2, screenHeight/2 }; 

                    DrawTexturePro( pixelTarget.texture, textureRect, previewRect1, (Vector2){ 0, 0 }, 0, (Color)WHITE);            
                    DrawTexturePro( mtlModeTarget.texture, textureRect, previewRect2, (Vector2){ 0, 0 }, 0, (Color)WHITE);            
                }

                if (frameDoCgaMode) {
                    BeginShaderMode( cgaShader );
                    EnableDitherEffect( cgaShader );

                    glActiveTexture( GL_TEXTURE1 );
                    glBindTexture( GL_TEXTURE_2D, gradientMapTexture.id );
                    CHECKGL("bind gradientMap texture");

                    glActiveTexture( GL_TEXTURE2 );
                    glBindTexture( GL_TEXTURE_2D, gradTest?ditherMtlTestTexture.id:mtlModeTarget.texture.id );
                    CHECKGL("bind mtltest texture");
        
                    glActiveTexture( GL_TEXTURE0 );
                }
                
                if (gradTest) {
                    textureRect = (Rectangle){ 0, 0, ditherTestTexture.width, ditherTestTexture.height };
                    DrawTexturePro( ditherTestTexture, textureRect, screenRect, (Vector2){ 0, 0 }, 0, (Color)WHITE);
                } else {
                    DrawTexturePro( pixelTarget.texture, textureRect, screenRect, (Vector2){ 0, 0 }, 0, (Color)WHITE);
                }
                

                if (frameDoCgaMode) {
                   EndShaderMode();
                }
            }

            if (showMultipass) {
                Rectangle graphRect = { screenWidth/2, screenHeight/2, screenWidth/2, 50 };
                DrawPhysicsGraph( &(carSim._graphSpeed), graphRect );

                graphRect.y += 52;
                DrawPhysicsGraph( &(carSim._graphAngle), graphRect );

                graphRect.y += 52;
                DrawPhysicsGraph( &(carSim._graphAngVel), graphRect );

                // Steering feedback gizmo
                float x1 = screenWidth/2 + 20; 
                float x2 = screenWidth - 20; 
                float steerPoint = x1 + (x2-x1) * ((-steerAmount * 0.5)+0.5);
                DrawLine( x1, screenHeight - 40, x2, screenHeight - 40, (Color)RED );
                DrawCircle( steerPoint, screenHeight - 40, 10.0, (Color)GOLD );
            }

            DrawFPS(15, screenHeight - 20);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------   
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

     // Clean up SoLoud
    soloud.deinit();


    return 0;
}