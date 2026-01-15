#version 450 core
in vec3 vWorldPos;

uniform vec3  uLightPosWS;
uniform float uFarPlane;

void main()
{
    float dist = length(vWorldPos - uLightPosWS);
    gl_FragDepth = dist / uFarPlane;   // store linear distance in depth
}
