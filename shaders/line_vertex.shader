#version 410 core

layout (location = 0) in vec3 aPos;

uniform mat4 u_View;
uniform mat4 u_Proj;
uniform bool u_border;

out vec4 v_Color;

void main()
{
    gl_Position = u_Proj * u_View * vec4(aPos, 1.0);

    if (u_border)
    {
        v_Color = vec4(0.7, 0.7, 0.8, 0.6);
    }
    else
    {
        v_Color = vec4(0.0);
    }
}
