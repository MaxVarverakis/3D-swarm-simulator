#version 410 core

layout (location = 0) out vec4 FragColor;

in vec4 v_Color;
in vec2 v_UV;

const float transition_fuzz = 0.1;

void main()
{
    // Calculate (squared) distance from the center of the circle (0,0)
    float dist = dot(v_UV, v_UV); // save the sqrt comparing squares instead (r_local^2 = 1^2 = 1)
    if (dist > 1.0) discard; // crop to circle

    // 3D sphere normal
    float z = sqrt(1.0 - dist);
    vec3 normal = vec3(v_UV, z);

    // Lighting calculations
    vec3 lightDir = normalize(vec3(0.5, 0.8, 1.0)); // Directional light
    float diff = max(dot(normal, lightDir), 0.1); // Diffuse shading with minimum ambient light

    // float alpha = smoothstep(0.0, transition_fuzz, 1.0 - dist) * v_Color.a;
    FragColor = vec4(diff * v_Color.rgb, v_Color.a);
}
