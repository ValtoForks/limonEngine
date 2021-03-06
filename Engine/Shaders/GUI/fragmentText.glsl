#version 330

uniform sampler2D GUISampler;
uniform vec3 inColor;

in vec2 vs_fs_textureCoord;
out vec4 color;

void main(void)
{
		color = vec4(inColor, texture(GUISampler, vs_fs_textureCoord).r);
}
