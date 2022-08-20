#version 150

in VertexData	{
	vec4 position;
	vec3 normal;
	vec4 color;
} vertexIn;

out vec4 fragColor;

void main(void) {
    const vec3 lightDirection = vec3(0, 0, 1);
    vec3 normalDirection = normalize(vertexIn.normal);
    float lambert = max(0.0, dot(normalDirection,lightDirection));
    fragColor = vec4(1)*vertexIn.color*vec4(vec3(lambert), 1.0);
}
