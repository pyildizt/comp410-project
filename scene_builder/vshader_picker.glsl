#version 410

in vec4 position;

uniform mat4 PickerProjectionPointer;
uniform mat4 PickerTransformPointer;

void main() {
    gl_Position = PickerProjectionPointer * PickerTransformPointer * position;
}
