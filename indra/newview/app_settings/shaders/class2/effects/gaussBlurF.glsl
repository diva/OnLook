
#extension GL_ARB_texture_rectangle : enable

uniform sampler2DRect RenderTexture;
uniform int horizontalPass;

vec2 offset = vec2( 1.3846153846, 3.2307692308 );
vec3 weight = vec3( 0.2270270270, 0.3162162162, 0.0702702703 );
 
void main(void)
{
	vec4 color = texture2DRect(RenderTexture, gl_TexCoord[0].st)*weight.x;
	
	
	if(horizontalPass == 1)
	{
		color += weight.y * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x+offset.x,gl_TexCoord[0].y));
		color += weight.y * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x-offset.x,gl_TexCoord[0].y));
		color += weight.z * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x+offset.y,gl_TexCoord[0].y));
		color += weight.z * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x-offset.y,gl_TexCoord[0].y));
		
	}
	else
	{
		color += weight.y * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x,gl_TexCoord[0].y+offset.x));
		color += weight.y * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x,gl_TexCoord[0].y-offset.x));
		color += weight.z * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x,gl_TexCoord[0].y+offset.y));
		color += weight.z * texture2DRect(RenderTexture, vec2(gl_TexCoord[0].x,gl_TexCoord[0].y-offset.y));
	}
	gl_FragColor = color;
}
