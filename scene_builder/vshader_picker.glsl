#version 410

in vec4 vPosition;

uniform mat4 Projection;
uniform mat4 Transform;
uniform vec4 pColor; // unique color for picking

void main() {
    gl_Position = Projection * Transform * vPosition;
}
