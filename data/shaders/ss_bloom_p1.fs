#version 400

in vec2 uv;

uniform sampler2D texScene;

layout (location = 0) out vec4 FragColor;


void main()
{
        // Downscale
        vec3 texel = texture(texScene, uv).rgb;

        // desaturate
	/*const float AvgLumR = 0.5;
        const float AvgLumG = 0.5;
        const float AvgLumB = 0.5;
        const vec3 LumCoeff = vec3(0.2125, 0.7154, 0.0721);

        vec3 AvgLumin = vec3(AvgLumR, AvgLumG, AvgLumB);
        vec3 intensity = vec3(dot(texel, LumCoeff));
	vec3 satColor = mix(intensity, texel, 0.5);*/

	float grey = max(texel.r, max(texel.g, texel.b));


	// HDR Phase 3: scene-referred threshold for linear HDR. Diffuse
	// rarely exceeds 1.0 pre-tonemap; only highlights do. The LDR-era
	// low threshold bloomed mid-tones, so the knee now starts at 1.0
	// and smoothsteps to full response at 2.0 instead of hard-clipping.
	// Output stays desaturated grey like before; ss_bloom_p2 adds it
	// onto the scene pre-tonemap.
	vec3 color = vec3(smoothstep(1.0, 2.0, grey));

        // Highlight boost
        //color = 1.0f-min(0.25+(1.0f-color), 1.0f);


  	FragColor = vec4( color, 1.0f );
}
