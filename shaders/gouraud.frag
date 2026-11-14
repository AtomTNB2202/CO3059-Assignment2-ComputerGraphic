#version 330 core
in vec3 frontColor;
in vec3 backColor;
out vec4 fragmentColor;
void main() { fragmentColor=vec4(gl_FrontFacing ? frontColor : backColor,1.0); }
