#version 400

in vec2 fTexcoord;
in vec3 fNormal;
in vec4 fShadowcoord;
in vec4 fShadowcoordCascade;
in vec3 fLightDir;
in float vViewDepth;

uniform sampler2D texDiffuse;
uniform sampler2D texAlpha;
// Per-material alpha-test threshold (replaces the hardcoded 0.8).
uniform float alphaCutoff;
// Stale-unit guards: rt_basic is shared by every mesh, so a sampler left
// over from another mesh's draw would sample garbage. When a map is not
// bound the shader never reads its sampler.
uniform int useDiffuse;
uniform int useAlpha;
uniform vec3 diffuseColor;

uniform sampler2DShadow texShadow;
uniform int cascadeShadow;
uniform sampler2DShadow texShadowCascade;
// Texel size is derived per lookup via textureSize(), so no resolution
// uniforms are needed (a declared-but-unused uniform would be optimized
// out and fail the fail-fast uniform transmit).
uniform int shadowQuality;
uniform vec3 lightIntensity;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragThreshold;

float shadowLookup(in sampler2DShadow tex, in vec4 coord, in vec2 texelOffset, in float bias)
{
    vec2 texelSize = 1.0 / vec2(textureSize(tex, 0));
    vec4 c = vec4(coord.xy + texelOffset * texelSize * coord.w,
                  coord.z - bias * coord.w,
                  coord.w);
    return textureProj(tex, c);
}

float sampleShadowMap(in sampler2DShadow tex, in vec4 coord, in float bias)
{
    if (shadowQuality == 1)
    {
        float sum = 0.0;
        for (int i = -1; i <= 1; ++i)
            for (int j = -1; j <= 1; ++j)
                sum += shadowLookup(tex, coord, vec2(float(i), float(j)), bias);
        return sum / 9.0;
    }
    return shadowLookup(tex, coord, vec2(0.0, 0.0), bias);
}

bool insideFrustum(in vec3 proj)
{
    return all(greaterThanEqual(proj.xy, vec2(0.0, 0.0))) &&
           all(lessThanEqual(proj.xy, vec2(1.0, 1.0)));
}

float compShadow(in float nDotL)
{
    float bias = max(0.005 * (1.0 - max(nDotL, 0.0)), 0.0005);

    if (fShadowcoord.w <= 0.0)
        return 1.0;
    vec3 projInner = fShadowcoord.xyz / fShadowcoord.w;
    if (insideFrustum(projInner))
        return sampleShadowMap(texShadow, fShadowcoord, bias);

    if (cascadeShadow == 1)
    {
        if (fShadowcoordCascade.w <= 0.0)
            return 1.0;
        vec3 projCascade = fShadowcoordCascade.xyz / fShadowcoordCascade.w;
        if (insideFrustum(projCascade))
            return sampleShadowMap(texShadowCascade, fShadowcoordCascade, bias);
    }

    // Outside every shadow frustum: lit, never dark.
    return 1.0;
}

// sRGB inputs (8-bit textures, no hardware decode) must be linearized
// before lighting; ss_display gamma-encodes once at the end. Skipping
// this decode leaves mid-tones ~1.5x too bright after the display gamma.
vec3 srgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }

// Phase 4 unified fog: ground_n values adopted for every lit shader.
// The authored color is sRGB for direct display, so it is stored linear
// here (display gamma-encodes the mixed result). Formula reads vViewDepth
// (view-space forward distance), never window coordinates.
const float LOG2 = 1.442695;
const vec3 FOG_COLOR = pow(vec3(0.796,0.918,0.949)*0.8, vec3(2.2));
const float FOG_DENSITY = 0.006;

vec3 compFog(vec3 finalColor)
{
    float z = vViewDepth;
    float fogFactor = exp2( -FOG_DENSITY *
                           FOG_DENSITY *
                           z *
                           z *
                           LOG2 );
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    return mix(FOG_COLOR, finalColor, fogFactor);
}

const float Ka = 0.1;
const float Kd = 1.0;
//const float Ks = 0.0;
//const float Shininess = 10.0;

// No specular calculation since we don't have good looking normal textures.
vec3 illumination(vec3 texel)
{
    // Double-sided lighting: flip the normal for back faces instead of
    // scaling the result with an ad-hoc coefficient.
    vec3 n = normalize(fNormal);
    if (!gl_FrontFacing)
        n = -n;
    vec3 s = normalize(-fLightDir);
    float nDotL = max(dot(s, n), 0.0);
    float shadow = compShadow(nDotL);

    // Ka/Kd shape the diffuse lobe driven by the light intensity.
    vec3 lightCol = lightIntensity * Kd * nDotL;

    return texel * (
                Ka +
                shadow * lightCol);
                //+ Ks * pow( max( dot(h,n), 0.0 ), Shininess)));
}

void main()
{

    vec3 texelDiffuse = diffuseColor;
    if (useDiffuse == 1)
        texelDiffuse = texture(texDiffuse, fTexcoord).rgb;
    texelDiffuse = srgbToLinear(texelDiffuse);

    if (useAlpha == 1)
    {
        float texelAlpha = texture(texAlpha, fTexcoord).r;
        if (texelAlpha < alphaCutoff) discard;
    }

    // Linear workflow: inputs decoded above, lighting in linear, the
    // display pass (ss_display) applies gamma once at the end.
    FragColor = vec4(compFog(illumination(texelDiffuse)), 1.0);
    FragThreshold = vec4(0.0,0.0,0.0,1.0);
}
