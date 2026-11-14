#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec2 texCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 objectColor;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform float shininess;
out vec3 frontColor;
out vec3 backColor;
vec3 shade(vec3 n,vec3 world)
{
    vec3 light=normalize(lightPosition-world);
    vec3 eye=normalize(cameraPosition-world);
    float diffuse=max(dot(n,light),0.0);
    float specular=diffuse>0.0 ? pow(max(dot(eye,reflect(-light,n)),0.0),shininess) : 0.0;
    return objectColor*(0.22+0.78*diffuse)+vec3(0.35)*specular;
}
void main()
{
    vec4 world=model*vec4(position,1.0);
    vec3 n=normalize(mat3(transpose(inverse(model)))*normal);
    frontColor=shade(n,world.xyz);
    backColor=shade(-n,world.xyz);
    gl_Position=projection*view*world;
}
