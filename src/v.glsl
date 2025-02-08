#version 150

in vec4 vPosition;
in vec2 texPosition;
out vec2 tex;

void main()
{
  gl_Position = vPosition;
  tex = texPosition;
}
