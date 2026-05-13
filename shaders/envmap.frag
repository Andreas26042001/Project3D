#version 330 core
out vec4 FragColor;

in vec3 WorldPos;
in vec3 WorldNormal;

struct Light {
    vec3 position;
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
};

uniform samplerCube skybox;
uniform vec3 cameraPos;
uniform Light light;
uniform vec3 topCornerLights[4];
uniform vec3 topLightColor;
uniform float topLightStrength;
uniform float reflectivity;
uniform float refractiveIndex;
uniform vec3 baseColor;
uniform float baseColorStrength;
uniform float alpha;

void main() {
    vec3 I = normalize(WorldPos - cameraPos);
    vec3 N = normalize(WorldNormal);

    vec3 reflectedDir = reflect(I, N);
    vec3 refractedDir = refract(I, N, 1.0 / refractiveIndex);

    vec3 reflectedColor = texture(skybox, reflectedDir).rgb;
    vec3 refractedColor = texture(skybox, refractedDir).rgb;
    vec3 envColor = mix(refractedColor, reflectedColor, reflectivity);

    vec3 lightDir = normalize(light.position - WorldPos);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    vec3 reflectDir = reflect(-lightDir, N);

    float diff = max(dot(N, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambientLit = light.ambient * baseColor * light.color;
    vec3 diffuseLit = light.diffuse * diff * baseColor * light.color;
    vec3 specularLit = light.specular * spec * light.color;
    vec3 topLit = vec3(0.0);
    for (int i = 0; i < 4; ++i) {
        vec3 topDir = normalize(topCornerLights[i] - WorldPos);
        float topDiff = max(dot(N, topDir), 0.0);
        topLit += topLightStrength * 0.14 * baseColor * topLightColor;
        topLit += topLightStrength * 0.35 * topDiff * baseColor * topLightColor;
    }
    vec3 localLighting = ambientLit + diffuseLit + specularLit + topLit;

    vec3 finalColor = mix(envColor, baseColor, baseColorStrength) + localLighting;

    FragColor = vec4(finalColor, alpha);
}
