#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec4 aColor;
layout (location = 2) in vec3 position;
layout (location = 3) in vec3 heading;
// layout (location = 4) in vec4 aInstanceColor;

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform float u_scale;
uniform bool u_color;
uniform bool u_2D;

out vec4 v_Color;
out vec2 v_UV;

void main()
{
    // transform particle to view space
    vec4 viewPos = u_View * vec4(position, 1.0);

    // offset vertex in view space so it always faces the camera
    viewPos.xy += aPos * u_scale;

    // project to clip space
    gl_Position = u_Proj * viewPos;

    // obtain UVs for raytracing (already normalized to [-1, 1] in aPos)
    v_UV = aPos.xy;
    
    if (u_color)
    {
        if (u_2D)
        {
            float angle = atan(heading.y, heading.x);
            float PI = 3.1415926535897932384626433832795;
            float r = sin(angle) * 0.5 + 0.5;
            float g = sin(angle + 2.0 * PI / 3.0) * 0.5 + 0.5;
            float b = sin(angle + 4.0 * PI / 3.0) * 0.5 + 0.5;
            v_Color = vec4(r, g, b, 1.0);
        }
        else
        {
            v_Color = vec4(heading * 0.5 + 0.5, 1.0);
        }
    }
    else
    {
        v_Color = vec4(1.0); // Default color
    }
}
