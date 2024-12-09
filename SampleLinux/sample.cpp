#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifndef F_PI
#define F_PI        ((float)(M_PI))
#define F_2_PI      ((float)(2.f*F_PI))
#define F_PI_2      ((float)(F_PI/2.f))
#endif

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

// title of these windows:
const char *WINDOWTITLE = "OpenGL / GLUT Sample with Modern OpenGL Merged";
const char *GLUITITLE   = "User Interface Window";

const int GLUITRUE  = true;
const int GLUIFALSE = false;

// Escape key:
const int ESCAPE = 0x1b;

// Initial window size:
const int INIT_WINDOW_SIZE = 600;

// Multiplication factors for input interaction:
const float ANGFACT = 1.f;
const float SCLFACT = 0.005f;

// Minimum allowable scale factor:
const float MINSCALE = 0.05f;

// Scroll wheel values:
const int SCROLL_WHEEL_UP   = 3;
const int SCROLL_WHEEL_DOWN = 4;
const float SCROLL_WHEEL_CLICK_FACTOR = 5.f;

// Mouse button masks:
const int LEFT   = 4;
const int MIDDLE = 2;
const int RIGHT  = 1;

// Which projection:
enum Projections
{
    ORTHO,
    PERSP
};

// Which button:
enum ButtonVals
{
    RESET,
    QUIT
};

// Panel positions:
enum PanelPositions
{
    PANEL_POS_1 = 0,
    PANEL_POS_2,
    PANEL_POS_3
};

// 9 panel positions in a 3x3 grid:
glm::vec3 panelPositionsArr[] = {
    glm::vec3(-2.0f,0.5f,-2.0f),
    glm::vec3( 0.0f,0.5f,-2.0f),
    glm::vec3( 2.0f,0.5f,-2.0f),

    glm::vec3(-2.0f,0.5f, 0.0f),
    glm::vec3( 0.0f,0.5f, 0.0f),
    glm::vec3( 2.0f,0.5f, 0.0f),

    glm::vec3(-2.0f,0.5f, 2.0f),
    glm::vec3( 0.0f,0.5f, 2.0f),
    glm::vec3( 2.0f,0.5f, 2.0f)
};


// window background color (rgba):
const GLfloat BACKCOLOR[ ] = { 0., 0., 0., 1. };

// line width for the axes:
const GLfloat AXES_WIDTH   = 3.;

// the color numbers:
enum Colors
{
    RED,
    YELLOW,
    GREEN,
    CYAN,
    BLUE,
    MAGENTA
};

char * ColorNames[ ] =
{
    (char *)"Red",
    (char*)"Yellow",
    (char*)"Green",
    (char*)"Cyan",
    (char*)"Blue",
    (char*)"Magenta"
};

const GLfloat Colors[ ][3] = 
{
    { 1., 0., 0. },     // red
    { 1., 1., 0. },     // yellow
    { 0., 1., 0. },     // green
    { 0., 1., 1. },     // cyan
    { 0., 0., 1. },     // blue
    { 1., 0., 1. },     // magenta
};

// fog parameters:
const GLfloat FOGCOLOR[4] = { .0f, .0f, .0f, 1.f };
const GLenum  FOGMODE     = GL_LINEAR;
const GLfloat FOGDENSITY  = 0.30f;
const GLfloat FOGSTART    = 1.5f;
const GLfloat FOGEND      = 4.f;

// for lighting:
const float WHITE[ ] = { 1.,1.,1.,1. };

// for animation:
const int MS_PER_CYCLE = 10000;     // 10 seconds for a full cycle

// non-constant global variables:
int     ActiveButton;           // current button that is down
GLuint  AxesList;               // list to hold the axes
int     AxesOn;                 // != 0 means to draw the axes
GLuint  PanelList;                // object display list

int     DebugOn;                // != 0 means to print debugging info
int     DepthCueOn;             // != 0 means to use intensity depth cueing
int     DepthBufferOn;          // != 0 means to use the z-buffer
int     DepthFightingOn;        // != 0 means to force z-fighting
int     MainWindow;             // window id for main graphics window
int     NowColor;               // index into Colors[ ]
int     NowProjection;          // ORTHO or PERSP
float   Scale;                  // scaling factor
int     ShadowsOn;              // != 0 means to turn shadows on
float   Time;                   // used for animation
int     Xmouse, Ymouse;         // mouse values
float   Xrot, Yrot;             // rotation angles in degrees
GLuint groundTexture;

bool autoRotate = true; // true means animate automatically

// For shadow mapping
GLuint depthMapFBO;
GLuint depthMap;
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;

// We'll have a separate shader program for depth pass
GLuint depthShaderProgram;

// Matrices for light's perspective
glm::mat4 lightSpaceMatrix;

int CurrentPanelPosition = PANEL_POS_1;

// NEW: We'll simulate the sun as a light that orbits around the Y-axis.
// We'll use `Time` from Animate() to compute its position.
float SunRadius = 5.0f;
float SunHeight = 3.0f;

// Modern OpenGL objects:
GLuint depth_vs;
GLuint depth_fs;
GLuint shaderProgram;
GLint modelLoc, viewLoc, projLoc, objectColorLoc, lightColorLoc;

// VAOs and VBOs:
GLuint terrainVAO, terrainVBO;
GLuint panelVAO, panelVBO;
GLuint panelGridVAO, panelGridVBO; // If we choose to add grid lines
GLuint baseVAO, baseVBO, baseEBO;
GLuint sunVAO, sunVBO, sunEBO;

// We will store geometry here (taken from modern example):
float terrainVertices[] = {
    //  x,    y,    z,     u,    v
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
    // Same base cube vertices from previous snippet ...
    -0.05f, -0.5f,  0.05f,
     0.05f, -0.5f,  0.05f,
     0.05f,  0.0f,  0.05f,
    -0.05f,  0.0f,  0.05f,

    -0.05f, -0.5f, -0.05f,
    -0.05f,  0.0f, -0.05f,
     0.05f,  0.0f, -0.05f,
     0.05f, -0.5f, -0.05f,

    -0.05f, -0.5f, -0.05f,
    -0.05f, -0.5f,  0.05f,
    -0.05f,  0.0f,  0.05f,
    -0.05f,  0.0f, -0.05f,

     0.05f, -0.5f, -0.05f,
     0.05f,  0.0f, -0.05f,
     0.05f,  0.0f,  0.05f,
     0.05f, -0.5f,  0.05f,

    -0.05f,  0.0f,  0.05f,
     0.05f,  0.0f,  0.05f,
     0.05f,  0.0f, -0.05f,
    -0.05f,  0.0f, -0.05f,

    -0.05f, -0.5f,  0.05f,
    -0.05f, -0.5f, -0.05f,
     0.05f, -0.5f, -0.05f,
     0.05f, -0.5f,  0.05f
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
    -0.1f,-0.1f,0.1f,  0.1f,-0.1f,0.1f,  0.1f,0.1f,0.1f,  -0.1f,0.1f,0.1f,
    -0.1f,-0.1f,-0.1f,-0.1f,0.1f,-0.1f, 0.1f,0.1f,-0.1f, 0.1f,-0.1f,-0.1f,
    -0.1f,-0.1f,-0.1f,-0.1f,-0.1f,0.1f,-0.1f,0.1f,0.1f, -0.1f,0.1f,-0.1f,
    0.1f,-0.1f,-0.1f, 0.1f,0.1f,-0.1f, 0.1f,0.1f,0.1f,  0.1f,-0.1f,0.1f,
    -0.1f,0.1f,0.1f,  0.1f,0.1f,0.1f,  0.1f,0.1f,-0.1f, -0.1f,0.1f,-0.1f,
    -0.1f,-0.1f,0.1f,-0.1f,-0.1f,-0.1f,0.1f,-0.1f,-0.1f,0.1f,-0.1f,0.1f
};

unsigned int cubeIndices[] = {
    0,1,2,2,3,0,
    4,5,6,6,7,4,
    8,9,10,10,11,8,
    12,13,14,14,15,12,
    16,17,18,18,19,16,
    20,21,22,22,23,20
};

// Shaders

const char* depth_vs_source = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main() {
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}

)";
GLuconst char* depth_fs_source

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main(){
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}

)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform sampler2D shadowMap;
uniform vec3 lightDir;
uniform vec3 objectColor;
uniform vec3 lightColor;

float ShadowCalculation(vec4 fragPosLightSpace) {
    // Transform to normalized device coordinates
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5; // Transform to [0, 1]

    // Sample the shadow map
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    // Check if in shadow
    float shadow = currentDepth > closestDepth + 0.005 ? 1.0 : 0.0; // Add bias to avoid artifacts
    return shadow;
}

void main() {
    float shadow = ShadowCalculation(FragPosLightSpace);

    vec3 ambient = 0.3 * objectColor;
    vec3 diffuse = max(dot(lightDir, normalize(FragPosLightSpace.xyz)), 0.0) * lightColor * objectColor;
    vec3 result = ambient + (1.0 - shadow) * diffuse;
    FragColor = vec4(result, 1.0);
}
)";

// Function prototypes:
void    Animate( );
void    Display( );
void    DoAxesMenu( int );
void    DoColorMenu( int );
void    DoDepthBufferMenu( int );
void    DoDepthFightingMenu( int );
void    DoDepthMenu( int );
void    DoDebugMenu( int );
void    DoMainMenu( int );
void    DoProjectMenu( int );
void    DoRasterString( float, float, float, char * );
void    DoStrokeString( float, float, float, float, char * );
float   ElapsedSeconds( );
void    InitGraphics( );
void	InitLists( );
void    InitMenus( );
void    Keyboard( unsigned char, int, int );
void    MouseButton( int, int, int, int );
void    MouseMotion( int, int );
void    Reset( );
void    Resize( int, int );
void    Visibility( int );
void    Axes( float );
void    HsvRgb( float[3], float[3] );
void    Cross(float[3], float[3], float[3]);
float   Dot(float[3], float[3]);
float   Unit(float[3], float[3]);
float   Unit(float[3]);
static void buildShaderProgram();
static void setupObjects();


float*  Array3(float a,float b,float c);
float*  MulArray3(float factor,float array0[]);
float*  MulArray3(float factor,float a,float b,float c);

// Compute angle for panel rotation as before:
float computePanelRotation(const glm::vec3& panelPos, const glm::vec3& lightPos) {
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

std::vector<GLfloat> panelGridVertices;

void buildPanelGrid() {
    float xs[5] = {-0.5f, -0.25f, 0.0f, 0.25f, 0.5f};
    float zs[5] = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f};

    // Vertical lines:
    for (int i=0; i<5; i++){
        // line from (xs[i],0.001f,0.0f) to (xs[i],0.001f,1.0f)
        panelGridVertices.push_back(xs[i]); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(0.0f);
        panelGridVertices.push_back(xs[i]); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(1.0f);
    }

    // Horizontal lines:
    for (int i=0; i<5; i++){
        // line from (-0.5,0.001f,zs[i]) to (0.5,0.001f,zs[i])
        panelGridVertices.push_back(-0.5f); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(zs[i]);
        panelGridVertices.push_back( 0.5f); panelGridVertices.push_back(0.001f); panelGridVertices.push_back(zs[i]);
    }
}


// LOGGING
#include <fstream>
#include <chrono>

struct PanelLog {
    int panelID;             // Unique ID for each panel
    glm::vec3 position;      // Panel position
    float timeStamp;         // Time in seconds
    float sunlightStrength;  // Strength of sunlight on the panel

    PanelLog(int id, glm::vec3 pos, float time, float strength)
        : panelID(id), position(pos), timeStamp(time), sunlightStrength(strength) {}
};


std::vector<PanelLog> panelLogs;

float calculateSunlightStrength(const glm::vec3& panelPos, const glm::vec3& lightPos) {
    glm::vec3 panelNormal(0.0f, 1.0f, 0.0f); // Assume panels are flat and face upwards
    glm::vec3 dirToLight = glm::normalize(lightPos - panelPos);

    float strength = glm::dot(panelNormal, dirToLight); // Cosine of the angle
    return glm::max(strength, 0.0f); // Clamp to [0.0, 1.0]
}
void logPanelPositions(const glm::vec3& lightPos) {
    float currentTime = ElapsedSeconds();
    for (int i = 0; i < 9; ++i) {
        float sunlightStrength = calculateSunlightStrength(panelPositionsArr[i], lightPos);
        panelLogs.push_back({i + 1, panelPositionsArr[i], currentTime, sunlightStrength}); // Panel ID starts from 1
    }

    // Optionally write to a file
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



// display logs on screen
void renderText(float x, float y, const char* text) {
    glRasterPos2f(x, y);
    while (*text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *text++);
    }
}

int
main(int argc,char* argv[])
{
    glutInit(&argc,argv);
    InitGraphics();
    Reset();
    InitMenus();

    // After GLUT window is created, initialize GLEW and build shaders:
    buildShaderProgram();
    buildPanelGrid();
	setupObjects(); // after building the panel grid

    int width, height, nrChannels;
    unsigned char* data = stbi_load("grass.jpg", &width, &height, &nrChannels, 0);
    if(data) {
        glGenTextures(1, &groundTexture);
        glBindTexture(GL_TEXTURE_2D, groundTexture);

        // set wrapping/filtering
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // upload texture data
        // If your grass.jpg is RGB, use GL_RGB. If it's RGBA, use GL_RGBA.
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    } else {
        fprintf(stderr, "Failed to load texture\n");
    }

    InitLists();
    glutSetWindow(MainWindow);
    glutMainLoop();
    return 0;
}



static void buildShaderProgram() {
    GLint textureLoc = glGetUniformLocation(shaderProgram, "texture1");
    glUseProgram(shaderProgram);
    glUniform1i(textureLoc, 0); // texture unit 0


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

    shaderProgram=glCreateProgram();
    glAttachShader(shaderProgram,vertexShader);
    glAttachShader(shaderProgram,fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram,GL_LINK_STATUS,&success);
    if(!success){
        glGetProgramInfoLog(shaderProgram,512,NULL,infoLog);
        fprintf(stderr,"Program Linking Error: %s\n",infoLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations:
    modelLoc = glGetUniformLocation(shaderProgram,"model");
    viewLoc = glGetUniformLocation(shaderProgram,"view");
    projLoc = glGetUniformLocation(shaderProgram,"projection");
    objectColorLoc = glGetUniformLocation(shaderProgram,"objectColor");
    lightColorLoc = glGetUniformLocation(shaderProgram,"lightColor");
}

// Create VAOs/VBOs for objects:
static void setupObjects()
{
    // terrain
    glGenVertexArrays(1,&terrainVAO);
    glGenBuffers(1,&terrainVBO);
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER,terrainVBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(terrainVertices),terrainVertices,GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,5*sizeof(float),(void*)0);
    glEnableVertexAttribArray(0);

    // texture coord attribute
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

	glGenVertexArrays(1, &panelGridVAO);
	glGenBuffers(1, &panelGridVBO);
	glBindVertexArray(panelGridVAO);
	glBindBuffer(GL_ARRAY_BUFFER, panelGridVBO);
	glBufferData(GL_ARRAY_BUFFER, panelGridVertices.size()*sizeof(float), panelGridVertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

}

void Animate() {
    static float lastLogTime = 0.0f;
    float currentTime = ElapsedSeconds();

    // Log every 2 seconds
    if (currentTime - lastLogTime > 2.0f) {
        float angle = Time * 2.0f * M_PI;
        glm::vec3 lightPos = glm::vec3(cos(angle) * SunRadius, SunHeight, sin(angle) * SunRadius);
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
    // Disable depth testing to draw text over everything
    glDisable(GL_DEPTH_TEST);

    // Set up orthographic projection to align with the map's visible area
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    // Assume the map extends from -5 to 5 in X and Z
    // Adjust these bounds to match your visible map area
    gluOrtho2D(-5.0, 5.0, -5.0, 5.0); 

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Set text color (white)
    glColor3f(1.0f, 1.0f, 1.0f);

    // Define the starting position for the logs (relative to map coordinates)
    float startX = -60; // Slightly inset from the map's left edge
    float startY = 4.5f;  // Start near the top of the map
    char buffer[256];

    // Loop through the logs and render them
    for (size_t i = 0; i < panelLogs.size() && i < 10; ++i) { // Display up to 10 logs
        snprintf(buffer, sizeof(buffer), "Panel ID: %d, Time: %.2fs, Position: (%.2f, %.2f, %.2f), Sunlight: %.2f",
                 panelLogs[i].panelID, panelLogs[i].timeStamp, panelLogs[i].position.x, 
                 panelLogs[i].position.y, panelLogs[i].position.z, panelLogs[i].sunlightStrength);
        renderText(startX, startY - (i * 0.5f), buffer); // Decrease Y for each log line
    }

    // Restore the previous projection and modelview matrices
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    // Re-enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void
Display()
{
    if (DebugOn != 0)
        fprintf(stderr, "Starting Display.\n");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, groundTexture);

    glutSetWindow( MainWindow );
    glDrawBuffer( GL_BACK );
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable( GL_DEPTH_TEST );

    glShadeModel( GL_FLAT );

    GLsizei vx = glutGet( GLUT_WINDOW_WIDTH );
    GLsizei vy = glutGet( GLUT_WINDOW_HEIGHT );
    GLsizei v = vx < vy ? vx : vy;
    GLint xl = ( vx - v ) / 2;
    GLint yb = ( vy - v ) / 2;
    glViewport( xl, yb,  v, v );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
    if( NowProjection == ORTHO )
        glOrtho( -2.f, 2.f, -2.f, 2.f, 0.1f, 1000.f );
    else
        gluPerspective( 70.f, 1.f, 0.1f, 1000.f );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    gluLookAt(0.f,2.f,6.f, 0.f,0.f,0.f, 0.f,1.f,0.f);

    glRotatef((GLfloat)Yrot,0.f,1.f,0.f);
    glRotatef((GLfloat)Xrot,1.f,0.f,0.f);

    if(Scale<MINSCALE)
        Scale=MINSCALE;
    glScalef((GLfloat)Scale,(GLfloat)Scale,(GLfloat)Scale);

    if(DepthCueOn!=0) {
        glFogi(GL_FOG_MODE,FOGMODE);
        glFogfv(GL_FOG_COLOR,FOGCOLOR);
        glFogf(GL_FOG_DENSITY,FOGDENSITY);
        glFogf(GL_FOG_START,FOGSTART);
        glFogf(GL_FOG_END,FOGEND);
        glEnable(GL_FOG);
    }else{
        glDisable(GL_FOG);
    }

    if(AxesOn!=0){
        glColor3fv(&Colors[NowColor][0]);
        glCallList(AxesList);
    }
    glEnable(GL_NORMALIZE);

    // Compute sun position:
	float angle = Time * 2.0f * M_PI;
	float radAngle = angle;
	glm::vec3 lightPos(cos(radAngle)*SunRadius, sin(radAngle)*SunRadius, 0.0f);

    // Use modern pipeline for drawing terrain, panel, etc.:
    glUseProgram(shaderProgram);

    // Set up camera via glm:
    glm::mat4 view = glm::lookAt(glm::vec3(0.f,2.f,6.f),
                                 glm::vec3(0.f,0.f,0.f),
                                 glm::vec3(0.f,1.f,0.f));
    glm::mat4 projection;
    if(NowProjection==ORTHO){
        projection = glm::ortho(-2.f,2.f,-2.f,2.f,0.1f,1000.f);
    }else{
        projection = glm::perspective(glm::radians(70.f),1.f,0.1f,1000.f);
    }

    glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
    glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(projection));

    glUniform3f(lightColorLoc,1.0f,1.0f,0.8f);

    
    // For terrain (textured ground):
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_TRUE);

    // Draw terrain
    glm::mat4 model=glm::mat4(1.0f);
    glUniform3f(objectColorLoc,0.2f,0.6f,0.2f);
    glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));
    glBindVertexArray(terrainVAO);
    glDrawArrays(GL_TRIANGLES,0,6);


	// Draw bases and panels for all 9 panels:
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), GL_FALSE);


	glUniform3f(objectColorLoc,0.1f,0.1f,0.1f);
	for(int i=0; i<9; i++){
		// Draw the base
		glm::mat4 baseModel=glm::mat4(1.0f);
		baseModel=glm::translate(baseModel, glm::vec3(panelPositionsArr[i].x, 0.0f, panelPositionsArr[i].z));
		glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(baseModel));
		glBindVertexArray(baseVAO);
		glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);

		// Compute panel rotation
		float angleDeg=computePanelRotation(panelPositionsArr[i], lightPos);

		// Draw panel
		glm::mat4 panelModel=glm::mat4(1.0f);
		panelModel=glm::translate(panelModel, panelPositionsArr[i]);
		panelModel=glm::rotate(panelModel, glm::radians(angleDeg), glm::vec3(0.0f,0.0f,1.0f));
		glUniform3f(objectColorLoc,0.2f,0.2f,0.5f);
		glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(panelModel));
		glBindVertexArray(panelVAO);
		glDrawArrays(GL_TRIANGLES,0,6);

		// Draw the grid on top of the panel
		glUniform3f(objectColorLoc, 0.0f, 0.0f, 0.0f); // black lines
		// Same panelModel, so no changes needed to model matrix:
		glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(panelModel));
		glBindVertexArray(panelGridVAO);
		glDrawArrays(GL_LINES, 0, (GLsizei)(panelGridVertices.size()/3));

	}

    DisplayLogsOnScreen();

    // Draw sun:
    {
        glm::mat4 sunModel=glm::mat4(1.0f);
        sunModel=glm::translate(sunModel,lightPos);
        glUniform3f(objectColorLoc,1.0f,1.0f,0.0f);
        glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(sunModel));
        glBindVertexArray(sunVAO);
        glDrawElements(GL_TRIANGLES,36,GL_UNSIGNED_INT,0);
    }

    // Swap buffers:
    glutSwapBuffers();
    glFlush();


}


void
DoAxesMenu( int id )
{
    AxesOn = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoColorMenu( int id )
{
    NowColor = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoDebugMenu( int id )
{
    DebugOn = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoDepthBufferMenu( int id )
{
    DepthBufferOn = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoDepthFightingMenu( int id )
{
    DepthFightingOn = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoDepthMenu( int id )
{
    DepthCueOn = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoMainMenu( int id )
{
    switch( id )
    {
        case RESET:
            Reset( );
            break;

        case QUIT:
            glutSetWindow( MainWindow );
            glFinish( );
            glutDestroyWindow( MainWindow );
            exit( 0 );
            break;

        default:
            fprintf( stderr, "Don't know what to do with Main Menu ID %d\n", id );
    }

    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoProjectMenu( int id )
{
    NowProjection = id;
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
DoRasterString( float x, float y, float z, char *s )
{
    glRasterPos3f( (GLfloat)x, (GLfloat)y, (GLfloat)z );
    char c;
    for( ; ( c = *s ) != '\0'; s++ )
    {
        glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_24, c );
    }
}

void
DoStrokeString( float x, float y, float z, float ht, char *s )
{
    glPushMatrix( );
        glTranslatef( (GLfloat)x, (GLfloat)y, (GLfloat)z );
        float sf = ht / ( 119.05f + 33.33f );
        glScalef( (GLfloat)sf, (GLfloat)sf, (GLfloat)sf );
        char c;
        for( ; ( c = *s ) != '\0'; s++ )
        {
            glutStrokeCharacter( GLUT_STROKE_ROMAN, c );
        }
    glPopMatrix( );
}

float
ElapsedSeconds( )
{
    int ms = glutGet( GLUT_ELAPSED_TIME );
    return (float)ms / 1000.f;
}

void
InitMenus( )
{
    glutSetWindow( MainWindow );

    int numColors = sizeof( Colors ) / ( 3*sizeof(float) );
    int colormenu = glutCreateMenu( DoColorMenu );
    for( int i = 0; i < numColors; i++ )
    {
        glutAddMenuEntry( ColorNames[i], i );
    }

    int axesmenu = glutCreateMenu( DoAxesMenu );
    glutAddMenuEntry( "Off",  0 );
    glutAddMenuEntry( "On",   1 );

    int depthcuemenu = glutCreateMenu( DoDepthMenu );
    glutAddMenuEntry( "Off",  0 );
    glutAddMenuEntry( "On",   1 );

#ifdef DEMO_DEPTH_BUFFER
    int depthbuffermenu = glutCreateMenu( DoDepthBufferMenu );
    glutAddMenuEntry( "Off",  0 );
    glutAddMenuEntry( "On",   1 );
#endif

#ifdef DEMO_Z_FIGHTING
    int depthfightingmenu = glutCreateMenu( DoDepthFightingMenu );
    glutAddMenuEntry( "Off",  0 );
    glutAddMenuEntry( "On",   1 );
#endif

    int projmenu = glutCreateMenu( DoProjectMenu );
    glutAddMenuEntry( "Orthographic",  ORTHO );
    glutAddMenuEntry( "Perspective",   PERSP );

    int mainmenu = glutCreateMenu( DoMainMenu );
    glutAddSubMenu(   "Axes",          axesmenu);
    glutAddSubMenu(   "Axis Colors",   colormenu);
#ifdef DEMO_DEPTH_BUFFER
    glutAddSubMenu(   "Depth Buffer",  depthbuffermenu);
#endif
#ifdef DEMO_Z_FIGHTING
    glutAddSubMenu(   "Depth Fighting",depthfightingmenu);
#endif
    glutAddSubMenu(   "Depth Cue",     depthcuemenu);
    glutAddSubMenu(   "Projection",    projmenu );
    glutAddMenuEntry( "Reset",         RESET );
    glutAddMenuEntry( "Quit",          QUIT );

    glutAttachMenu( GLUT_RIGHT_BUTTON );
}

void
InitGraphics( )
{
    glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
    glutInitWindowPosition( 0, 0 );
    glutInitWindowSize( INIT_WINDOW_SIZE, INIT_WINDOW_SIZE );

    MainWindow = glutCreateWindow( WINDOWTITLE );
    glutSetWindowTitle( WINDOWTITLE );

    glClearColor( BACKCOLOR[0], BACKCOLOR[1], BACKCOLOR[2], BACKCOLOR[3] );

    glutSetWindow( MainWindow );
    glutDisplayFunc( Display );
    glutReshapeFunc( Resize );
    glutKeyboardFunc( Keyboard );
    glutMouseFunc( MouseButton );
    glutMotionFunc( MouseMotion );
    glutPassiveMotionFunc(MouseMotion);
    glutVisibilityFunc( Visibility );
    glutEntryFunc( NULL );
    glutSpecialFunc( NULL );
    glutSpaceballMotionFunc( NULL );
    glutSpaceballRotateFunc( NULL );
    glutSpaceballButtonFunc( NULL );
    glutButtonBoxFunc( NULL );
    glutDialsFunc( NULL );
    glutTabletMotionFunc( NULL );
    glutTabletButtonFunc( NULL );
    glutMenuStateFunc( NULL );
    glutTimerFunc( -1, NULL, 0 );
    glutIdleFunc( Animate );


    GLenum err = glewInit( );
    if( err != GLEW_OK )
    {
        fprintf( stderr, "glewInit Error\n" );
    }
    else
        fprintf( stderr, "GLEW initialized OK\n" );
    fprintf( stderr, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));

}

void
InitLists( )
{
    glutSetWindow( MainWindow );

	float panelWidth = 1.0f;  // width from -0.5 to 0.5 in X
	float panelHeight = 1.0f; // height from 0.0 to 1.0 in Z
	float halfWidth = panelWidth * 0.5f;

    AxesList = glGenLists(1);
    glNewList( AxesList, GL_COMPILE );
        glLineWidth( AXES_WIDTH );
        Axes( 1.5 );
        glLineWidth( 1. );
    glEndList();
}

void
Keyboard( unsigned char c, int x, int y )
{
    if( DebugOn != 0 )
        fprintf( stderr, "Keyboard: '%c' (0x%0x)\n", c, c );

    switch( c )
    {
        case 'o':
        case 'O':
            NowProjection = ORTHO;
            break;

        case 'p':
        case 'P':
            NowProjection = PERSP;
            break;

        case '1':
            // Set Time to 0.0f => Sun at start (east)
            Time = 0.0f;
            autoRotate = false;
            break;

        case '2':
            // Middle of the cycle => overhead (π/2)
            Time = 0.25f;
            autoRotate = false;
            break;

        case '3':
            // End => west side (π)
            Time = 0.5f;
            autoRotate = false;
            break;

        case 'a':
        case 'A':
            // Resume auto rotation
            autoRotate = true;
            break;

        case 'q':
        case 'Q':
        case ESCAPE:
            DoMainMenu( QUIT ); 
            break;

        default:
            fprintf( stderr, "Don't know what to do with keyboard hit: '%c' (0x%0x)\n", c, c );
    }

    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
MouseButton( int button, int state, int x, int y )
{
    int b = 0;

    if( DebugOn != 0 )
        fprintf( stderr, "MouseButton: %d, %d, %d, %d\n", button, state, x, y );

    switch( button )
    {
        case GLUT_LEFT_BUTTON:
            b = LEFT;      break;
        case GLUT_MIDDLE_BUTTON:
            b = MIDDLE;    break;
        case GLUT_RIGHT_BUTTON:
            b = RIGHT;     break;
        case SCROLL_WHEEL_UP:
            Scale += SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
            if (Scale < MINSCALE)
                Scale = MINSCALE;
            break;
        case SCROLL_WHEEL_DOWN:
            Scale -= SCLFACT * SCROLL_WHEEL_CLICK_FACTOR;
            if (Scale < MINSCALE)
                Scale = MINSCALE;
            break;
        default:
            b = 0;
            fprintf( stderr, "Unknown mouse button: %d\n", button );
    }

    if( state == GLUT_DOWN )
    {
        Xmouse = x;
        Ymouse = y;
        ActiveButton |= b;
    }
    else
    {
        ActiveButton &= ~b;
    }

    glutSetWindow(MainWindow);
    glutPostRedisplay();
}

void
MouseMotion( int x, int y )
{
    int dx = x - Xmouse;
    int dy = y - Ymouse;

    if( ( ActiveButton & LEFT ) != 0 )
    {
        Xrot += ( ANGFACT*dy );
        Yrot += ( ANGFACT*dx );
    }

    if( ( ActiveButton & MIDDLE ) != 0 )
    {
        Scale += SCLFACT * (float) ( dx - dy );
        if( Scale < MINSCALE )
            Scale = MINSCALE;
    }

    Xmouse = x;
    Ymouse = y;

    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
Reset( )
{
    ActiveButton = 0;
    AxesOn = 1;
    DebugOn = 0;
    DepthBufferOn = 1;
    DepthFightingOn = 0;
    DepthCueOn = 0;
    Scale  = 1.0;
    ShadowsOn = 0;
    NowColor = YELLOW;
    NowProjection = PERSP;
    Xrot = Yrot = 0.;
    CurrentPanelPosition = PANEL_POS_1;
}

void
Resize( int width, int height )
{
    glutSetWindow( MainWindow );
    glutPostRedisplay( );
}

void
Visibility ( int state )
{
    if( DebugOn != 0 )
        fprintf( stderr, "Visibility: %d\n", state );

    if( state == GLUT_VISIBLE )
    {
        glutSetWindow( MainWindow );
        glutPostRedisplay( );
    }
}

///////////////////////////////////////   HANDY UTILITIES:  //////////////////////////

static float xx[ ] = { 0.f, 1.f, 0.f, 1.f };
static float xy[ ] = { -.5f, .5f, .5f, -.5f };
static int xorder[ ] = { 1, 2, -3, 4 };

static float yx[ ] = { 0.f, 0.f, -.5f, .5f };
static float yy[ ] = { 0.f, .6f, 1.f, 1.f };
static int yorder[ ] = { 1, 2, 3, -2, 4 };

static float zx[ ] = { 1.f, 0.f, 1.f, 0.f, .25f, .75f };
static float zy[ ] = { .5f, .5f, -.5f, -.5f, 0.f, 0.f };
static int zorder[ ] = { 1, 2, 3, 4, -5, 6 };

const float LENFRAC = 0.10f;
const float BASEFRAC = 1.10f;

void
Axes( float length )
{
    glBegin( GL_LINE_STRIP );
        glVertex3f( length, 0., 0. );
        glVertex3f( 0., 0., 0. );
        glVertex3f( 0., length, 0. );
    glEnd( );
    glBegin( GL_LINE_STRIP );
        glVertex3f( 0., 0., 0. );
        glVertex3f( 0., 0., length );
    glEnd( );

    float fact = LENFRAC * length;
    float base = BASEFRAC * length;

    glBegin( GL_LINE_STRIP );
        for( int i = 0; i < 4; i++ )
        {
            int j = xorder[i];
            if( j < 0 )
            {
                glEnd( );
                glBegin( GL_LINE_STRIP );
                j = -j;
            }
            j--;
            glVertex3f( base + fact*xx[j], fact*xy[j], 0.0 );
        }
    glEnd( );

    glBegin( GL_LINE_STRIP );
        for( int i = 0; i < 5; i++ )
        {
            int j = yorder[i];
            if( j < 0 )
            {
                glEnd( );
                glBegin( GL_LINE_STRIP );
                j = -j;
            }
            j--;
            glVertex3f( fact*yx[j], base + fact*yy[j], 0.0 );
        }
    glEnd( );

    glBegin( GL_LINE_STRIP );
        for( int i = 0; i < 6; i++ )
        {
            int j = zorder[i];
            if( j < 0 )
            {
                glEnd( );
                glBegin( GL_LINE_STRIP );
                j = -j;
            }
            j--;
            glVertex3f( 0.0, fact*zy[j], base + fact*zx[j] );
        }
    glEnd( );
}

void
HsvRgb( float hsv[3], float rgb[3] )
{
    float h = hsv[0] / 60.f;
    while( h >= 6. )    h -= 6.;
    while( h <  0. )    h += 6.;

    float s = hsv[1];
    if( s < 0. )
        s = 0.;
    if( s > 1. )
        s = 1.;

    float v = hsv[2];
    if( v < 0. )
        v = 0.;
    if( v > 1. )
        v = 1.;

    if( s == 0.0 )
    {
        rgb[0] = rgb[1] = rgb[2] = v;
        return;
    }

    float i = (float)floor( h );
    float f = h - i;
    float p = v * ( 1.f - s );
    float q = v * ( 1.f - s*f );
    float t = v * ( 1.f - ( s * (1.f-f) ) );

    float r=0., g=0., b=0.;
    switch( (int) i )
    {
        case 0:
            r = v; g = t; b = p;
            break;

        case 1:
            r = q; g = v; b = p;
            break;

        case 2:
            r = p; g = v; b = t;
            break;

        case 3:
            r = p; g = q; b = v;
            break;

        case 4:
            r = t; g = p; b = v;
            break;

        case 5:
            r = v; g = p; b = q;
            break;
    }

    rgb[0] = r;
    rgb[1] = g;
    rgb[2] = b;
}

void
Cross(float v1[3], float v2[3], float vout[3])
{
    float tmp[3];
    tmp[0] = v1[1] * v2[2] - v2[1] * v1[2];
    tmp[1] = v2[0] * v1[2] - v1[0] * v2[2];
    tmp[2] = v1[0] * v2[1] - v2[0] * v1[1];
    vout[0] = tmp[0];
    vout[1] = tmp[1];
    vout[2] = tmp[2];
}

float
Dot(float v1[3], float v2[3])
{
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

float
Unit(float vin[3], float vout[3])
{
    float dist = vin[0]*vin[0] + vin[1]*vin[1] + vin[2]*vin[2];
    if(dist > 0.0)
    {
        dist = sqrtf(dist);
        vout[0] = vin[0]/dist;
        vout[1] = vin[1]/dist;
        vout[2] = vin[2]/dist;
    }
    else
    {
        vout[0] = vin[0];
        vout[1] = vin[1];
        vout[2] = vin[2];
    }
    return dist;
}

float
Unit(float v[3])
{
    float dist = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
    if(dist > 0.0)
    {
        dist = sqrtf(dist);
        v[0]/=dist;
        v[1]/=dist;
        v[2]/=dist;
    }
    return dist;
}


float *
Array3( float a, float b, float c )
{
    static float array[4];
    array[0] = a;
    array[1] = b;
    array[2] = c;
    array[3] = 1.;
    return array;
}

float *
MulArray3( float factor, float array0[] )
{
    static float array[4];
    array[0] = factor * array0[0];
    array[1] = factor * array0[1];
    array[2] = factor * array0[2];
    array[3] = 1.;
    return array;
}

float *
MulArray3(float factor, float a, float b, float c )
{
    static float array[4];
    float* abc = Array3(a, b, c);
    array[0] = factor * abc[0];
    array[1] = factor * abc[1];
    array[2] = factor * abc[2];
    array[3] = 1.;
    return array;
}
