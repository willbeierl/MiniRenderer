#version 450 core
in vec3 vNormalWS;
in vec3 vPosWS;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTex0;
uniform vec3 uLightDirWS;   // direction of light (world)
uniform vec3 uCameraPosWS;

void main()
{
    vec3 albedo = texture(uTex0, vUV).rgb;

    vec3 N = normalize(vNormalWS);
    vec3 L = normalize(-uLightDirWS);

    // diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * albedo;

    // ambient
    vec3 ambient = 0.15 * albedo;

    // specular (Blinn-Phong)
    vec3 V = normalize(uCameraPosWS - vPosWS);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0); // shininess
    vec3 specular = vec3(0.25) * spec;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}
