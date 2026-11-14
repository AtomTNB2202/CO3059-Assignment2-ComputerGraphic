#version 330 core
in vec2 uv;
uniform sampler2D objectIds;
uniform int selectedId;
out vec4 fragmentColor;
int idAt(ivec2 point)
{
    ivec2 size = textureSize(objectIds, 0);
    if (any(lessThan(point, ivec2(0))) || any(greaterThanEqual(point, size))) return 0;
    ivec3 bytes = ivec3(round(texelFetch(objectIds, point, 0).rgb * 255.0));
    return bytes.r | (bytes.g << 8) | (bytes.b << 16);
}
void main()
{
    ivec2 point = ivec2(uv * vec2(textureSize(objectIds, 0)));
    if (idAt(point) == selectedId) discard;
    for (int y=-2; y<=2; ++y)
        for (int x=-2; x<=2; ++x)
            if (idAt(point + ivec2(x,y)) == selectedId)
            {
                fragmentColor = vec4(1.0, 0.57, 0.13, 1.0);
                return;
            }
    discard;
}
