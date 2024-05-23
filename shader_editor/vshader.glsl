#version 410
in vec4 vPosition;
in vec4 vNormal;
in float vSelected;
in float vindex;
uniform mat4 Projection;
uniform mat4 Transform;
uniform vec4 diffuseProperty;
uniform vec4 ambientProperty;
uniform vec4 specularProperty;
uniform float shininess;
uniform int isEdge;
uniform float isFakeDisplay;

in vec4 UVpos;
out vec4 UV;


in vec4 vColor;
out vec4 normal;
out vec4 wcolor;

vec4 inter;

// output values that will be interpretated per-fragment
out  vec3 fN;
out  vec3 fV;
out  vec3 fL;
out float fSelected;
out vec2 onEdgeDetector;

uniform vec4 lightPosition;

void main()
{
    UV = UVpos;
    if(isFakeDisplay <= .9){
    // Transform vertex position into camera (eye) coordinates
    vec3 pos = (Transform * vPosition).xyz;
    vec4 norm = vNormal;
    norm.w = 0.0;
    fN = (Transform * norm).xyz; // normal direction in camera coordinates

    fV = -pos; //viewer direction in camera coordinates

    fL = lightPosition.xyz; // light direction

    if( lightPosition.w != 0.0 ) {
        fL = lightPosition.xyz - pos;  //point light source
    }

    fSelected = vSelected;
    
    if(isEdge == 1){
    int idx = ((int(vindex)) % 3);
    if(idx == 0){
    onEdgeDetector = vec2(0.0,0.0);
    }else if(idx == 1){
    onEdgeDetector = vec2(1.0,0.0);
    }else if(idx == 2){
    onEdgeDetector = vec2(0.0,1.0);
    }
    }else{
    onEdgeDetector = vec2(1.0,1.0);
    }
    }else{
    fN = vNormal.xyz;
    }
    gl_Position = Projection * Transform * vPosition;
}
