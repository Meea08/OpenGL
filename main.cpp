#define GLEW_STATIC
#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "glm/glm.hpp" //core glm functionality
#include "glm/gtc/matrix_transform.hpp" //glm extension for generating common transformation matrices
#include "glm/gtc/matrix_inverse.hpp" //glm extension for computing inverse matrices
#include "glm/gtc/type_ptr.hpp" //glm extension for accessing the internal data structure of glm types

#include "Window.h"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Model3D.hpp"
#include "SkyBox.hpp"
#include <iostream>
// window
gps::Window myWindow;

int retina_width, retina_height;

// matrices
glm::mat4 model;
glm::mat4 view;
glm::mat4 projection;
glm::mat3 normalMatrix;
glm::mat4 waterMatrix;
glm::mat3 lightDirMatrix;

// light parameters
glm::vec3 lightDir;
glm::vec3 lightColor;

// shader uniform locations
GLint modelLoc;
GLint viewLoc;
GLint projectionLoc;
GLint normalMatrixLoc;
GLint lightDirLoc;
GLint lightColorLoc;
GLint lightDirMatrixLoc;

// camera
gps::Camera myCamera(
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(-0.27, -0.21f, -0.55f),
    glm::vec3(0.0f, 1.0f, 0.0f));

GLfloat cameraSpeed = 0.01f;

GLboolean pressedKeys[1024];

// models
gps::Model3D scene;
gps::Model3D water_plane;
gps::Model3D umbrela_top;
gps::Model3D dog_tail;
gps::Model3D eagle_body;
//gps::Model3D eagle_wing_right;
//gps::Model3D eagle_wing_left;
gps::Model3D duck_3_formation;
gps::Model3D duck1;
gps::Model3D duck2;
gps::Model3D car1;
gps::Model3D lightCube;

GLfloat angle;

// shaders
gps::Shader myBasicShader;
gps::Shader depthMapShader;
gps::Shader lightShader;

// skybox
gps::SkyBox mySkyBox;
gps::Shader skyboxShader;

// fog
GLint fogLoc;
GLfloat fogDensity = 0.0f;

// camera anim
bool animation = false;

// shadows
const unsigned int SHADOW_WIDTH = 1084;
const unsigned int SHADOW_HEIGHT = 1084;
GLuint shadowMapFBO;
GLuint depthMapTexture;

glm::vec3 P0 = glm::vec3(-0.32f,-0.18f, -1.306f);
//glm::vec3 P2 = glm::vec3(0.06f, -0.006f, 1.19f);
glm::vec3 P2 = glm::vec3(1.03f, 0.34f, -0.54f);
glm::vec3 P1 = glm::vec3(0.07f, -0.21f, 0.05f);

glm::vec3 Bezier3Points(float t, glm::vec3 P0b, glm::vec3 P1b, glm::vec3 P2b) {
    return (1 - t) * (P0b * (1 - t) + P1b * t) + (P1b * (1 - t) + P2b * t) * t;
}

GLenum glCheckError_(const char *file, int line)
{
	GLenum errorCode;
	while ((errorCode = glGetError()) != GL_NO_ERROR) {
		std::string error;
		switch (errorCode) {
            case GL_INVALID_ENUM:
                error = "INVALID_ENUM";
                break;
            case GL_INVALID_VALUE:
                error = "INVALID_VALUE";
                break;
            case GL_INVALID_OPERATION:
                error = "INVALID_OPERATION";
                break;
            case GL_STACK_OVERFLOW:
                error = "STACK_OVERFLOW";
                break;
            case GL_STACK_UNDERFLOW:
                error = "STACK_UNDERFLOW";
                break;
            case GL_OUT_OF_MEMORY:
                error = "OUT_OF_MEMORY";
                break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:
                error = "INVALID_FRAMEBUFFER_OPERATION";
                break;
        }
		std::cout << error << " | " << file << " (" << line << ")" << std::endl;
	}
	return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__)

void windowResizeCallback(GLFWwindow* window, int width, int height) {
	fprintf(stdout, "Window resized! New width: %d , and height: %d\n", width, height);
	//TODO
    glfwGetFramebufferSize(myWindow.getWindow(), &retina_width, &retina_height);
    /*retina_width = myWindow.getWindowDimensions().width;
    retina_height = myWindow.getWindowDimensions().height;*/

    myBasicShader.useShaderProgram();

    // set projection matrix
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)retina_width / (float)retina_height, 0.01f, 1000.0f);
    //send matrix data to shader
    GLint projLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // set Viewport transform
    glViewport(0, 0, retina_width, retina_height);
}

void keyboardCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

	if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            pressedKeys[key] = true;
        } else if (action == GLFW_RELEASE) {
            pressedKeys[key] = false;
        }
    }
}

bool mouse = true;
bool eagleAnim = false;
float lastX = 400, lastY = 300;
float yaw = -90.0f, pitch;
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (mouse)
    {
        lastX = xpos;
        lastY = ypos;
        mouse = false;
    }

    float xoffset = lastX - xpos;
    float yoffset = lastY - ypos; // reversed since y-coordinates range from bottom to top
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    myCamera.rotate(pitch, yaw);
}

int far = 0;
float R = 1, G = 0, B = 0;
float angleY = 0.0f;
GLfloat lightAngle;
float ducksx;
void processMovement() {
    // GO FORWARD
	if (pressedKeys[GLFW_KEY_W]) {
		myCamera.move(gps::MOVE_FORWARD, cameraSpeed);
		//update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	}
    // GO BACKWARD
	if (pressedKeys[GLFW_KEY_S]) {
		myCamera.move(gps::MOVE_BACKWARD, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}
    // GO LEFT
	if (pressedKeys[GLFW_KEY_A]) {
		myCamera.move(gps::MOVE_LEFT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}
    // GO RIGHT
	if (pressedKeys[GLFW_KEY_D]) {
		myCamera.move(gps::MOVE_RIGHT, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
	}
    // ROTATE TO LEFT
    if (pressedKeys[GLFW_KEY_Q]) {
        angle -= 1.0f;
        if (angle > 360.0f)
            angle -= 360.0f;
        myCamera.rotate(0, angle);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
    // ROTATE TO RIGHT
    if (pressedKeys[GLFW_KEY_E]) {
        angle += 1.0f;
        if (angle < 0.0f)
            angle += 360.0f;
        myCamera.rotate(0, angle);
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
    // MOVE CAMERA UP
    if (pressedKeys[GLFW_KEY_UP]) {
        myCamera.move(gps::MOVE_UP, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
    // MOVE CAMERA DOWN
    if (pressedKeys[GLFW_KEY_DOWN]) {
        myCamera.move(gps::MOVE_DOWN, cameraSpeed);
        //update view matrix
        view = myCamera.getViewMatrix();
        myBasicShader.useShaderProgram();
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        // compute normal matrix for teapot
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    }
    // LINE VISUALIZATION
    if (pressedKeys[GLFW_KEY_1]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    // POINT VISUALIZATION
    if (pressedKeys[GLFW_KEY_2]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
    }
    // NORMAL VISUALIZATION
    if (pressedKeys[GLFW_KEY_3]) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    // INCREASE FOG DENSITY
    if (pressedKeys[GLFW_KEY_RIGHT])
    {
        myBasicShader.useShaderProgram();
        if(fogDensity<=0.985f)
            fogDensity += 0.005f;
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
    }
    // DECREASE FOG DENSITY
    if (pressedKeys[GLFW_KEY_LEFT])
    {
        myBasicShader.useShaderProgram();
        if(fogDensity>=0.005f)
            fogDensity -= 0.005f;
        glUniform1f(glGetUniformLocation(myBasicShader.shaderProgram, "fogDensity"), fogDensity);
    }
    // PRINT CAMERA POSITION & TARGET
    if (pressedKeys[GLFW_KEY_SPACE]) {
        myCamera.printPosition();
        myCamera.printTarget();
    }
    // START CAMERA ANIMATION
    if (pressedKeys[GLFW_KEY_Z]) {
        animation = true;
    }
    // MOVE DUCK
    if (pressedKeys[GLFW_KEY_N]) {
        if(ducksx<=0.23)
            ducksx += 0.001;
    }
    if (pressedKeys[GLFW_KEY_M]) {
        if(ducksx>=-0.19)
            ducksx -= 0.001;
    }
}

void initOpenGLWindow() {
    myWindow.Create(1024, 768, "OpenGL Project Core");
}

void setWindowCallbacks() {
	glfwSetWindowSizeCallback(myWindow.getWindow(), windowResizeCallback);
    glfwSetKeyCallback(myWindow.getWindow(), keyboardCallback);
    glfwSetInputMode(myWindow.getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetCursorPosCallback(myWindow.getWindow(), mouseCallback);
}

void initOpenGLState() {
	glClearColor(0.7f, 0.7f, 0.7f, 1.0f);
	glViewport(0, 0, myWindow.getWindowDimensions().width, myWindow.getWindowDimensions().height);
    glEnable(GL_FRAMEBUFFER_SRGB);
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CCW); // GL_CCW for counter clock-wise
}

void initFBOs()
{
    glGenFramebuffers(1, &shadowMapFBO);
    glGenTextures(1, &depthMapTexture);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMapTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void initModels() {
    lightCube.LoadModel("models/cube/cube.obj");
    scene.LoadModel("models/final_scene/munti_mai_frumosi.obj");
    water_plane.LoadModel("models/water_plane/water_plane.obj");
    umbrela_top.LoadModel("models/umbrella_top/umbrella.obj");
    dog_tail.LoadModel("models/doggo_tail/doggo_tail.obj");
    eagle_body.LoadModel("models/eagle/eagle_body.obj");
   /* eagle_wing_right.LoadModel("models/eagle/eagle_wing_right.obj");
    eagle_wing_left.LoadModel("models/eagle/eagle_wing_left.obj");*/
    duck_3_formation.LoadModel("models/ducks/3ducks.obj");
    duck1.LoadModel("models/ducks/duck1.obj");
    duck2.LoadModel("models/ducks/duck2.obj");
    //car1.LoadModel("models/cars/cars.obj");
}

void initShaders() {
	myBasicShader.loadShader(
        "shaders/basic.vert",
        "shaders/basic.frag"
    );
    
    std::vector<const GLchar*> faces;
    faces.push_back("skybox/bluecloud_rt.tga");
    faces.push_back("skybox/bluecloud_lf.tga");
    faces.push_back("skybox/bluecloud_up.tga");
    faces.push_back("skybox/bluecloud_dn.tga");
    faces.push_back("skybox/bluecloud_bk.tga");
    faces.push_back("skybox/bluecloud_ft.tga");

    mySkyBox.Load(faces);
    skyboxShader.loadShader(
        "shaders/skyboxShader.vert", 
        "shaders/skyboxShader.frag"
    );
    skyboxShader.useShaderProgram();

    /*depthMapShader.loadShader(
        "shaders/simpleDepthMap.vert",
        "shaders/simpleDepthMap.frag"
        );
    lightShader.loadShader(
        "shaders/lightCube.vert",
        "shaders/lightCube.frag"
    );*/
}

void initUniforms() {
	myBasicShader.useShaderProgram();

    // create model matrix for teapot
    model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(myBasicShader.shaderProgram, "model");

	// get view matrix for current camera
	view = myCamera.getViewMatrix();
	viewLoc = glGetUniformLocation(myBasicShader.shaderProgram, "view");

	// send view matrix to shader
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    // compute normal matrix for teapot
    normalMatrix = glm::mat3(glm::inverseTranspose(view*model));
	normalMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "normalMatrix");

	// create projection matrix
	projection = glm::perspective(glm::radians(45.0f),
                               (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height,
                               0.01f, 100.0f);
	projectionLoc = glGetUniformLocation(myBasicShader.shaderProgram, "projection");
	// send projection matrix to shader
	glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projection));	
    

	//set the light direction (direction towards the light)
	lightDir = glm::vec3(0.0f, 1.0f, 1.0f);
	lightDirLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDir");
    
	// send light dir to shader
	glUniform3fv(lightDirLoc, 1, glm::value_ptr(lightDir));
    

    /*lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    */
	//set light color
    myBasicShader.useShaderProgram();
	lightColor = glm::vec3(1.0f, 1.0f, 1.0f); //white light
	lightColorLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightColor");
    
	// send light color to shader
	glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));
    

    skyboxShader.useShaderProgram();
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    projection = glm::perspective(glm::radians(45.0f), (float)myWindow.getWindowDimensions().width / (float)myWindow.getWindowDimensions().height, 0.1f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(skyboxShader.shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

glm::mat4 computeLightSpaceTrMatrix() {
    const GLfloat near_plane = 0.01f, far_plane = 1000.0f;
    glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, near_plane, far_plane);

    glm::vec3 lightDirTr = glm::vec3(glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(lightDir, 1.0f));
    glm::mat4 lightView = glm::lookAt(myCamera.cameraTarget + 1.0f * lightDirTr, myCamera.cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    return lightProjection * lightView;
}
bool shadow = true;
void renderSceneMountains(gps::Shader shader) {
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    if (shadow==false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw scene
    scene.Draw(shader);
}

float i;
float p = 0.00005f;
void renderWater(gps::Shader shader) {
    shader.useShaderProgram();
    //waterMatrix = glm::mat4(1.0f);
    model = glm::mat4(1.0f);
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::translate(model, glm::vec3(i, 0.0f, 0.0f));
    i += p;
    if (i >= 0.002f || i<=-0.002f)
            p =-p;
    
    //printf("p:%f, i:%f\n", p, i);

    //send scene model matrix data to shader
    //glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "modelLoc"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    
    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw scene
    water_plane.Draw(shader);
    //waterMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
    /*glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));*/
}

float wind;
float p2 = 0.002;
float angleUmbrella;
void renderUmbrellaTop(gps::Shader shader) {
    angleUmbrella += p2;
    if (angleUmbrella > 0.005 || angleUmbrella < -0.005)
        p2 = -p2;
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::rotate(glm::radians(angleUmbrella), glm::vec3(0, 1, 0));
    model = glm::translate(model, glm::vec3(0.00000000002f + wind, 0.0f, -0.004f));
    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw scene
    umbrela_top.Draw(shader);
}

float angleTail;
float p3=0.05;
glm::mat4 modelDog=glm::mat4(1.0f);
void renderDogTail(gps::Shader shader) {
    angleTail += p3;
    if (angleTail >= 0.1 || angleTail <= -0.1)
        p3 = -p3;
    shader.useShaderProgram();
    modelDog = glm::mat4(1.0f);
    //modelDog = glm::rotate(modelDog, glm::radians(angle), glm::vec3(0, 1, 0));
    //model = glm::translate(model, glm::vec3(-10.0f, -10.0f, -10.0f));
    //modelDog = glm::translate(modelDog, glm::vec3(0.0f, 0.0f, 0.0f));
    modelDog = glm::rotate(glm::radians(angleTail), glm::vec3(0, 0, 1));
    //modelDog = glm::translate(modelDog, glm::vec3(0.23972f, -0.25816f, 0.12199f));
    
    
    //model = glm::rotate(glm::radians(angleTail), glm::vec3(0, 1, 1));

    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(modelDog));

    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }// draw scene
    dog_tail.Draw(shader);
}

float eagleAngle;
void renderEagle(gps::Shader shader) {
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    model = glm::rotate(glm::mat4(1.0f), glm::radians(eagleAngle), glm::vec3(0, 1, 0));
    eagleAngle += 0.5f;
    if (eagleAngle == 359.0f)
        eagleAngle = 0.0f;
    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    // draw scene
    eagle_body.Draw(shader);
   
}

float duck1Angle;
float p4 = 0.01f;
void renderDuck1(gps::Shader shader) {
    duck1Angle -= p4;
    if (duck1Angle < -9.0f || duck1Angle > 3.5f)
        p4 = -p4;
    shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    model = glm::rotate(glm::radians(duck1Angle), glm::vec3(0, 1, 0));
    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }//duck_3_formation.Draw(shader);
    duck1.Draw(shader);
    //printf("%f\n", duck1Angle);
    /*duck2.Draw(shader);*/
}


glm::vec3 duckP1 = glm::vec3(0.58f, -0.28f, 0.63f);
glm::vec3 duckP2 = glm::vec3(0.33f, -0.28f, 0.44f);

void renderDucks_3Formation(gps::Shader shader) {
    
    shader.useShaderProgram();
    model = glm::mat4(1.0f);

    model = glm::translate(model, glm::vec3(ducksx, 0.0028f, 0.045f));
    //printf("%f\n", ducksx);
    //send scene model matrix data to shader
    glUniformMatrix4fv(glGetUniformLocation(shader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    if (shadow == false) {
        normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
        glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    }
    duck_3_formation.Draw(shader);
    //duck1.Draw(shader);
    //printf("%f\n", duck1Angle);
    /*duck2.Draw(shader);*/
}

void renderCars(gps::Shader shader) {
    shader.useShaderProgram();

    //send scene model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    car1.Draw(shader);
}

void renderSkybox(gps::Shader shader) {
    //shader.useShaderProgram();
    model = glm::mat4(1.0f);
    //model = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0, 1, 0));
    //send scene model matrix data to shader
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    //send scene normal matrix data to shader
    normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
    glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));
    mySkyBox.Draw(skyboxShader, view, projection);
}

float t=0.0f;
void renderScene() {
    if (animation) {
        if (t < 1.0f) {
            
            glm::vec3 newPos = Bezier3Points(t, P0, P2, P1);
            
            glm::vec3 newTar = glm::normalize(newPos);
            myCamera.moveTo(newPos, newTar);
            //view = myCamera.getViewMatrix();
            //myBasicShader.useShaderProgram();
            //glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
            //// compute normal matrix for teapot
            //normalMatrix = glm::mat3(glm::inverseTranspose(view * model));
            t += 0.001f;
            
        }
        else {
            animation = false;
            t = 0.0f;
            
        }
    }
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glEnable(GL_BLEND);
    
    wind = sin(glfwGetTime()) * 0.002f;

    depthMapShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(depthMapShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    shadow = true;
    renderSceneMountains(depthMapShader);
    renderWater(depthMapShader);
    renderUmbrellaTop(depthMapShader);
    renderDogTail(depthMapShader);
    renderDuck1(depthMapShader);
    renderDucks_3Formation(depthMapShader);
    

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    myBasicShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "lightSpaceTrMatrix"),
        1,
        GL_FALSE,
        glm::value_ptr(computeLightSpaceTrMatrix()));
    view = myCamera.getViewMatrix();
    glUniformMatrix4fv(glGetUniformLocation(myBasicShader.shaderProgram, "view"),
        1,
        GL_FALSE,
        glm::value_ptr(view));
    lightDirMatrix = glm::mat3(glm::inverseTranspose(view));
    lightDirMatrixLoc = glGetUniformLocation(myBasicShader.shaderProgram, "lightDirMatrix");
    glUniformMatrix3fv(lightDirMatrixLoc, 1, GL_FALSE, glm::value_ptr(lightDirMatrix));

    
    glViewport(0, 0, retina_width, retina_height);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthMapTexture);
    glUniform1i(glGetUniformLocation(myBasicShader.shaderProgram, "shadowMap"), 3);

    shadow = false;
    renderSceneMountains(myBasicShader);
    renderWater(myBasicShader);
    renderUmbrellaTop(myBasicShader);
    renderDogTail(myBasicShader);
    renderDuck1(myBasicShader);
    renderDucks_3Formation(myBasicShader);
    renderEagle(myBasicShader);
    
    lightShader.useShaderProgram();
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    
    model = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, 1.0f * lightDir);
    model = glm::scale(model, glm::vec3(0.005f, 0.005f, 0.005f));
    glUniformMatrix4fv(glGetUniformLocation(lightShader.shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    

    lightCube.Draw(lightShader);
    skyboxShader.useShaderProgram();
    renderSkybox(skyboxShader);
    myBasicShader.useShaderProgram();

}

void cleanup() {
    myWindow.Delete();
    //cleanup code for your own data
}

int main(int argc, const char * argv[]) {

    try {
        initOpenGLWindow();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    initOpenGLState();
    
    initFBOs();
    
	initModels();
    
	initShaders();
    
	initUniforms();
    
    setWindowCallbacks();
    glCheckError();
	
	// application loop
	while (!glfwWindowShouldClose(myWindow.getWindow())) {
        processMovement();
	    renderScene();

		glfwPollEvents();
        
		glfwSwapBuffers(myWindow.getWindow());
	}
    glCheckError();

	cleanup();

    return EXIT_SUCCESS;
}
