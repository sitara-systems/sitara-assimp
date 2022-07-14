#version 150

uniform vec4 ambientColor;
uniform vec4 specularColor;
uniform vec4 diffuseColor;
uniform vec4 emissionColor;
uniform float Ns;

in VertexData	{
	vec4 position;
	vec3 normal;
	vec4 color;
} vertexIn;

out vec4 fragColor;

void main() {
	// lighting calculations
    vec3 normalDirection = normalize(vertexIn.normal);
    vec3 lightDirection = vec3(0, 0, 1);

	// Calculate coefficients.
    float specularCoefficient = 0.0;
	float lambert = max(dot(normalDirection, lightDirection), 0.0);

    if(lambert > 0.0) {
        vec3 viewDirection = normalize(-vertexIn.position.xyz);
    	vec3 halfDirection = normalize(lightDirection + viewDirection);
        specularCoefficient = max(dot(normalDirection, halfDirection), 0.0);
        specularCoefficient = pow(specularCoefficient, Ns);
    }

	fragColor = vec4(ambientColor + vec4(vec3(lambert), 1.0)*diffuseColor + specularCoefficient*specularColor);
}
