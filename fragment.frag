#version 330

in vec2 uv;

uniform sampler2D tex;

out vec4 outColor;

void main()
{
  outColor = texture(tex, uv);
  outColor.g = outColor.r;
  outColor.b = outColor.r;
}
