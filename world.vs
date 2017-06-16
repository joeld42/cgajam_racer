#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 litColor;

// NOTE: Add here your custom variables 


void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    vec3 N = normalize((modelMatrix*vec4(vertexNormal, 0.0)).rgb);

    vec3 lightDir0 = normalize( vec3( -0.4, 1.0, 0.1) );
    vec3 lightColor0 = vec3( 0.6, 1.0, 1.0 ) * 0.5;
    vec3 lightAmt0 = dot(lightDir0, N) * lightColor0;

	vec3 lightDir1 = normalize( vec3( 1.0, .5, -0.2) );
    vec3 lightColor1 = vec3( 1.0, 0.6, 1.0 ) * 0.2;
    vec3 lightAmt1 = dot(lightDir1, N) * lightColor1;


    litColor = lightAmt0 + lightAmt1 +vec3(0.2,0.2,0.2);
    
    // Calculate final vertex position
    gl_Position = mvpMatrix*vec4(vertexPosition, 1.0);
}