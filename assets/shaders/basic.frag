#version 450 core
in vec3 vColor;
in vec2 vUV;

out vec4 FragColor;

uniform float uTime;
uniform float uPulse; // 0..1
uniform sampler2D uTex0;

void main()
{
    float wave = 0.5 + 0.5 * sin(uTime * 2.0);
    float brightness = mix(0.1, 1.9, wave) * uPulse;

    vec3 tex = texture(uTex0, vUV).rgb;

    FragColor = vec4(tex * brightness, 1.0);
}
