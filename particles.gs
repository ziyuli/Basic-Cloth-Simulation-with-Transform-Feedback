#version 330 core

layout (points) in;
layout (points, max_vertices = 1) out;

uniform float deltaTime;

in vec3 Position0[];
in vec3 PositionOld0[];

out vec3 Position1;
out vec3 PositionOld1;

void main()
{
    Position1 = Position0[0];
    PositionOld1 = PositionOld0[0];
    
    EmitVertex();
    EndPrimitive();
}
