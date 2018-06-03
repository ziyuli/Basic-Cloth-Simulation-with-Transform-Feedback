#version 330 core

layout (location = 0) in vec3 aPosition;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_PointSize = 5.0;
    gl_Position = projection * view * vec4(aPosition,1.0);
}
