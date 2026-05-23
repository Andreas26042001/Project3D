#version 330 core
out vec4 FragColor;

in vec3 v_frag_coord;
in vec3 v_normal;

uniform vec3 u_view_pos;
uniform samplerCube cubemapSampler;
uniform float refractionIndice;

void main() {
    vec3 N = normalize(v_normal);
    vec3 V = normalize(u_view_pos - v_frag_coord);
    vec3 R = reflect(-V, N);
    vec3 reflColor = texture(cubemapSampler, R).rgb;
    float ratio = 1.0 / refractionIndice;
    vec3 T = refract(-V, N, ratio);
    vec3 refrColor = length(T) > 0.001 ? texture(cubemapSampler, T).rgb : reflColor;
    float fresnel = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 env = mix(refrColor, reflColor, fresnel);
    env *= 0.88;
    env = mix(env, vec3(0.30, 0.60, 1.00), 0.22);
    float alpha = 0.88 + 0.12 * fresnel;
    FragColor = vec4(env, alpha);
}
