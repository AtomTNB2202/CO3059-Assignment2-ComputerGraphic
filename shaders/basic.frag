#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec2 uv;
uniform vec3 objectColor;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;
uniform float shininess;
uniform int renderMode;
uniform sampler2D imageTexture;
out vec4 fragmentColor;
void main()
{
    vec3 base=objectColor;
    if(renderMode==3) base*=texture(imageTexture,uv).rgb;
    if(renderMode==0 || renderMode==4)
    {
        fragmentColor=vec4(base,1.0);
        return;
    }
    vec3 n=normalize(worldNormal);
    if(!gl_FrontFacing) n=-n;
    vec3 light=normalize(lightPosition-worldPosition);
    vec3 eye=normalize(cameraPosition-worldPosition);
    float diffuse=max(dot(n,light),0.0);
    float specular=diffuse>0.0 ? pow(max(dot(eye,reflect(-light,n)),0.0),shininess) : 0.0;
    fragmentColor=vec4(base*(0.22+0.78*diffuse)+vec3(0.35)*specular,1.0);
}
