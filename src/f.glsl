#version 150

out vec4 color;
in vec2 tex;
uniform sampler2D densityTexture;

void main() 
{ 
	vec3 density = texture(densityTexture, tex).xyz;
	color = vec4(density, 1.0); 		
}
