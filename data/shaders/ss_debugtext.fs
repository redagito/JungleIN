#version 400

in vec2 uv;

uniform sampler2D texAtlas;

layout (location = 0) out vec4 FragColor;

// Debug text overlay: white glyphs, atlas red channel as coverage.
// Fully transparent texels are discarded so blending only touches ink.
void main()
{
  float coverage = texture(texAtlas, uv).r;
  if (coverage < 0.5)
    discard;
  FragColor = vec4(1.0f, 1.0f, 1.0f, coverage);
}
