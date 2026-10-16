#version 400

in vec3 fPosition;
in vec3 fNormal;
in vec4 fShadowcoord;
in vec4 fShadowcoordCascade;
in vec3 fLightDir;
in vec3 vViewPos;
in float vViewDepth;

// Normal mapping
in vec3 fTangent;
in vec3 fBinormal;
uniform sampler2D texNormalEarth;
uniform sampler2D texNormalShatter;
uniform sampler2D texNormalMoss;

// Shadow mapping
uniform sampler2DShadow texShadow;
uniform int cascadeShadow;
uniform sampler2DShadow texShadowCascade;
// Texel size is derived per lookup via textureSize(), so no resolution
// uniforms are needed (a declared-but-unused uniform would be optimized
// out and fail the fail-fast uniform transmit).
uniform int shadowQuality;
uniform vec3 lightIntensity;

uniform float tileSize;
uniform float heightRange;
uniform sampler2D texEarth;
uniform sampler2D texShatter;
uniform sampler2D texMoss;

const float TILE_RATIO = 80.0;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 FragThreshold;

vec3 computeNormal(vec3 texel)
{
    vec3 normalTex = normalize(texel * 2.0 - 1.0);
    mat3 tsb = mat3(normalize(fTangent), normalize(fBinormal), normalize(fNormal));

    return normalize(tsb * normalTex);
}

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

float computeShadow(in float nDotL)
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

// sRGB albedo inputs (8-bit textures, no hardware decode) must be
// linearized before lighting; ss_display gamma-encodes once at the end.
// Normal maps are data, never decoded.
vec3 srgbToLinear(vec3 c) { return pow(c, vec3(2.2)); }

// Phase 4 unified fog: shared color/density with rt_basic (ground_n
// values adopted). The authored color is sRGB for direct display, so it
// is stored linear here. Formula reads vViewDepth, never window
// coordinates.
const float LOG2 = 1.442695;
const vec3 FOG_COLOR = pow(vec3(0.796,0.918,0.949)*0.8, vec3(2.2));
const float FOG_DENSITY = 0.006;

vec3 computeFog(vec3 finalColor)
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
const float Kd = 0.7;
const float Ks = 0.4;
// Narrow lobe: 4.0 sprayed white specular across every bumpy pixel at
// grazing angles (the chalky ground). 24.0 keeps highlights tight.
const float Shininess = 24.0;
// Per-band specular response: vegetation is near-matte, rock shines.
// The ladder in main() blends this mask exactly like the albedo.
const float SPEC_SHATTER = 0.35;
const float SPEC_EARTH = 0.08;
const float SPEC_MOSS = 0.12;

vec3 illumination(vec3 texel, vec3 normal, float specScale)
{
    // Double-sided lighting: flip the normal for back faces.
    vec3 n = gl_FrontFacing ? normal : -normal;
    vec3 s = normalize(-fLightDir);
    float nDotL = max(dot(s, n), 0.0);
    float shadow = computeShadow(nDotL);

    // [OPT] Early exit
    if(shadow < 0.05) return texel * Ka;

    vec3 v = normalize(-vViewPos);
    vec3 h = normalize(v + s);

    // Light intensity plumbing: defaults to (1,1,1), legacy look unchanged.
    vec3 diffuseCol = lightIntensity * Kd * max(dot(s, n), 0.0);
    vec3 specCol = lightIntensity * Ks * specScale *
        pow(max(dot(h,n), 0.0), Shininess);

    vec3 color = texel * (
                    // Ambient
                    Ka +
                    // Diffuse
                    shadow * diffuseCol
                ) + // Specular
                    shadow * specCol;

    return color;
}

void main()
{
    vec2 texcoords = fPosition.xz/(tileSize/TILE_RATIO);
    float hStep = heightRange / 4.0;

    vec3 texel;
    vec3 texelNormal;
    float specMask;
    float amount;

    if(fPosition.y < hStep)
    {
        texel = texture(texShatter, texcoords).rgb;
        texelNormal = texture(texNormalShatter, texcoords).rgb;
        specMask = SPEC_SHATTER;
    }
    else if(fPosition.y < 2*hStep)
    {
        amount = (fPosition.y - hStep) / hStep;
        texel = mix(texture(texShatter, texcoords),
                    texture(texEarth, texcoords),
                    amount).rgb;
        texelNormal = mix(texture(texNormalShatter, texcoords),
                    texture(texNormalEarth, texcoords),
                    amount).rgb;
        specMask = mix(SPEC_SHATTER, SPEC_EARTH, amount);
    }
    else if(fPosition.y < 3*hStep)
    {
        amount = (fPosition.y - 2*hStep) / hStep;
        texel = mix(texture(texEarth, texcoords),
                    texture(texMoss, texcoords),
                    amount).rgb;
        texelNormal = mix(texture(texNormalEarth, texcoords),
                    texture(texNormalMoss, texcoords),
                    amount).rgb;
        specMask = mix(SPEC_EARTH, SPEC_MOSS, amount);
    }
    else
    {
        texel = texture(texMoss, texcoords).rgb;
        texelNormal = texture(texNormalMoss, texcoords).rgb;
        specMask = SPEC_MOSS;
    }

    // Linear workflow: decode the sRGB albedo to linear (normal map
    // stays raw data); the display pass applies gamma once at the end.
    texel = srgbToLinear(texel);
    FragColor = vec4(
                    computeFog(
                        illumination(
                            texel,
                            computeNormal(texelNormal),
                            specMask
                        )
                    )
                , 1.0);

    FragThreshold = vec4(0.0,0.0,0.0,1.0);

    //FragColor = texture(texDiffuse, fTexcoord);
}
