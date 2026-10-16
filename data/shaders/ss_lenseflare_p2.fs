#version 400

in vec2 uv;

uniform sampler2D texScene;

uniform int flareSamples;
uniform float flareDispersal;
uniform float flareHaloWidth;
// NOTE: a flareChromaDispertion uniform + chromatic sampling used to live
// here, but the sampling was commented out, so the uniform optimized away
// and its transmit could never succeed. Deleted outright; restoring the
// chromatic lookup is a visual task, not a reopen of this declaration.

layout (location = 0) out vec4 FragColor;

vec3 textureDistorted(
	in sampler2D tex,
	in vec2 sampleCenter,
	in vec2 sampleVector
) {
        return texture(texScene, sampleCenter + sampleVector).rgb;
}

void main()
{
        vec2 imageCenter = vec2(0.5);
	vec2 sampleVector = (imageCenter - uv) * flareDispersal;
	vec2 haloVector = normalize(sampleVector) * flareHaloWidth;

        vec3 result = textureDistorted(texScene, uv + haloVector, haloVector).rgb;

	for (int i = 0; i < flareSamples; ++i) {
                vec2 offset = sampleVector * float(i);
		result += textureDistorted(texScene, uv + offset, offset).rgb;
	}

  	FragColor = vec4(result, 1.0f);
}
