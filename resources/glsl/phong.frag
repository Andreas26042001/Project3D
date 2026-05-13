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
uniform int beamLightCount;
uniform vec3 beamLightStarts[2];
uniform vec3 beamLightEnds[2];
uniform vec3 beamLightColors[2];
uniform float beamLightStrengths[2];
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
// The light-space transforms are also needed in the fragment shader: the normal-offset
// bias (RTR4 §7.4 p. 238, Holbert) shifts the receiver along the surface normal in
// *world* space and must therefore reproject through the light's matrix per fragment.
// GLSL links uniforms by name across stages, so these are the same variables already
// declared and set in phong.vert — no extra C++ uniform plumbing required.
uniform mat4 lightSpaceMatrices[4];
uniform mat4 dynamicLightSpaceMatrix;
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

float calculateShadow(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir, mat4 lightMat, sampler2DShadow mapTex) {
    // === Normal-offset bias (Holbert) — RTR4 §7.4 p. 238 and Figure 7.24 (p. 250) ===
    // Shift the receiver location along the surface normal in *world* space, by an
    // amount proportional to sin(angle(N, L)). This moves the test sample onto a
    // "virtual surface" slightly above the true receiver, which kills self-shadowing
    // without needing the depth bias to be inflated (the inflated-bias trick causes
    // Peter Panning, RTR4 §7.4 p. 238). Both the depth *and* the UVs of the test point
    // shift together, which is what makes this bias more effective than slope-scale
    // alone (Figure 7.24, right panel).
    //
    // Implementation note: the shift is computed in world space (FragPos + N*offset),
    // but we equivalently apply it in clip space as `fragPosLightSpace + lightMat *
    // vec4(N, 0) * offset` to avoid a second full matrix multiply per fragment.
    float cosTheta = clamp(dot(normal, lightDir), 0.0, 1.0);
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
    const float normalOffsetScale = 0.012; // world-space meters at fully grazing angles
    vec4 shifted = fragPosLightSpace + lightMat * vec4(normal, 0.0) * (normalOffsetScale * sinTheta);

    // Standard shadow-map projection (RTR4 §7.4 "Shadow Maps", p. 234): divide by w
    // for perspective lights then remap from clip [-1, 1] to texture [0, 1].
    vec3 projCoords = shifted.xyz / max(shifted.w, 0.0001);
    projCoords = projCoords * 0.5 + 0.5;
    // Anything past the light's far plane is treated as fully lit; the borders are
    // already handled by the depth texture's GL_CLAMP_TO_BORDER + white border.
    if (projCoords.z > 1.0) {
        return 0.0;
    }

    // === Receiver plane depth bias -- RTR4 7.5 p. 249-250 ===
    // (Isidoro 2006 / Schuler / Tuft / Dou et al.) Use the screen-space gradient of
    // the shadow-projected depth to estimate the plane of the receiver triangle, then
    // correct every PCF tap so that all samples lie on that same plane rather than on
    // the constant-depth plane that the central tap implicitly assumes. This is the
    // exact per-pixel form of slope-scale bias and replaces the manual cos(theta)
    // residual we used before. Mathematically:
    //   given a sample shifted by (du, dv) on the shadow UV, the receiver plane's
    //   depth at that location is approximately z + du*dz/du + dv*dz/dv.
    // The 2x2 Jacobian to extract (dz/du, dz/dv) from screen-space derivatives is
    // standard (Isidoro 2006). RTR4 7.5 p. 250 notes that this assumes nearby samples
    // lie on the same plane, which fails at silhouette discontinuities, hence the
    // magnitude clamp below.
    vec3 duvz_dx = dFdx(projCoords);
    vec3 duvz_dy = dFdy(projCoords);
    vec2 receiverPlaneBias = vec2(0.0);
    float det = duvz_dx.x * duvz_dy.y - duvz_dx.y * duvz_dy.x;
    if (abs(det) > 1.0e-8) {
        receiverPlaneBias = vec2(
            duvz_dy.y * duvz_dx.z - duvz_dx.y * duvz_dy.z,
            duvz_dx.x * duvz_dy.z - duvz_dy.x * duvz_dx.z
        ) / det;
        // Cap magnitude to avoid the gradient blowing up across silhouette edges
        // (RTR4 §7.5 p. 250: receiver-plane bias must be combined with other biases
        // because it breaks at concavities and discontinuities).
        float biasLen = length(receiverPlaneBias);
        const float kMaxRPB = 0.01;
        if (biasLen > kMaxRPB) {
            receiverPlaneBias *= kMaxRPB / biasLen;
        }
    }

    // Residual constant bias for numerical safety (slope-scale is already taken care
    // of by glPolygonOffset on the depth-write side, RTR4 §7.4 p. 236-237).
    float refDepth = projCoords.z - 0.00002;

    // PCF kernel: rotated Poisson disk with per-tap receiver-plane depth correction.
    // RTR4 §7.5 Fig. 7.23 (p. 249) for the rotation; per-tap depth correction is the
    // application of the receiver-plane bias derived above.
    vec2 texelSize = 1.0 / vec2(textureSize(mapTex, 0));
    float angle = pcfHash(gl_FragCoord.xy) * 6.2831853;
    float c = cos(angle);
    float s = sin(angle);
    mat2 rot = mat2(c, -s, s, c);

    const float filterRadius = 1.8;
    float lit = 0.0;
    for (int i = 0; i < 12; ++i) {
        vec2 offset = rot * POISSON_DISK_12[i] * texelSize * filterRadius;
        float perTapDepth = refDepth + dot(offset, receiverPlaneBias);
        lit += texture(mapTex, vec3(projCoords.xy + offset, perTapDepth));
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
            // The 4th argument is the per-light clip transform, needed inside
            // calculateShadow() to project the world-space normal offset (Holbert).
            if (i == 0) localShadow = calculateShadow(FragPosLightSpace[0], norm, topDir, lightSpaceMatrices[0], shadowMaps[0]);
            if (i == 1) localShadow = calculateShadow(FragPosLightSpace[1], norm, topDir, lightSpaceMatrices[1], shadowMaps[1]);
            if (i == 2) localShadow = calculateShadow(FragPosLightSpace[2], norm, topDir, lightSpaceMatrices[2], shadowMaps[2]);
            if (i == 3) localShadow = calculateShadow(FragPosLightSpace[3], norm, topDir, lightSpaceMatrices[3], shadowMaps[3]);
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
            placedShadow = calculateShadow(FragPosDynamicLightSpace, norm, placedDir, dynamicLightSpaceMatrix, dynamicShadowMap);
        }
        topLight += placedCornerLikeStrength * placedAttenuation * placedCornerLikeFlicker * 0.10 * material.ambient * placedCornerLikeColor;
        topLight += placedCornerLikeStrength * placedAttenuation * (1.0 - 0.82 * placedShadow) * placedCornerLikeFlicker * 0.95 * placedDiff * sampledDiffuse * placedCornerLikeColor;
    }

    // Sources lineaires bleues: rayon principal du canon + rayons devies par les
    // pyramides bleues. La contribution d'une lumiere segment est approchee en cherchant
    // le point le plus proche du segment par rapport au fragment (cf. Heidrich/Seidel
    // 1999), puis en l'eclairant comme une source ponctuelle a cette position.
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
        float beamAttenuation = 1.0 / (1.0 + cornerAttLinear * beamDistance + cornerAttQuadratic * beamDistance * beamDistance);
        float beamDiff = pow(max(dot(norm, beamDir), 0.0), 2.8);
        topLight += beamLightStrengths[i] * beamAttenuation * 0.10 * material.ambient * beamLightColors[i];
        topLight += beamLightStrengths[i] * beamAttenuation * 0.95 * beamDiff * sampledDiffuse * beamLightColors[i];
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
