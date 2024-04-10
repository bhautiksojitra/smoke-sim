#version 150

in vec4 vPosition;
in vec2 texPosition;
out vec2 tex;

uniform vec2 Window;
uniform vec2 Texture;
void main()
{
  gl_Position = vPosition;
  tex = texPosition;

  //vec2 ul = Window/Texture;
  //vec2 xy = vec2((vPosition.x+0.5) / Window.x, (vPosition.y-0.5) / Window.y);
  
  //gl_Position = vec4(xy.x * 2 - 1, xy.y * 2 - 1, 0, 1);
  //tex = xy * ul;
  
}
