#version 450 core
in vec3 vNormalWS;
in vec3 vPosWS;
in vec2 vUV;

out vec4 FragColor;

uniform sampler2D uTex0;
uniform int uUseTexture;
uniform vec3 uCameraPosWS;

uniform samplerCube uShadowCube;
uniform float uFarPlane;

// Point light:
uniform vec3 uLightPosWS;
uniform vec3 uLightColor;

// Debug: draw the light cube as a solid emissive color
uniform int uIsLight;


float ShadowPoint(vec3 fragPosWS, vec3 lightPosWS)
{
    vec3 toLight = fragPosWS - lightPosWS;
    float current = length(toLight);

    // bias: scale with angle to reduce acne on grazing angles
    vec3 N = normalize(vNormalWS);
    vec3 L = normalize(lightPosWS - fragPosWS);
    float bias = max(0.08 * (1.0 - dot(N, L)), 0.02);

    // PCF sampling
    float shadow = 0.0;
    int samples = 20;

    // disk radius grows with distance (helps stabilize softness)
    float diskRadius = 0.01 + (current / uFarPlane) * 0.03;

    vec3 offsets[20] = vec3[](
        vec3( 1, 1, 1), vec3( 1,-1, 1), vec3(-1,-1, 1), vec3(-1, 1, 1),
        vec3( 1, 1,-1), vec3( 1,-1,-1), vec3(-1,-1,-1), vec3(-1, 1,-1),
        vec3( 1, 1, 0), vec3( 1,-1, 0), vec3(-1,-1, 0), vec3(-1, 1, 0),
        vec3( 1, 0, 1), vec3(-1, 0, 1), vec3( 1, 0,-1), vec3(-1, 0,-1),
        vec3( 0, 1, 1), vec3( 0,-1, 1), vec3( 0, 1,-1), vec3( 0,-1,-1)
    );

    for (int i = 0; i < samples; i++)
    {
        float closest = texture(uShadowCube, toLight + offsets[i] * diskRadius).r * uFarPlane;
        if (current - bias > closest)
            shadow += 1.0;
    }

    shadow /= float(samples);

    // return visibility: 1 = lit, 0 = shadowed
    return 1.0 - shadow;
}


void main()
{
    // Draw the light gizmo cube as a flat color (no lighting)
    if (uIsLight != 0)
    {
        FragColor = vec4(uLightColor, 1.0);
        return;
    }

    vec3 albedo = (uUseTexture != 0)
        ? texture(uTex0, vUV).rgb
        : vec3(0.7); // plain gray helps see lighting

    vec3 N = normalize(vNormalWS);

    // Point-light vector
    vec3 Lvec = uLightPosWS - vPosWS;
    float dist = length(Lvec);
    vec3 L = Lvec / max(dist, 0.0001);

    // Attenuation (tweakable constants)
    float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);

    // Diffuse
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * albedo * uLightColor;

    // Ambient
    vec3 ambient = 0.12 * albedo;

    // Specular (Blinn-Phong)
    vec3 V = normalize(uCameraPosWS - vPosWS);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);
    vec3 specular = vec3(0.25) * spec * uLightColor;

    float vis = ShadowPoint(vPosWS, uLightPosWS);

    vec3 color = ambient + (diffuse + specular) * att * vis;
    FragColor = vec4(color, 1.0);
}
