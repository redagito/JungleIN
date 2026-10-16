#version 400

in vec2 position;
in vec2 texcoord;

out vec2 uv;

void main()
{
  gl_Position = vec4(position, 0.0f, 1.0f);
  uv = texcoord;
}
