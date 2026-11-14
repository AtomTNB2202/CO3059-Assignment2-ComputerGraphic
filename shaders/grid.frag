#version 330 core
in vec3 worldPosition;
uniform vec3 cameraPosition;
uniform float fadeDistance;
uniform int frontView;
out vec4 fragmentColor;

float gridLine(vec2 point, float spacing)
{
    vec2 coordinates = point / spacing;
    vec2 width = max(fwidth(coordinates), vec2(0.00001));
    vec2 distanceToLine = abs(fract(coordinates - 0.5) - 0.5) / width;
    float coverage = 1.0 - min(min(distanceToLine.x, distanceToLine.y), 1.0);
    // Fade sub-pixel cells instead of aliasing near the horizon.
    return coverage * (1.0 - smoothstep(0.2, 0.8, max(width.x, width.y)));
}

void main()
{
    vec2 point = frontView == 1 ? worldPosition.xy : worldPosition.xz;
    float minor = gridLine(point, 1.0);
    float major = gridLine(point, 5.0);
    vec3 color = vec3(0.53);
    float alpha = max(minor * 0.25, major * 0.45);
    vec2 width = max(fwidth(point), vec2(0.00001));
    float xAxis = 1.0 - smoothstep(0.0, width.y * 1.5, abs(point.y));
    float otherAxis = 1.0 - smoothstep(0.0, width.x * 1.5, abs(point.x));
    if (xAxis > 0.0)
    {
        color = mix(color, vec3(0.70, 0.24, 0.23), xAxis);
        alpha = max(alpha, xAxis * 0.8);
    }
    if (otherAxis > 0.0)
    {
        vec3 axisColor = frontView == 1 ? vec3(0.35, 0.65, 0.25) : vec3(0.25, 0.45, 0.80);
        color = mix(color, axisColor, otherAxis);
        alpha = max(alpha, otherAxis * 0.8);
    }
    alpha *= 1.0 - smoothstep(fadeDistance * 0.4, fadeDistance, length(worldPosition - cameraPosition));
    if (alpha < 0.005) discard;
    fragmentColor = vec4(color, alpha);
}
