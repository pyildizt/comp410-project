#version 410

in   vec4 vPosition;
in   vec4 vNormal;
in   vec4 uvs;

// output values that will be interpretated per-fragment
out  vec3 fN;
out  vec3 fV;
out  vec3 fL;
out vec2 texc;

uniform int HasTexture;

uniform mat4 Transform;
uniform vec4 LightPosition;
uniform mat4 Projection;


void main()
{
    // Transform vertex position into camera (eye) coordinates
    vec3 pos = (Transform * vPosition).xyz;
    
    fN = (Transform * vNormal).xyz; // normal direction in camera coordinates

    fV = -pos; //viewer direction in camera coordinates

    fL = LightPosition.xyz; // light direction

    if( LightPosition.w != 0.0 ) {
        fL = LightPosition.xyz - pos;  //point light source
    }

    gl_Position = Projection * Transform * vPosition;
    texc = uvs.xy;
}
