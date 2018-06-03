#version 330 core

// Reference
// Chapter 17: Real-Time Physically Based Deformation Using Transform Feedback from OpenGL Insights
// Muhammad Mobeen Movania and Lin Feng

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 PositionOld;

uniform samplerBuffer tex_position;
uniform samplerBuffer tex_prev_position;

uniform int texsize_x = 20;
uniform int texsize_y = 20;
uniform float kd = 0.00;
uniform float ks = 0.75;
uniform float mass = 0.02;
uniform float inv_cloth_size = 0.05;
uniform float deltaTime;

out vec3 Position0;
out vec3 PositionOld0;

ivec2 neighbors[12] = ivec2[](
    ivec2( 1,  0), // 0
    ivec2( 0, -1),
    ivec2(-1,  0),
    ivec2( 0,  1),
    ivec2( 1, -1),
    ivec2(-1, -1),
    ivec2(-1,  1),
    ivec2( 1,  1), // 7
    ivec2( 2,  0),
    ivec2( 0, -2),
    ivec2(-2,  0),
    ivec2( 0,  2) // 11
);

ivec2 getNextNeighbor(int k)
{
    return neighbors[k];
}

void main()
{
    // init
    float dt     = deltaTime * 2;
    float m      = mass;
    vec3 pos     = Position;
    vec3 pos_old = PositionOld;
    vec3 vel     = (pos - pos_old) * dt;
    
    
    int index = gl_VertexID;
    int ix = index % texsize_x;
    int iy = index / texsize_x;
    
    if (index == 0 || index == (texsize_x - 1) ||
        index == (texsize_x * texsize_y) - 1 ||
        index == (texsize_x * texsize_y) - texsize_x)
    {
        m = 0.0;
    }

    vec3 F = vec3(0.0, -0.001, 0.0);
    for (int k = 0; k < 12; ++k) {
        ivec2 coord = getNextNeighbor(k);
        int i = coord.x;
        int j = coord.y;
        
        if (((iy + i) < 0) || ((iy + i) > (texsize_y-1))) {
            continue;
        }
        if (((ix + j) < 0) || ((ix + j) > (texsize_x-1))) {
            continue;
        }
        
        int index_neigh = (iy + i) * texsize_x + ix + j;
        vec3 p2 = texelFetch(tex_position, index_neigh).xyz;
        vec3 p2_last = texelFetch(tex_prev_position, index_neigh).xyz;
        
        vec2 coord_neigh = vec2(ix + j, iy + i);
        float rest_length = length(vec2(coord) * inv_cloth_size);
        vec3 v2 = (p2 - p2_last) / dt;
        vec3 deltaP = pos - p2;
        vec3 deltaV = vel - v2;
        float dist = length(deltaP);
        float leftTerm = -ks * (dist - rest_length);
        float rightTerm = kd * (dot(deltaV, deltaP) / dist);
        vec3 springForce = (leftTerm + rightTerm) * normalize(deltaP);
        F += springForce;
    }
    
    vec3 acc = vec3(0);
    if (m != 0)
    {
        acc = F / m;
    }
    
    vec3 tmp = pos;
    pos = pos * 2.0 - pos_old + acc* dt * dt;
    pos_old = tmp;
    pos.y = max(-2, pos.y);
    
    PositionOld0 = pos_old;
    Position0 = pos;
}
