#version 330 core
in vec4 vPosition;
uniform mat4 Projection;

out vec2 TexCoord;

void main() {
    gl_Position = Projection * vPosition;
    // map -1,1 to 0,1 for texCoord
    vec2 tx = vPosition.xy;
    tx.y *= -1;
    TexCoord = (tx + vec2(1.0, 1.0)) / 2.0;
}