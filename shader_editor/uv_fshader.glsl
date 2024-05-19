// Per-fragment interpolated values from the vertex shader

//Since normals are fixed for a given face of the cube in this example, fN per-fragment interpolation yields fixed values. Per-fragment interpolation of fL and fV however gives smoothly varying values through faces.
#version 410

in float fSelected;
in vec2 onEdgeDetector;
in vec4 wcolor;

uniform float isFakeDisplay;
out vec4 fcolor;

const vec4 edgeColor = vec4(1.0, 0.5, 0.0, 1.0);
const vec4 selectedColor = vec4(1.0, 0.8, 0.0, .6);

void main() 
{ 
    if(isFakeDisplay >= .9){
        fcolor = wcolor;
        return;
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
            fcolor.a = 1.0;
            return;
        }

        fcolor = ((1.0 - fSelected) * (vec4(1.0, 0.0, 1.0, .3))) + (fSelected * selectedColor);

} 
