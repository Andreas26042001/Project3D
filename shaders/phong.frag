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
in vec4 FragPosLightSpace[4];
in vec4 FragPosDynamicLightSpace;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform vec3 topCornerLights[4];
uniform vec3 topLightColor;
uniform float topLightStrength;
uniform float topCornerFlicker[4];
uniform float cornerAttLinear;
uniform float cornerAttQuadratic;
uniform bool placedCornerLikeActive;
uniform vec3 placedCornerLikePos;
uniform vec3 placedCornerLikeColor;
uniform float placedCornerLikeFlicker;
uniform float placedCornerLikeStrength;
uniform bool beamLightActive;
uniform vec3 beamLightStart;
uniform vec3 beamLightEnd;
uniform vec3 beamLightColor;
uniform float beamLightStrength;
uniform sampler2D diffuseMap;
uniform bool useTexture;
uniform vec2 uvScale;
// Hardware-PCF shadow samplers (Real-Time Rendering 4e, §7.5 "Percentage-Closer Filtering",
// p. 248-249). With GL_COMPARE_REF_TO_TEXTURE + GL_LINEAR set on the depth texture,
// each texture() call performs a 2x2 bilinear depth comparison in a single instruction.
uniform sampler2DShadow shadowMaps[4];
uniform bool useShadowMap;
uniform sampler2DShadow dynamicShadowMap;
uniform bool dynamicShadowActive;
uniform bool isGroundPass;
uniform int pillarShadowCount;
uniform vec3 pillarShadowCenters[64];
uniform float pillarShadowRadius;
uniform float pillarShadowStrength;

// 12-tap Poisson disk used for PCF — RTR4 §7.5 Figure 7.23 (p. 249). The samples are
// spread so that no two are too close, eliminating the visible grid pattern of a regular
// NxN kernel. Combined with a per-pixel random rotation (below) the residual aliasing is
// turned into noise rather than structured bands.
const vec2 POISSON_DISK_12[12] = vec2[](
    vec2(-0.326,-0.406), vec2(-0.840,-0.074), vec2(-0.696, 0.457), vec2(-0.203, 0.621),
    vec2( 0.962,-0.195), vec2( 0.473,-0.480), vec2( 0.519, 0.767), vec2( 0.185,-0.893),
    vec2( 0.507, 0.064), vec2( 0.896, 0.412), vec2(-0.322,-0.933), vec2(-0.792,-0.598)
);

// Cheap per-pixel hash used to rotate the Poisson disk so that the sampling pattern
// changes from pixel to pixel — RTR4 §7.5 p. 249: "Such artifacts can be avoided by
// randomly rotating the sample distribution around its center, which turns aliasing
// into noise."
float pcfHash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, sampler2DShadow mapTex) {
    // Standard shadow-map projection (RTR4 §7.4 "Shadow Maps", p. 234): divide by w
    // for perspective lights then remap from clip [-1, 1] to texture [0, 1].
    vec3 projCoords = fragPosLightSpace.xyz / max(fragPosLightSpace.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;
    // Anything past the light's far plane is treated as fully lit; the borders are
    // already handled by the depth texture's GL_CLAMP_TO_BORDER + white border.
    if (projCoords.z > 1.0) {
        return 0.0;
    }

    // Residual constant + slope-scale bias on top of the canonical glPolygonOffset
    // bias applied on the C++ side (RTR4 §7.4 p. 236-237). The book recommends a
    // small clamped slope-scale to handle the remaining numerical-precision cases
    // (p. 237: "Slope scale bias is also often clamped at some maximum...").
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float bias = clamp(0.0008 * (1.0 - cosTheta), 0.00005, 0.0015);
    float refDepth = projCoords.z - bias;

    // PCF kernel: rotate the Poisson disk by a per-pixel random angle, sample 12 taps,
    // each tap doing hardware 2x2 bilinear comparison (RTR4 §7.5, Fig. 7.23, p. 249).
    vec2 texelSize = 1.0 / vec2(textureSize(mapTex, 0));
    float angle = pcfHash(gl_FragCoord.xy) * 6.2831853;
    float c = cos(angle);
    float s = sin(angle);
    mat2 rot = mat2(c, -s, s, c);

    const float filterRadius = 1.8;
    float lit = 0.0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = rot * POISSON_DISK_12[i] * texelSize * filterRadius;
        lit += texture(mapTex, vec3(projCoords.xy + offset, refDepth));
    }
    lit *= (1.0 / 12.0);
    return 1.0 - lit;
}

void main() {
    vec3 sampledDiffuse = material.diffuse;
    if (useTexture) {
        sampledDiffuse = texture(diffuseMap, TexCoords * uvScale).rgb;
    }

    // Ambient
    vec3 ambient = light.ambient * material.ambient * light.color;
    // Global cave fill to avoid full-black scenes.
    ambient += vec3(0.028, 0.026, 0.022) * material.ambient;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * diff * sampledDiffuse * light.color;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular * spec * material.specular * light.color;

    // Four corner lights from above.
    vec3 topLight = vec3(0.0);
    float shadowFactor = 0.0;
    for (int i = 0; i < 4; ++i) {
        vec3 toTop = topCornerLights[i] - FragPos;
        float topDistance = length(toTop);
        vec3 topDir = normalize(toTop);
        float topAttenuation = 1.0 / (1.0 + cornerAttLinear * topDistance + cornerAttQuadratic * topDistance * topDistance);
        // Wider cone so corner torches actually light the room.
        float topDiff = pow(max(dot(norm, topDir), 0.0), 2.8);
        float flicker = topCornerFlicker[i];
        float localShadow = 0.0;
        if (useShadowMap) {
            // One shadow map per corner light (RTR4 §7.4, p. 234: a separate light view
            // is needed for each light source). The hardware-PCF sampler returns a
            // pre-filtered 0..1 visibility ratio used directly as the shadow factor.
            if (i == 0) localShadow = calculateShadow(FragPosLightSpace[0], norm, topDir, shadowMaps[0]);
            if (i == 1) localShadow = calculateShadow(FragPosLightSpace[1], norm, topDir, shadowMaps[1]);
            if (i == 2) localShadow = calculateShadow(FragPosLightSpace[2], norm, topDir, shadowMaps[2]);
            if (i == 3) localShadow = calculateShadow(FragPosLightSpace[3], norm, topDir, shadowMaps[3]);
        }
        shadowFactor = max(shadowFactor, localShadow);
        topLight += topLightStrength * topAttenuation * flicker * 0.10 * material.ambient * topLightColor;
        topLight += topLightStrength * topAttenuation * (1.0 - 0.72 * localShadow) * flicker * 0.95 * topDiff * sampledDiffuse * topLightColor;
    }

    // Placed light using the same shading model as corner sources.
    if (placedCornerLikeActive) {
        vec3 placedToLight = placedCornerLikePos - FragPos;
        float placedDistance = length(placedToLight);
        vec3 placedDir = normalize(placedToLight);
        float placedAttenuation = 1.0 / (1.0 + cornerAttLinear * placedDistance + cornerAttQuadratic * placedDistance * placedDistance);
        float placedDiff = pow(max(dot(norm, placedDir), 0.0), 2.8);
        float placedShadow = 0.0;
        if (dynamicShadowActive) {
            placedShadow = calculateShadow(FragPosDynamicLightSpace, norm, placedDir, dynamicShadowMap);
        }
        topLight += placedCornerLikeStrength * placedAttenuation * placedCornerLikeFlicker * 0.10 * material.ambient * placedCornerLikeColor;
        topLight += placedCornerLikeStrength * placedAttenuation * (1.0 - 0.82 * placedShadow) * placedCornerLikeFlicker * 0.95 * placedDiff * sampledDiffuse * placedCornerLikeColor;
    }

    // Source lineaire bleue: rayon emis par le canon du pilier central. La contribution
    // d'une lumiere segment est approchee en cherchant le point le plus proche du segment
    // par rapport au fragment (cf. Heidrich/Seidel 1999), puis en l'eclairant comme une
    // source ponctuelle a cette position.
    if (beamLightActive) {
        vec3 ab = beamLightEnd - beamLightStart;
        float lenSq = max(dot(ab, ab), 0.0001);
        float t = clamp(dot(FragPos - beamLightStart, ab) / lenSq, 0.0, 1.0);
        vec3 closestOnBeam = beamLightStart + t * ab;
        vec3 beamToFrag = closestOnBeam - FragPos;
        float beamDistance = length(beamToFrag);
        vec3 beamDir = beamDistance > 0.0001 ? beamToFrag / beamDistance : vec3(0.0, 1.0, 0.0);
        float beamAttenuation = 1.0 / (1.0 + cornerAttLinear * beamDistance + cornerAttQuadratic * beamDistance * beamDistance);
        float beamDiff = pow(max(dot(norm, beamDir), 0.0), 2.8);
        topLight += beamLightStrength * beamAttenuation * 0.10 * material.ambient * beamLightColor;
        topLight += beamLightStrength * beamAttenuation * 0.95 * beamDiff * sampledDiffuse * beamLightColor;
    }

    vec3 result = ambient + diffuse + specular + topLight;
    // Make shadow map impact clearly visible on lit surfaces.
    if (useShadowMap) {
        result *= (1.0 - 0.35 * shadowFactor);
    }

    // Cheap analytic "drop shadow" under each pillar applied to the ground only.
    // This is the classical projective-shadow trick from RTR4 §7.1 ("Planar Shadows",
    // p. 225) / Figure 7.6 p. 229 — an approximation that anchors objects to the ground
    // even when the shadow-map sample density is insufficient at oblique angles
    // (Wanger's observation, RTR4 §7.1 p. 225: "it is usually better to have an inaccurate
    // shadow than none at all").
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

    FragColor = vec4(result, 1.0);
}
