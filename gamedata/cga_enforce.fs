#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform float mirrorMode;
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
	vec2 adjTexCoord;
	// mirror mode 0 = No mirror
	// mirror mode 1 = No mirror (but also titles are turned off in game)
	// mirror mode 2 = horiz mirror
	// mirror mode 3 = circle vert mirror
	if (mirrorMode < 1.5) {
		adjTexCoord = fragTexCoord;
	} else if (mirrorMode < 2.5) {
		adjTexCoord = vec2( abs(fragTexCoord.x - 0.5) + 0.5, fragTexCoord.y );
	} else if (mirrorMode < 3.5) {
		//float d = length( vec2(fragTexCoord.x, fragTexCoord.y * (size.y/size.x)) - vec2( 0.5, 0.5) );
		float aspect = size.x/size.y;
		float d = length( fragTexCoord * vec2( aspect, 1.0) - vec2( 0.5*aspect, 0.5) );

		d = abs(cos( d * 3 ));

		vec2 texCoordFlip =  vec2( fragTexCoord.x, 1.0-fragTexCoord.y );
		adjTexCoord = mix( fragTexCoord, texCoordFlip, step( 0.5, d));
		//adjTexCoord = texCoordFlip;
	} else if (mirrorMode < 4.5) {
		adjTexCoord = vec2( fragTexCoord.x, abs(fragTexCoord.y - 0.5) + 0.5 );
	} else if (mirrorMode < 5.5) {
		adjTexCoord = vec2( abs(fragTexCoord.x - 0.5) + 0.5, abs(fragTexCoord.y - 0.5) + 0.5 );
	}

    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, adjTexCoord);
    vec4 mask = texture(mtlmask, adjTexCoord);
    
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
