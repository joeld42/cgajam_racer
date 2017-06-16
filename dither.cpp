// ===================================================================
//     DITHER EFFECT
//
// From my LD38 entry, modified to use CGA instead
// https://github.com/joeld42/ld48jovoc/tree/master/ld37_oneroom
// ===================================================================

#include <stdio.h>
#include <stdlib.h>

#include "raymath.h"
#include "math.h"

#include <OpenGL/gl3.h>


// raylib defines "DEBUG" for loggins :(
#ifdef DEBUG
#undef DEBUG
#endif
#include "rlgl.h"

#include "dither.h"

// ARNE retro 16-color palette from
// https://androidarts.com/palette/16pal.htm
Color arnePalette[16] = {
    { 0, 0, 0, 255 },
    { 157, 157, 157, 255 },
    { 255, 255, 255, 255 },
    { 190, 38, 51, 255 },
    { 224, 111, 139, 255 }, // MEAT
    //{ 220, 220, 230, 255 }, // LIGHTER GRAY
    { 73, 60, 43, 255 },
    { 164, 100, 34, 255 },
    { 235, 137, 49, 255 },
    { 247, 226, 107, 255 },
    { 47, 72, 78, 255 },
    { 68, 137, 26, 255 },
    { 163, 206, 39, 255 },
    { 27, 38, 50, 255 },
    { 0, 87, 132, 255 },
    { 49, 162, 242, 255 },
    { 178, 220, 239, 255 },
};

Color cgaPalette[4] = {
    { 0, 0, 0, 255 },
    { 85, 255, 255, 255 },
    { 255, 85, 255, 255 },
    { 255, 255, 255, 255 },
    };

// Bayer dither pattern
// https://en.wikipedia.org/wiki/Ordered_dithering
unsigned char bayerDither[16] = {
    0, 8, 2, 10,
    12, 4, 14, 6,
    3, 11, 1, 9,
    15, 7, 13, 6
};

int checkForGLErrors( const char *s, const char * file, int line )
{
    int errors = 0 ;
    int counter = 0 ;
    
    while ( counter < 1000 )
    {
        GLenum x = glGetError() ;
        
        if ( x == GL_NO_ERROR )
            return errors ;
        
        //printf( "%s:%d [%s] OpenGL error: %s [%08x]\n",  file, line, s ? s : "", gluErrorString ( x ), x ) ;
        printf( "%s:%d [%s] OpenGL error: [%08x]\n",  file, line, s ? s : "",  x ) ;
        errors++ ;
        counter++ ;
    }
    return 0;
}



// For now just use RGB distance, should use a better distance like L*a*b* deltaE
float ColorDistance( Color a, Color b) {
    return sqrt( (a.r-b.r)*(a.r-b.r) + (a.g-b.g)*(a.g-b.g) + (a.b-b.b)*(a.b-b.b) );
}

// from
// http://www.compuphase.com/cmetric.htm
double ColorDistance2(Color e1, Color e2)
{
    long rmean = ( (long)e1.r + (long)e2.r ) / 2;
    long r = (long)e1.r - (long)e2.r;
    long g = (long)e1.g - (long)e2.g;
    long b = (long)e1.b - (long)e2.b;
    return sqrt((((512+rmean)*r*r)>>8) + 4*g*g + (((767-rmean)*b*b)>>8));
}

GLuint MakePaletteTexture( int size ) {
    GLuint texPalette;

    uint8_t *paldata = (uint8_t*)malloc( size*size*size*3 );
    
    int shiftAmt =0;
    int t = size;
    while (t < 255) {
        shiftAmt += 1;
        t <<= 1;
    }
    printf("shiftamt %d %d\n", shiftAmt, size << shiftAmt );
    
    for (int i=0; i < size; i++) {
        for (int j=0; j < size; j++) {
            for (int k=0; k < size; k++) {
                int ndx = ((k*size*size) + (j*size) + i) * 3;
                Color cellc;
                cellc.r = (i << shiftAmt);
                cellc.g = (j << shiftAmt);
                cellc.b = (k << shiftAmt);
                
                int bestIndex = 0;
                float bestDistance = 0.0;
                for (int pal=0; pal < 4; pal++) {
                    float d = ColorDistance2( cgaPalette[pal], cellc );
                    if ((pal==0)||(d < bestDistance)) {
                        bestIndex = pal;
                        bestDistance = d;
                    }
                }
                
                Color palColor = cgaPalette[bestIndex];
                paldata[ndx + 0] = palColor.r;
                paldata[ndx + 1] = palColor.g;
                paldata[ndx + 2] = palColor.b;
                
//                paldata[ndx + 0] = cellc.r;
//                paldata[ndx + 1] = cellc.g;
//                paldata[ndx + 2] = cellc.b;
//                
//                paldata[ndx + 0] = rand() & 0xFF;
//                paldata[ndx + 1] = rand() & 0xFF;
//                paldata[ndx + 2] = rand() & 0xFF;
            }
        }
    }
    
    // Make a 3D pallete lookup texture
    glGenTextures( 1, &texPalette );
    glBindTexture( GL_TEXTURE_3D, texPalette );
    CHECKGL("makepal");
    
    glTexImage3D( GL_TEXTURE_3D, 0, GL_RGB8, size, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, paldata );
    CHECKGL("makepal teximage");
    
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    
    free (paldata);

    printf("Palette texture is texid %d\n", texPalette );
    return texPalette;
}

GLuint MakeBayerDitherTexture()
{
    GLuint texBayerDither;

    // Make a 2d Bayer dither texture
    
    // scale up to 0..255
    for (int i=0; i < 16; i++) {
        bayerDither[i] = bayerDither[i] * 17;
    }
    
    glGenTextures( 1, &texBayerDither );
    glBindTexture( GL_TEXTURE_2D, texBayerDither );
    CHECKGL("makepal");
    
    glTexImage2D( GL_TEXTURE_2D, 0, GL_R8, 4, 4, 0, GL_RED, GL_UNSIGNED_BYTE, bayerDither );
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    
    printf("texBayer is %d\n", texBayerDither );
    CHECKGL("makepal texbayer");


    return texBayerDither;
}
