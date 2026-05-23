#version 330 core
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 color;
    float ambient;
    float diffuse;
    float specular;
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec4 FragPosDynamicLightSpace;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform vec3 ceilingLightPos;
uniform vec3 ceilingLightColor;
uniform float ceilingLightStrength;
uniform float ceilingAttLinear;
uniform float ceilingAttQuadratic;
uniform bool dynamicLightActive;
uniform vec3 dynamicLightPos;
uniform vec3 dynamicLightColor;
uniform float dynamicLightStrength;
uniform int beamLightCount;
uniform vec3 beamLightStarts[2];
uniform vec3 beamLightEnds[2];
uniform vec3 beamLightColors[2];
uniform float beamLightStrengths[2];
uniform sampler2D diffuseMap;
uniform bool useTexture;
uniform vec2 uvScale;
uniform sampler2D shadowMap;
uniform bool useShadowMap;
uniform sampler2D dynamicShadowMap;
uniform bool dynamicShadowActive;
uniform mat4 lightSpaceMatrix;
uniform mat4 dynamicLightSpaceMatrix;
uniform bool isGroundPass;
uniform int pillarShadowCount;
uniform vec3 pillarShadowCenters[64];
uniform float pillarShadowRadius;
uniform float pillarShadowStrength;
uniform samplerCube environmentMap;
uniform bool useEnvironmentReflection;
uniform float reflectionStrength;

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, mat4 lightMat, sampler2D mapTex) {
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    const float normalOffsetScale = 0.012;
    vec4 shifted = fragPosLightSpace + lightMat * vec4(normal, 0.0) * (normalOffsetScale * sinTheta);

    vec3 projCoords = shifted.xyz / max(shifted.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 ||
        projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z - 0.00002;
    float closestDepth = texture(mapTex, projCoords.xy).r;
    return currentDepth > closestDepth ? 1.0 : 0.0;
}

void main() {
    vec3 sampledDiffuse = material.diffuse;
    if (useTexture) {
        sampledDiffuse = texture(diffuseMap, TexCoords * uvScale).rgb;
    }

    vec3 ambient = light.ambient * material.ambient * light.color;
    ambient += vec3(0.028, 0.026, 0.022) * material.ambient;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * sampledDiffuse * light.color;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular * light.color;

    vec3 topLight = vec3(0.0);
    float shadowFactor = 0.0;
    {
        vec3 toCeiling = ceilingLightPos - FragPos;
        float ceilingDistance = length(toCeiling);
        vec3 ceilingDir = normalize(toCeiling);
        float ceilingAttenuation = 1.0 / (1.0 + ceilingAttLinear * ceilingDistance + ceilingAttQuadratic * ceilingDistance * ceilingDistance);
        float ceilingDiff = pow(max(dot(norm, ceilingDir), 0.0), 2.8);
        float localShadow = 0.0;
        if (useShadowMap) {
            localShadow = calculateShadow(FragPosLightSpace, norm, ceilingDir, lightSpaceMatrix, shadowMap);
        }
        shadowFactor = localShadow;
        topLight += ceilingLightStrength * ceilingAttenuation * 0.10 * material.ambient * ceilingLightColor;
        topLight += ceilingLightStrength * ceilingAttenuation * (1.0 - 0.72 * localShadow) * 0.95 * ceilingDiff * sampledDiffuse * ceilingLightColor;
    }

    if (dynamicLightActive) {
        vec3 toDynamic = dynamicLightPos - FragPos;
        float dynamicDistance = length(toDynamic);
        vec3 dynamicDir = normalize(toDynamic);
        float dynamicAttenuation = 1.0 / (1.0 + ceilingAttLinear * dynamicDistance + ceilingAttQuadratic * dynamicDistance * dynamicDistance);
        float dynamicDiff = pow(max(dot(norm, dynamicDir), 0.0), 2.8);
        float dynamicShadow = 0.0;
        if (dynamicShadowActive) {
            dynamicShadow = calculateShadow(FragPosDynamicLightSpace, norm, dynamicDir, dynamicLightSpaceMatrix, dynamicShadowMap);
        }
        topLight += dynamicLightStrength * dynamicAttenuation * 0.10 * material.ambient * dynamicLightColor;
        topLight += dynamicLightStrength * dynamicAttenuation * (1.0 - 0.82 * dynamicShadow) * 0.95 * dynamicDiff * sampledDiffuse * dynamicLightColor;
    }

    for (int i = 0; i < 2; ++i) {
        if (i >= beamLightCount) {
            break;
        }
        vec3 ab = beamLightEnds[i] - beamLightStarts[i];
        float lenSq = max(dot(ab, ab), 0.0001);
        float t = clamp(dot(FragPos - beamLightStarts[i], ab) / lenSq, 0.0, 1.0);
        vec3 closestOnBeam = beamLightStarts[i] + t * ab;
        vec3 beamToFrag = closestOnBeam - FragPos;
        float beamDistance = length(beamToFrag);
        vec3 beamDir = beamDistance > 0.0001 ? beamToFrag / beamDistance : vec3(0.0, 1.0, 0.0);
        float beamAttenuation = 1.0 / (1.0 + ceilingAttLinear * beamDistance + ceilingAttQuadratic * beamDistance * beamDistance);
        float beamDiff = pow(max(dot(norm, beamDir), 0.0), 2.8);
        topLight += beamLightStrengths[i] * beamAttenuation * 0.10 * material.ambient * beamLightColors[i];
        topLight += beamLightStrengths[i] * beamAttenuation * 0.95 * beamDiff * sampledDiffuse * beamLightColors[i];
    }

    vec3 result = ambient + diffuse + specular + topLight;
    if (useShadowMap) {
        result *= (1.0 - 0.35 * shadowFactor);
    }

    if (isGroundPass && pillarShadowCount > 0) {
        float maxShadow = 0.0;
        for (int i = 0; i < 64; ++i) {
            if (i >= pillarShadowCount) {
                break;
            }
            vec2 toCenter = FragPos.xz - pillarShadowCenters[i].xz;
            float dist = length(toCenter);
            float localShadow = 1.0 - smoothstep(0.0, pillarShadowRadius, dist);
            maxShadow = max(maxShadow, localShadow);
        }
        result *= (1.0 - pillarShadowStrength * maxShadow);
    }

    if (useEnvironmentReflection) {
        vec3 R = reflect(-viewDir, norm);
        vec3 envColor = texture(environmentMap, R).rgb;
        result = mix(result, envColor, reflectionStrength);
    }

    FragColor = vec4(result, 1.0);
}
