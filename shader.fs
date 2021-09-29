#version 130

out vec4 outputColor;

void main()
{
	float Value = gl_FragCoord.z;
	outputColor = mix(vec4(1.0f, 0.0f, 0.0f, 1.0f), vec4(0.0f, 0.0f, 1.0f, 1.0f), Value);

}
