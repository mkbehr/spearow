#version 330

in vec2 pos;

out vec2 uv;

void main()
{
  uv = pos;
  gl_Position = vec4((pos.x * 2) - 1, 1 - (pos.y * 2),
                     0.0, 1.0);
}
