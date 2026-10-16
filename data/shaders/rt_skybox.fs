#version 400

in vec3 pos;

uniform samplerCube texCube;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragThreshold;

// Phase 4 unified fog: same FOG_COLOR/DENSITY as the lit shaders. The
// sky has no view depth, so it blends to the fog color at the horizon
// where distant fogged ground meets the sky. Zenith keeps the cubemap.
// sRGB inputs are linearized (display gamma-encodes once); the authored
// color is stored linear so the horizon displays as designed.
// Horizon seam note: the blend exponent (8.0) and the full-fog
// below-horizon (dir.y < 0.0) are eye-tuned against the ground fog in
// rt_ground_n.fs (same values adopted in rt_basic.fs). Retune here if
// FOG_COLOR/FOG_DENSITY change there.
const vec3 FOG_COLOR = pow(vec3(0.796,0.918,0.949)*0.8, vec3(2.2));

vec3 srgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }

void main()
{
    // No brightness gain: the cubemap is displayed through the same
    // linear-to-gamma chain as every other surface (a 1.4x gain here
    // double-lifts the sky after the display gamma).
    vec3 sky = srgbToLinear(texture(texCube, pos).rgb);
    vec3 dir = normalize(pos);
    float h = clamp(dir.y, 0.0, 1.0);
    float fogFactor = pow(1.0 - h, 8.0);
    if (dir.y < 0.0)
        fogFactor = 1.0;
    vec3 col = mix(sky, FOG_COLOR, fogFactor);
    FragColor = vec4(col, 1.0);
    FragThreshold = vec4(0.0, 0.0, 0.0, 1.0);
}
