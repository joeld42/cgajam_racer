#ifndef CGAJAM_DITHER_H
#define CGAJAM_DITHER_H


int checkForGLErrors( const char *s, const char * file, int line );

//#ifdef NDEBUG
#define CHECKGL( msg )
// #else
// #define CHECKGL( msg ) checkForGLErrors( msg, __FILE__, __LINE__ );
// #endif

float ColorDistance( Color a, Color b);
double ColorDistance2(Color e1, Color e2);

GLuint MakePaletteTexture( int size );
GLuint MakeBayerDitherTexture();

#endif