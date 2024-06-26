// Per-fragment interpolated values from the vertex shader

//Since normals are fixed for a given face of the cube in this example, fN per-fragment interpolation yields fixed values. Per-fragment interpolation of fL and fV however gives smoothly varying values through faces.
#version 410

in  vec3 fN;
in  vec3 fL;
in  vec3 fV;
in float fSelected;
in vec2 onEdgeDetector;

uniform vec4 diffuseProperty;
uniform vec4 ambientProperty;
uniform vec4 specularProperty;
uniform float shininess;
uniform float isFakeDisplay;


uniform sampler2D texture1;
uniform int hasTexture;
in vec4 UV;

out vec4 fcolor;

const vec4 edgeColor = vec4(1.0, 0.5, 0.0, 1.0);
const vec4 selectedColor = vec4(1.0, 0.6, 0.0, 1.0);

void main() 
{ 
    if(isFakeDisplay >= .9){
        fcolor = vec4(fN, 1.0);
        return;
    }
        // Normalize the input lighting vectors
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L = normalize(fL);

        vec3 H = normalize( L + V );
        vec2 ux = UV.xy;
        ux.y *= -1;
        vec4 textureColor = texture( texture1, ux ); 
        float textureProportion = hasTexture * .8;
        
        // remap properties with texture

        vec4 ambientProperty1 = ambientProperty * ((1.0 - textureProportion) + textureColor * textureProportion);
        vec4 diffuseProperty1 = diffuseProperty * ((1.0 - textureProportion) + textureColor * textureProportion);
        vec4 specularProperty1 = specularProperty * ((1.0 - textureProportion) + textureColor * textureProportion);

        vec4 ambient = ambientProperty1;

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd*diffuseProperty1;
        
        float Ks = pow(max(dot(N, H), 0.0), shininess);
        vec4 specular = Ks*specularProperty1;

        // discard the specular highlight if the light's behind the vertex
        if( dot(L, N) < 0.0 ) {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }


        //check if fragment is on an edge
        float limit = 0.02;
        // onEdgeDetector is the interpolated value from the vertex shader
        // check if the interpolated value is limit away from any edge
        // on a (0,0) (0,1) (1,0) triangle
        bool is_edge = false;
        if(onEdgeDetector.x < limit || onEdgeDetector.y < limit || onEdgeDetector.x + onEdgeDetector.y > 1.0 - limit){
            is_edge = true;
        }
        if(is_edge){
            // color the edges a shade of orange
            fcolor = edgeColor;
            return;
        }

        float fs = fSelected * .8;
        fcolor = ((1.0 - fs) * (ambient + diffuse + specular)) + (fs * selectedColor);
        fcolor.a = 1.0;
        // fcolor = texture( texture1, UV.xy );

} 
