#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 litColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D texture2;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 maskColor = texture(texture2, fragTexCoord);
    
    // NOTE: Implement here your fragment shader code
    
    vec3 maskedLightColor = mix( vec3(0.5, 0.5, 0.5),litColor, maskColor.r );
    finalColor.rgb = (texelColor.rgb*colDiffuse.rgb) * (maskedLightColor * 2.0);
    finalColor.a = 1.0;
    //finalColor = texelColor*colDiffuse;
    //finalColor = (texelColor*colDiffuse) * maskColor;

    //finalColor = vec4( finalColor.b, 1.0, finalColor.b * 0.5, finalColor.a );
}

