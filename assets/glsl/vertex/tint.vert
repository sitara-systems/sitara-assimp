#version 150

uniform mat4 ciModelViewProjection;
uniform mat4 ciModelView;
uniform mat3 ciNormalMatrix;
uniform vec4 tintColor;
uniform float tintRatio;

in vec4 ciPosition;
in vec4 ciColor;
in vec3 ciNormal;

out VertexData {
	vec4 position;
	vec3 normal;
	vec4 color;
} vertexOut;

void main(void) {
    gl_Position = ciModelViewProjection * ciPosition;
    vertexOut.position = ciModelView * ciPosition;
    vertexOut.normal = ciNormalMatrix * ciNormal;
    vertexOut.color = mix(ciColor, tintColor, tintRatio);
}
