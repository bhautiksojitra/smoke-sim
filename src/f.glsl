#version 150

out vec4 color;
in vec2 tex;
uniform sampler2D velocityTexture;
uniform sampler2D densityTexture;
uniform float deltaTime;

void main() 
{ 
	//vec2 velocity = texture(ourTexture, tex).xy;

	//vec2 velocity = texture(velocityTexture, tex).xy;
	vec3 density = texture(densityTexture, tex).xyz;

	//vec2 offset = velocity * deltaTime;
	//vec2 newUV = tex.xy + offset;

	//float aD = texture(densityTexture, newUV).r;

	//if(density < 0.01)
	//{
		
	//}
	//else
	//{
	//color = vec4(density + 0.3, density + 0.3 ,  0.3 + density, 1.0);

	//}

	color = vec4(density, 1.0); 
		
	//if(density < 0.000001)
	//{
	//color = vec4(1,1,1,1);
	//}
	 

	//vec2 offset = vec2(cos(deltaTime), sin(deltaTime));
	//vec2 newTexCoord = tex + offset * 0.1;
	
	//vec3 df = texture(ourTexture, newTexCoord).xyz;
	//if(deltaTime > 10)
	//{
	//	color.rgb = vec3(0,0,0);
	//}
	//else
	//{
	//color.rgb = df;
	//}
	
	//color.a = 1;

	//vec2 offset = velocity * deltaTime;
	//vec2 newUV = gl_FragCoord.xy - offset;
	//float advectedDensity = texture(ourTexture, newUV).r;
    //color = vec4(advectedDensity, 0, advectedDensity, 1.0);
}
