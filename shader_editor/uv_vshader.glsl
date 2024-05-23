#version 410
in vec4 vPosition;
in vec4 uniqueColors;
in float vSelected;
in float uSelected;
in float vindex;
uniform mat4 Projection;
uniform mat4 Transform;
uniform float isFakeDisplay;


vec4 inter;

// output values that will be interpretated per-fragment
out vec4 wcolor;
out float fSelected;
out vec2 onEdgeDetector;
void main()
{
    if(isFakeDisplay <= .9){
    // Transform vertex position into camera (eye) coordinates
    fSelected = uSelected;
    
    if(true){
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
        onEdgeDetector = vec2(999.0, 999.0);
        wcolor = uniqueColors;
    }

    if(vSelected < .5){ 
        gl_Position = vec4(-9999, -9999, 0, 1);
        return;
    } 
    inter = vPosition;
    inter.xy = vPosition.xy * 2.0 - 1.0;
    gl_Position = Projection * inter;
    
}
