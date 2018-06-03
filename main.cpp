//
//  main.cpp
//  Cloth
//
//  Created by Ziyu Li on 4/26/18.
//  Copyright © 2018 Ziyu Li. All rights reserved.
//

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"
#include "camera.h"

#include <iostream>
#include <random>

void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 480;
const unsigned int SCR_HEIGHT = 480;

Camera camera(glm::vec3(0.0f, 0.0f, 0.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

int curr_particles_vbo = 0;
int curr_particles_tfo = 1;
bool first = true;

int main(int argc, const char * argv[]) {
    
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow * window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Cloth", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glewExperimental = GL_TRUE;
    glewInit();
    
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    const GLchar * Varyings[2];
    Varyings[0] = "Position1";
    Varyings[1] = "PositionOld1";

    Shader particlesShader("/Users/Lee/Documents/GLAdvTutorial/ParticleSystem/particles.vs",
                           "/Users/Lee/Documents/GLAdvTutorial/ParticleSystem/particles.fs",
                           "/Users/Lee/Documents/GLAdvTutorial/ParticleSystem/particles.gs",
                           2, Varyings, GL_SEPARATE_ATTRIBS);
    
    Shader pointShader("/Users/Lee/Documents/GLAdvTutorial/ParticleSystem/point.vs",
                       "/Users/Lee/Documents/GLAdvTutorial/ParticleSystem/point.fs");
    
    
    particlesShader.use();
    
    // Cloth Index
    std::vector<unsigned int> cloth_index;
    for (unsigned int i = 0; i < 20; ++i) {
        for (unsigned int j = 0; j < 19; ++j) {
            cloth_index.push_back(i * 20 + j);
            cloth_index.push_back(i * 20 + j + 1);
        }
    }
    
    for (unsigned int i = 0; i < 19; ++i) {
        for (unsigned int j = 0; j < 20; ++j) {
            cloth_index.push_back(i * 20 + j);
            cloth_index.push_back((i + 1) * 20 + j);
        }
    }
    
    // Cloth Particles
    glm::vec3 particles_pos[1000];
    glm::vec3 particles_pos_old[1000];
    for (unsigned int i = 0; i < 20; ++i) {
        for (unsigned int j = 0; j < 20; ++j) {
            particles_pos[20*i+j] = glm::vec3(i / 20.0f,1.0f,j / 20.0f);
            particles_pos_old[20*i+j] = glm::vec3(i / 20.0f,1.0f,j / 20.0f);
        }
    }
    
    // Cloth VAO
    GLuint particle_vao;
    glGenVertexArrays(1, &particle_vao);
    glBindVertexArray(particle_vao);
    
    // Cloth EBO
    GLuint cloth_ebo;
    glGenBuffers(1, &cloth_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloth_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * cloth_index.size(), &cloth_index[0], GL_STATIC_DRAW);
    
    // Particle VBOs and TFO
    GLuint particles_pos_vbo[2], particles_pos_old_vbo[2];
    glGenBuffers(2, particles_pos_vbo);
    glGenBuffers(2, particles_pos_old_vbo);
    
    GLuint particles_tfo[2];
    glGenTransformFeedbacks(2, particles_tfo);
    for (unsigned int i = 0; i < 2; ++i) {
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, particles_tfo[i]);
        
        glBindBuffer(GL_ARRAY_BUFFER, particles_pos_vbo[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(particles_pos), particles_pos, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, particles_pos_vbo[i]);
        
        glBindBuffer(GL_ARRAY_BUFFER, particles_pos_old_vbo[i]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(particles_pos_old), particles_pos_old, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 1, particles_pos_old_vbo[i]);
    }
    
    // Particle Texture Buffer
    GLuint particle_pos_tbo, particle_pos_old_tbo;
    
    glGenTextures(1, &particle_pos_tbo);
    glBindTexture(GL_TEXTURE_BUFFER, particle_pos_tbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, particles_pos_vbo[0]);
    
    glGenTextures(1, &particle_pos_old_tbo);
    glBindTexture(GL_TEXTURE_BUFFER, particle_pos_old_tbo);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, particles_pos_old_vbo[0]);
    
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    
    // Simulation / Render Loop
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        processInput(window);
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Update Particle System
        glEnable(GL_RASTERIZER_DISCARD);
        
        particlesShader.use();
        particlesShader.setFloat("deltaTime", deltaTime * 0.1f);
        particlesShader.setInt("tex_position", 0);
        particlesShader.setInt("tex_prev_position", 1);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_BUFFER, particle_pos_tbo);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, particles_pos_vbo[curr_particles_vbo]);
        
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, particle_pos_old_tbo);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGB32F, particles_pos_old_vbo[curr_particles_vbo]);
        
        glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, particles_tfo[curr_particles_tfo]);
        
        glBindBuffer(GL_ARRAY_BUFFER, particles_pos_vbo[curr_particles_vbo]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(GLvoid *)0); // pos
        
        glBindBuffer(GL_ARRAY_BUFFER, particles_pos_old_vbo[curr_particles_vbo]);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(GLvoid *)0); // pos old
        
        
        glBeginTransformFeedback(GL_POINTS);
        if (first) {
            glDrawArrays(GL_POINTS, 0, 400);
            first = false;
        }
        else {
            glDrawTransformFeedback(GL_POINTS, particles_tfo[curr_particles_vbo]); // old
        }
        
        glEndTransformFeedback();
        glDisableVertexAttribArray(0);
        
        
        // Render Cloth
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 200.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        pointShader.use();
        pointShader.setMat4("projection", projection);
        pointShader.setMat4("view", view);
        
        glDisable(GL_RASTERIZER_DISCARD);
        glEnable(GL_PROGRAM_POINT_SIZE);
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloth_ebo);
        glBindBuffer(GL_ARRAY_BUFFER, particles_pos_vbo[curr_particles_tfo]);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(glm::vec3),(GLvoid *)0);
        
        pointShader.setVec3("color", glm::vec3(1,0,0));
        glDrawArrays(GL_POINTS, 0, 400);
        pointShader.setVec3("color", glm::vec3(1,1,1));
        glDrawElements(GL_LINES, cloth_index.size(), GL_UNSIGNED_INT, nullptr);
        
        glDisableVertexAttribArray(0);

        curr_particles_vbo = curr_particles_tfo;
        curr_particles_tfo = (curr_particles_tfo + 1) & 0x1;
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    camera.ProcessKeyboard(DOWN, deltaTime);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    
    lastX = xpos;
    lastY = ypos;
    
    camera.ProcessMouseMovement(xoffset, yoffset);
}
