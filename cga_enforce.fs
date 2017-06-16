#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D gradientmap;
uniform sampler2D mtlmask;
uniform vec4 colDiffuse;

// Extra textures for the 16-color effect
uniform sampler3D pally;
uniform sampler2D dither;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

//const vec2 size = vec2(800, 450);   // render size
const vec2 size = vec2(640, 420);   // render size
const float samples = 7.0;          // pixels per axis; higher = bigger glow, worse performance
const float quality = 2.5; 	        // lower = smaller glow, better quality

void main()
{
//    vec4 sum = vec4(0);
//    vec2 sizeFactor = vec2(1)/size*quality;

    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);
    vec4 mask = texture(mtlmask, fragTexCoord);
    
    vec4 ditherColor = texture( dither, fragTexCoord*vec2(80,50) );
    float ditherStrength = mask.g;
    
    float ditherAmt = (ditherColor.r * ditherStrength) - (ditherStrength/2.0);
    vec3 lookupColor = source.rgb + vec3(ditherAmt, ditherAmt, ditherAmt);
    lookupColor = max( lookupColor, vec3(0.0, 0.0, 0.0) );
    lookupColor = min( lookupColor, vec3(0.9, 0.9, 0.9) );
    
    if (mask.b < 0.25 ) {
    	vec4 pallycolor = texture( pally, lookupColor);
		finalColor = pallycolor;
	} else {
		float gradientChoice = (mask.b - 0.25) / 0.75;
		finalColor = texture(gradientmap, vec2( gradientChoice, 1.0-dot(lookupColor, vec3( 0.2126, 0.7152, 0.0722))) );
	}
}
