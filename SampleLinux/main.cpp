#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <ctype.h>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef WIN32
#include <windows.h>
#pragma warning(disable:4996)
#endif

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include "glew.h"
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include "glut.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Constants:
const char *WINDOWTITLE = "OpenGL / GLUT Sample Minimal";
const int INIT_WINDOW_SIZE = 600;
const int MS_PER_CYCLE = 30000; // 10 seconds for a full cycle

// Escape Key:
const int ESCAPE = 0x1b;

// Mouse:
int ActiveButton = 0;
int Xmouse, Ymouse;
const float ANGFACT = 1.f;
const float SCLFACT = 0.005f;
const float MINSCALE = 0.05f;
const int LEFT = 4;
const int MIDDLE = 2;
const int RIGHT = 1;
const int SCROLL_WHEEL_UP = 3;
const int SCROLL_WHEEL_DOWN = 4;
const float SCROLL_WHEEL_CLICK_FACTOR = 5.f;

int MainWindow;
float Scale = 1.0f;
float Xrot = 0.f, Yrot = 0.f;

bool autoRotate = true;
float Time = 0.f;

// Panels:
enum ProjectionType { ORTHO, PERSP };
ProjectionType NowProjection = PERSP;

// Panel grid positions (9 panels):
glm::vec3 panelPositionsArr[] = {
    glm::vec3(-2.0f, 0.5f, -2.0f),
    glm::vec3(0.0f, 0.5f, -2.0f),
    glm::vec3(2.0f, 0.5f, -2.0f),
    glm::vec3(-2.0f, 0.5f, 0.0f),
    glm::vec3(0.0f, 0.5f, 0.0f),
    glm::vec3(2.0f, 0.5f, 0.0f),
    glm::vec3(-2.0f, 0.5f, 2.0f),
    glm::vec3(0.0f, 0.5f, 2.0f),
    glm::vec3(2.0f, 0.5f, 2.0f)
};

// For logging:
struct PanelLog {
    int panelID;
    glm::vec3 position;
    float timeStamp;
    float sunlightStrength;

    PanelLog(int id, glm::vec3 pos, float time, float strength)
        : panelID(id), position(pos), timeStamp(time), sunlightStrength(strength) {}
};

std::vector<PanelLog> panelLogs;

float ElapsedSeconds() {
    int ms = glutGet(GLUT_ELAPSED_TIME);
    return (float)ms / 1000.f;
}

float calculateSunlightStrength(const glm::vec3& panelPos, const glm::vec3& lightPos) {
    glm::vec3 panelNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 dirToLight = glm::normalize(lightPos - panelPos);
    float strength = glm::dot(panelNormal, dirToLight);
    return glm::max(strength, 0.0f);
}

void logPanelPositions(const glm::vec3& lightPos) {
    float currentTime = ElapsedSeconds();
    for (int i = 0; i < 9; ++i) {
        float sunlightStrength = calculateSunlightStrength(panelPositionsArr[i], lightPos);
        panelLogs.push_back({i + 1, panelPositionsArr[i], currentTime, sunlightStrength});
    }

    std::ofstream logFile("panel_log.txt", std::ios::app);
    if (logFile.is_open()) {
        for (const auto& log : panelLogs) {
            logFile << "Panel ID: " << log.panelID << ", Time: " << log.timeStamp << "s, Position: ("
                    << log.position.x << ", " << log.position.y << ", "
                    << log.position.z << "), Sunlight Strength: " << log.sunlightStrength << "\n";
        }
        logFile.close();
    }
}

// Geometry:
float terrainVertices[] = {
    -5.0f, 0.0f, -5.0f,   0.0f, 0.0f,
     5.0f, 0.0f, -5.0f,   1.0f, 0.0f,
    -5.0f, 0.0f,  5.0f,   0.0f, 1.0f,

     5.0f, 0.0f, -5.0f,   1.0f, 0.0f,
     5.0f, 0.0f,  5.0f,   1.0f, 1.0f,
    -5.0f, 0.0f,  5.0f,   0.0f, 1.0f
};

float panelVertices[] = {
    -0.5f, 0.0f, 0.0f,
     0.5f, 0.0f, 0.0f,
    -0.5f, 0.0f, 1.0f,
     0.5f, 0.0f, 0.0f,
     0.5f, 0.0f, 1.0f,
    -0.5f, 0.0f, 1.0f
};

float baseVertices[] = {
    // Front face (z =  0.05)
    -0.05f, 0.0f,  0.05f,
     0.05f, 0.0f,  0.05f,
     0.05f, 1.0f,  0.05f,
    -0.05f, 1.0f,  0.05f,

    // Back face (z = -0.05)
    -0.05f, 0.0f, -0.05f,
    -0.05f, 1.0f, -0.05f,
     0.05f, 1.0f, -0.05f,
     0.05f, 0.0f, -0.05f,

    // Left face (x = -0.05)
    -0.05f, 0.0f, -0.05f,
    -0.05f, 0.0f,  0.05f,
    -0.05f, 1.0f,  0.05f,
    -0.05f, 1.0f, -0.05f,

    // Right face (x = 0.05)
     0.05f, 0.0f, -0.05f,
     0.05f, 1.0f, -0.05f,
     0.05f, 1.0f,  0.05f,
     0.05f, 0.0f,  0.05f,

    // Top face (y = 1.0)
    -0.05f, 1.0f,  0.05f,
     0.05f, 1.0f,  0.05f,
     0.05f, 1.0f, -0.05f,
    -0.05f, 1.0f, -0.05f,

    // Bottom face (y = 0.0)
    -0.05f, 0.0f,  0.05f,
    -0.05f, 0.0f, -0.05f,
     0.05f, 0.0f, -0.05f,
     0.05f, 0.0f,  0.05f
};



unsigned int baseIndices[] = {
    0,1,2,2,3,0,
    4,5,6,6,7,4,
    8,9,10,10,11,8,
    12,13,14,14,15,12,
    16,17,18,18,19,16,
    20,21,22,22,23,20
};

float sunVertices[] = {
    -0.1f,-0.1f,0.1f,   0.1f,-0.1f,0.1f,   0.1f,0.1f,0.1f,   -0.1f,0.1f,0.1f,
    -0.1f,-0.1f,-0.1f, -0.1f,0.1f,-0.1f,  0.1f,0.1f,-0.1f,  0.1f,-0.1f,-0.1f,
    -0.1f,-0.1f,-0.1f, -0.1f,-0.1f,0.1f,  -0.1f,0.1f,0.1f,  -0.1f,0.1f,-0.1f,
     0.1f,-0.1f,-0.1f,  0.1f,0.1f,-0.1f,  0.1f,0.1f,0.1f,   0.1f,-0.1f,0.1f,
    -0.1f,0.1f,0.1f,    0.1f,0.1f,0.1f,   0.1f,0.1f,-0.1f,  -0.1f,0.1f,-0.1f,
    -0.1f,-0.1f,0.1f,  -0.1f,-0.1f,-0.1f,  0.1f,-0.1f,-0.1f, 0.1f,-0.1f,0.1f
};

unsigned int cubeIndices[] = {
    0,1,2,2,3,0,
    4,5,6,6,7,4,
    8,9,10,10,11,8,
    12,13,14,14,15,12,
    16,17,18,18,19,16,
    20,21,22,22,23,20
};

// Panel Grid lines:
std::vector<GLfloat> panelGridVertices;

float SunRadius = 15.0f;
float SunHeight = 3.0f;

// Shader:
GLuint shaderProgram;
GLint modelLoc, viewLoc, projLoc;

// Uniforms:
GLint brightnessLoc;

// Objects:
GLuint terrainVAO, terrainVBO;
GLuint panelVAO, panelVBO;
GLuint panelGridVAO, panelGridVBO;
GLuint baseVAO, baseVBO, baseEBO;
GLuint sunVAO, sunVBO, sunEBO;

// Texture:
GLuint groundTexture;

// Simple Shaders:
static GLuint buildSimpleShaderProgram() {
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;

    void main(){
        gl_Position = projection * view * model * vec4(aPos,1.0);
        TexCoord = aTexCoord;
    }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec2 TexCoord;
        out vec4 FragColor;

        uniform vec3 objectColor;        // Color for solid objects
        uniform sampler2D texture1;      // Texture for textured objects
        uniform bool useTexture;         // Flag to determine if texture should be applied
        uniform float brightness;        // Lighting brightness

        void main() {
            vec3 color;
            if (useTexture) {
                color = texture(texture1, TexCoord).rgb; // Use texture color
            } else {
                color = objectColor; // Use solid color
            }

            vec3 finalColor = color * brightness; // Apply lighting
            FragColor = vec4(finalColor, 1.0);
        }

    )";

    GLuint vertexShader=glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader,1,&vertexShaderSource,NULL);
    glCompileShader(vertexShader);

    GLint success; char infoLog[512];
    glGetShaderiv(vertexShader,GL_COMPILE_STATUS,&success);
    if(!success){
        glGetShaderInfoLog(vertexShader,512,NULL,infoLog);
        fprintf(stderr,"Vertex Shader Error: %s\n",infoLog);
    }

    GLuint fragmentShader=glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader,1,&fragmentShaderSource,NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader,GL_COMPILE_STATUS,&success);
    if(!success){
        glGetShaderInfoLog(fragmentShader,512,NULL,infoLog);
        fprintf(stderr,"Fragment Shader Error: %s\n",infoLog);
    }

    GLuint program=glCreateProgram();
    glAttachShader(program,vertexShader);
    glAttachShader(program,fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program,GL_LINK_STATUS,&success);
    if(!success){
        glGetProgramInfoLog(program,512,NULL,infoLog);
        fprintf(stderr,"Program Linking Error: %s\n",infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

static void buildPanelGrid() {
    float xs[5] = {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f};
    float zs[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    for (int i=0; i<5; i++){
        panelGridVertices.push_back(xs[i]); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(0.0f);
        panelGridVertices.push_back(xs[i]); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(1.0f);
    }

    for (int i=0; i<5; i++){
        panelGridVertices.push_back(-0.5f); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(zs[i]);
        panelGridVertices.push_back( 0.5f); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(zs[i]);
    }
}


static void setupObjects() {
    // terrain
    glGenVertexArrays(1,&terrainVAO);
    glGenBuffers(1,&terrainVBO);
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(terrainVertices),terrainVertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    // panel
    glGenVertexArrays(1,&panelVAO);
    glGenBuffers(1,&panelVBO);
    glBindVertexArray(panelVAO);
    glBindBuffer(GL_ARRAY_BUFFER,panelVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(panelVertices),panelVertices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    // base
    glGenVertexArrays(1,&baseVAO);
    glGenBuffers(1,&baseVBO);
    glGenBuffers(1,&baseEBO);
    glBindVertexArray(baseVAO);
    glBindBuffer(GL_ARRAY_BUFFER,baseVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(baseVertices),baseVertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,baseEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(baseIndices),baseIndices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    // sun
    glGenVertexArrays(1,&sunVAO);
    glGenBuffers(1,&sunVBO);
    glGenBuffers(1,&sunEBO);
    glBindVertexArray(sunVAO);
    glBindBuffer(GL_ARRAY_BUFFER,sunVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(sunVertices),sunVertices,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,sunEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(cubeIndices),cubeIndices,GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    // panel grid
    glGenVertexArrays(1, &panelGridVAO);
    glGenBuffers(1, &panelGridVBO);
    glBindVertexArray(panelGridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, panelGridVBO);
    glBufferData(GL_ARRAY_BUFFER, panelGridVertices.size()*sizeof(float), panelGridVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);
}

static float computePanelRotation(const glm::vec3& panelPos, const glm::vec3& lightPos) {
    glm::vec3 panelNormal(0.0f,1.0f,0.0f);
    glm::vec3 dirToLight = glm::normalize(lightPos - panelPos);

    float dotVal = glm::dot(panelNormal, dirToLight);
    dotVal = glm::clamp(dotVal,-1.0f,1.0f);
    float angleRad = acos(dotVal);
    float angleDeg = glm::degrees(angleRad);
    if(angleDeg > 75.0f) angleDeg=75.0f;

    if((lightPos.x - panelPos.x) > 0.0f) {
        angleDeg = -angleDeg;
    }

    return angleDeg;
}


void Animate() {
    static float lastLogTime = 0.0f;
    float currentTime = ElapsedSeconds();

    // Log every 2 seconds
    if (currentTime - lastLogTime > 2.0f) {
        float angleRad = Time * 2.0f * M_PI;
        glm::vec3 lightPos(cos(angleRad)*SunRadius, sin(angleRad)*SunRadius, 0.0f);
        logPanelPositions(lightPos);
        lastLogTime = currentTime;
    }

    if (autoRotate) {
        int ms = glutGet(GLUT_ELAPSED_TIME);
        ms %= MS_PER_CYCLE;
        Time = (float)ms / (float)MS_PER_CYCLE;
    }

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void DisplayLogsOnScreen() {
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(-5.0, 5.0, -5.0, 5.0);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.f, 1.f, 1.f);

    float startX = -4.5f;
    float startY = 4.5f;
    char buffer[256];

    for (size_t i = 0; i < panelLogs.size() && i < 10; ++i) {
        snprintf(buffer, sizeof(buffer), "Panel ID: %d, Time: %.2fs, Pos:(%.2f, %.2f, %.2f), Sunlight: %.2f",
                 panelLogs[i].panelID, panelLogs[i].timeStamp, panelLogs[i].position.x,
                 panelLogs[i].position.y, panelLogs[i].position.z, panelLogs[i].sunlightStrength);

        glRasterPos2f(startX, startY - (i * 0.5f));
        const char* txt = buffer;
        while (*txt) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *txt++);
        }
    }

    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glEnable(GL_DEPTH_TEST);
}

void Display() {
    glutSetWindow(MainWindow);
    glDrawBuffer(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    GLsizei vx = glutGet(GLUT_WINDOW_WIDTH);
    GLsizei vy = glutGet(GLUT_WINDOW_HEIGHT);
    GLsizei v = vx < vy ? vx : vy;
    GLint xl = (vx - v)/2;
    GLint yb = (vy - v)/2;
    glViewport(xl,yb,v,v);

    // Compute sun pos:
    float angleRad = Time * 2.0f * M_PI;
    glm::vec3 lightPos(cos(angleRad)*SunRadius, sin(angleRad)*SunRadius, 0.0f);

    // Compute brightness: sun above horizon => brightness=1, else=0.3
    float elevation = sin(angleRad);
    float brightness = (elevation > 0.0f) ? 1.0f : 0.3f;

    glUseProgram(shaderProgram);
    glUniform1f(brightnessLoc, brightness);

    // Projection:
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (NowProjection == ORTHO)
        glOrtho(-2.f,2.f,-2.f,2.f,0.1f,1000.f);
    else
        gluPerspective(90.f,1.f,0.1f,1000.f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.f,2.f,6.f,
              0.f,0.f,0.f,
              0.f,1.f,0.f);

    glRotatef(Yrot,0.f,1.f,0.f);
    glRotatef(Xrot,1.f,0.f,0.f);

    if(Scale<MINSCALE)
        Scale=MINSCALE;
    glScalef(Scale,Scale,Scale);

    // Setup uniforms:
    glm::mat4 view = glm::lookAt(glm::vec3(0.f,2.f,6.f), glm::vec3(0.f,0.f,0.f), glm::vec3(0.f,1.f,0.f));
    glm::mat4 projection;
    if (NowProjection == ORTHO)
        projection = glm::ortho(-2.f,2.f,-2.f,2.f,0.1f,1000.f);
    else
        projection = glm::perspective(glm::radians(70.f),1.f,0.1f,1000.f);

    GLint viewLoc = glGetUniformLocation(shaderProgram,"view");
    GLint projLoc = glGetUniformLocation(shaderProgram,"projection");
    GLint modelLoc = glGetUniformLocation(shaderProgram,"model");
    glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));

    for (int i = 0; i < 9; i++) {
        // Draw base (solid color)
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_FALSE);
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.1f, 0.1f, 0.1f); // Dark gray
        glm::mat4 baseModel = glm::mat4(1.0f);
        baseModel = glm::translate(baseModel, glm::vec3(panelPositionsArr[i].x, 0.0f, panelPositionsArr[i].z - 0.5f));
        baseModel = glm::translate(baseModel, glm::vec3(-0.0f, 0.0f, 1.1f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(baseModel));
        glBindVertexArray(baseVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // Draw panel (solid color)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.2f, 0.2f); // Slightly lighter gray
        float angleDeg = computePanelRotation(panelPositionsArr[i], lightPos);
        glm::mat4 panelModel = glm::translate(glm::mat4(1.0f), panelPositionsArr[i]);
        panelModel = glm::translate(panelModel, glm::vec3(0.0f, 0.6f, 0.0f)); 
        panelModel = glm::rotate(panelModel, glm::radians(angleDeg), glm::vec3(0.f, 0.f, 1.f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(panelModel));
        glBindVertexArray(panelVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Draw grid (black lines on top of the panel)
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.0f, 0.0f); // Black
        glBindVertexArray(panelGridVAO);
        glDrawArrays(GL_LINES, 0, (GLsizei)(panelGridVertices.size() / 3));
    }

    // Draw terrain (textured)
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_TRUE);
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.0f, 0.0f); // Unused when texture is enabled
    glm::mat4 terrainModel = glm::mat4(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(terrainModel));
    glBindVertexArray(terrainVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    DisplayLogsOnScreen();

    // Sun:
    glm::mat4 sunModel=glm::mat4(1.0f);
    sunModel=glm::translate(sunModel,lightPos);
    glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(sunModel));
    glBindVertexArray(sunVAO);
    glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);

    glutSwapBuffers();
    glFlush();
}

void Reset() {
    ActiveButton = 0;
    Scale=1.0f;
    Xrot=Yrot=0.f;
    autoRotate=true;
}

void Keyboard(unsigned char c, int x, int y) {
    switch(c) {
        case 'o':
        case 'O':
            // set Orthographic
            break;
        case 'p':
        case 'P':
            // set Perspective
            break;
        case '1':
            Time = 0.0f;
            autoRotate=false;
            break;
        case '2':
            Time = 0.25f;
            autoRotate=false;
            break;
        case '3':
            Time = 0.5f;
            autoRotate=false;
            break;
        case 'a':
        case 'A':
            autoRotate=true;
            break;
        case 'q':
        case 'Q':
        case ESCAPE:
            glutSetWindow(MainWindow);
            glFinish();
            glutDestroyWindow(MainWindow);
            exit(0);
            break;
    }
    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void MouseButton(int button,int state,int x,int y) {
    int b=0;
    switch(button) {
        case GLUT_LEFT_BUTTON: b=LEFT; break;
        case GLUT_MIDDLE_BUTTON: b=MIDDLE; break;
        case GLUT_RIGHT_BUTTON: b=RIGHT; break;
        case SCROLL_WHEEL_UP:
            Scale += SCLFACT*SCROLL_WHEEL_CLICK_FACTOR;
            if(Scale<MINSCALE)
                Scale=MINSCALE;
            break;
        case SCROLL_WHEEL_DOWN:
            Scale -= SCLFACT*SCROLL_WHEEL_CLICK_FACTOR;
            if(Scale<MINSCALE)
                Scale=MINSCALE;
            break;
    }

    if(state==GLUT_DOWN) {
        Xmouse=x; Ymouse=y;
        ActiveButton |= b;
    } else {
        ActiveButton &= ~b;
    }
    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void MouseMotion(int x,int y) {
    int dx=x - Xmouse;
    int dy=y - Ymouse;

    if((ActiveButton&LEFT)!=0) {
        Xrot += ANGFACT*dy;
        Yrot += ANGFACT*dx;
    }
    if((ActiveButton&MIDDLE)!=0) {
        Scale += SCLFACT*(float)(dx - dy);
        if(Scale<MINSCALE)
            Scale=MINSCALE;
    }

    Xmouse=x;
    Ymouse=y;

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void Resize(int width,int height) {
    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void Visibility(int state) {
    if(state==GLUT_VISIBLE) {
        glutSetWindow(MainWindow);
        glutPostRedisplay();
    }
}

void InitMenus() {
    // Removed menus for simplicity
}

void InitGraphics() {
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowPosition(0,0);
    glutInitWindowSize(INIT_WINDOW_SIZE,INIT_WINDOW_SIZE);

    MainWindow=glutCreateWindow(WINDOWTITLE);
    glutSetWindowTitle(WINDOWTITLE);

    glClearColor(0.,0.,0.,1.);
    glutSetWindow(MainWindow);
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(Keyboard);
    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);
    glutPassiveMotionFunc(MouseMotion);
    glutVisibilityFunc(Visibility);
    glutEntryFunc(NULL);
    glutSpecialFunc(NULL);
    glutIdleFunc(Animate);

    GLenum err=glewInit();
    if(err!=GLEW_OK) {
        fprintf(stderr,"glewInit Error\n");
    } else {
        fprintf(stderr,"GLEW initialized OK\n");
    }
}

int main(int argc,char* argv[]) {
    glutInit(&argc,argv);
    InitGraphics();
    Reset();

    // No menus now
    //InitMenus();

    shaderProgram=buildSimpleShaderProgram();
    glUseProgram(shaderProgram);
    brightnessLoc=glGetUniformLocation(shaderProgram,"brightness");
    glUniform1f(brightnessLoc,1.0f);

    buildPanelGrid();
    setupObjects();

    int width, height, nrChannels;
    unsigned char* data = stbi_load("grass.jpg",&width,&height,&nrChannels,0);
    if(data) {
        glGenTextures(1,&groundTexture);
        glBindTexture(GL_TEXTURE_2D,groundTexture);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D,0,GL_RGB,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    } else {
        fprintf(stderr,"Failed to load texture\n");
    }

    glutSetWindow(MainWindow);
    glutMainLoop();
    return 0;
}
