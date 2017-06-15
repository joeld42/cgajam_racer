
#include <stdio.h> // for printf

#include <OpenGL/gl3.h>

#include "raylib.h"
#include "carsim.h"
#include "util.h"
#include "track.h"

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
Texture2D cycleTexture;

// ===================================================================================
// Editor, config, display options
bool doPixelate = true;
bool doCGAMode = true;
bool editMode = false;

Track raceTrack;

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

void DrawScene( CarModel *carSim )
{
    DrawGroundSquares();
    DrawShapes();

    raceTrack.drawTrack();

    // Draw the car
    Vector3 carPos = {0};
    carPos.x = carSim->_pos.x;
    carPos.y = 0.5f;
    carPos.z = carSim->_pos.y;

    DrawCube( carPos, 0.5f, 0.5f, 0.5f, (Color)CGA_MAGENTA );
    DrawCubeWires(carPos, 0.51f, 0.51f, 0.51f, (Color)RED );

    DrawModelEx( cycleMesh, carPos, (Vector3){0.0f, 1.0f, 0.0f},
                    -RAD2DEG * (carSim->_angle), (Vector3){1.0f, 1.0f, 1.0f}, 
                    (Color)WHITE );
    // RLAPI void DrawModelEx(Model model, Vector3 position, Vector3 rotationAxis,
    //                    float rotationAngle, Vector3 scale, Color tint);                                 // Draw a model with extended parameters


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

    // initialize SoLoud.
    soloud.init();
    

    if (!rocket_init("data/sync")) {
        return -1;
    }

    InitWindow(screenWidth, screenHeight, "CGAJAM - Racer4c");

    RenderTexture2D pixelTarget = LoadRenderTexture(pixelWidth, pixelHeight);
    SetTextureFilter( pixelTarget.texture, FILTER_POINT );
    
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
    raceTrack.trackModel.material.shader = worldShader;

    Vector3 trackStart = raceTrack.point[0].pos;
    carSim._pos = Vector2Make( trackStart.x, trackStart.z );

    cycleMesh = LoadModel("cubecycle.obj");
    cycleTexture = LoadTexture("cubecycle.png");
    cycleMesh.material.texDiffuse = cycleTexture; 
    cycleMesh.material.shader = worldShader;

    int grabPointNdx = 0;
    bool grabbed = false;

    // Main game loop
    while (!WindowShouldClose())                // Detect window close button or ESC key
    {

         // Update sync tracker
        rocket_update();
        row_f = ms_to_row_f(curtime_ms, row_rate);

        // Update
        //----------------------------------------------------------------------------------
        float throttle = 0.0f;
        float turn = 0.0f;
        bool brake = false;
        if (IsKeyDown(KEY_UP)) {
            throttle += 1.0f;
        } 
        if (IsKeyDown(KEY_DOWN)) {
            brake = true;
        } 

        // TODO: make wheel continuous, and recenter
        if (IsKeyDown(KEY_RIGHT)) {
            turn -= 1.0;
        } 
        if (IsKeyDown(KEY_LEFT)) {
            turn += 1.0;
        } 

        // DBG/Edit keys
        if (IsKeyPressed(KEY_Z)) {
            // DBG reset
            Vector3 trackStart = raceTrack.point[0].pos;
            carSim._pos = Vector2Make( trackStart.x, trackStart.z );
            carSim._vel = Vector2Make( 0.0f, 0.0f );
            carSim._angularvelocity = 0.0f;
            carSim._angle = 0.0f;
        }
        if (IsKeyPressed(KEY_NINE)) {
            doPixelate = !doPixelate;
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
                editCamera.position = (Vector3){ 4.0f, 200.0f, 50.0f};
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
            carSim.Update( 1.0f/60.0f, throttle, turn, brake );
        }

        Vector3 camTarget = Vector3Make( carSim._pos.x, 0.0f, carSim._pos.y );
        camera.target = camTarget;
        Vector3 followDir = (Vector3){0.0, 0.0, 1.0f };
        if (Vector2Lenght( carSim._vel) > 0.0f) {
            followDir = Vector3Make( -carSim._vel.x, 0.0f, -carSim._vel.y );
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
        glLineWidth(4.0f);
        BeginDrawing();

            bool frameDoPixelate = doPixelate && (!editMode);
            bool frameDoCgaMode = doCGAMode && frameDoPixelate;            

            Camera activeCamera = camera;
            if (editMode) {
                activeCamera = editCamera;
            }            

            ClearBackground( (Color)BLACK);
            if (frameDoPixelate) {
                BeginTextureMode(pixelTarget);   // Enable drawing to texture            
            }


            Begin3dMode(activeCamera);

            BeginShaderMode( worldShader );
            DrawScene( &carSim );
            EndShaderMode( );

            if (editMode) {
                raceTrack.drawTrackEditMode();
            }

            End3dMode();

            if (frameDoPixelate) {
                EndTextureMode();

                ClearBackground( (Color)GREEN );

                Rectangle textureRect = (Rectangle){ 0, 0, pixelTarget.texture.width, -pixelTarget.texture.height };            
                Rectangle screenRect = (Rectangle){ 0, 0, screenWidth, screenHeight };
                
    //            float displayScale = 2.0;
    //            textureRect.width *= displayScale;
    //            textureRect.height *= displayScale;
    //            screenRect.width *= displayScale;
    //            screenRect.height *= displayScale;

                if (frameDoCgaMode) {
                    BeginShaderMode( cgaShader );
                }
                // DrawTexturePro( frameDoCgaMode?postProcTarget.texture:pixelTarget.texture,
                //                textureRect, screenRect, (Vector2){ 0, 0 }, 0, (Color)WHITE);            
                DrawTexturePro( pixelTarget.texture, textureRect, screenRect, (Vector2){ 0, 0 }, 0, (Color)WHITE);            
                
                if (frameDoCgaMode) {
                   EndShaderMode();
                }
            }

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