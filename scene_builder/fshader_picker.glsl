#version 410

out vec4 FragColor;

uniform vec4 PickerFakeColorPointer;

void main() {
    FragColor = PickerFakeColorPointer;
}
