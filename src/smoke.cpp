#include "common.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <random>


#define STB_IMAGE_IMPLEMENTATION 
#include "stb_image.h"
using namespace std;

const char* WINDOW_TITLE = "Smoke Simulation";
const double FRAME_RATE_MS = 1000.0 / 60.0;

typedef glm::vec4  color4;
typedef glm::vec4  point4;
typedef glm::vec3 colour3;

GLuint program, vPosition, texture[2], texPosition, vao[1], indexBuff[1];
GLuint Window, Texture;
GLuint velocityTexture, densityTexture;

std::random_device rd;
std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
std::uniform_real_distribution<> dis(50.0, 150.0); // Range [0.0, 1.0)

// Generate a random floating-point value
double randomValue = dis(gen);

GLuint deltaTime;
float timeStamp = 0.0;

const int NumQuadPoints = 4;
const float QuadSize = 2.0;
const float EdgeCoord = QuadSize / 2.0;

const int DIMENSION = 100;
const int NUMVALUES = DIMENSION * DIMENSION;
colour3 velocityTexValues[NUMVALUES];
float densityTexValues[NUMVALUES];
float velocityX[NUMVALUES];
float velocityY[NUMVALUES];
float velocityX0[NUMVALUES];
float velocityY0[NUMVALUES];
float density0[NUMVALUES];

float diffusion = 0.0f;
float viscocity = 0.0000001;
float dt = 0.2;


point4 cubeVertexPoints[NumQuadPoints] =
{
    point4(-EdgeCoord, EdgeCoord, 0.0, 1.0),
    point4(-EdgeCoord, -EdgeCoord, 0.0, 1.0),
    point4(EdgeCoord, -EdgeCoord, 0.0, 1.0),
    point4(EdgeCoord, EdgeCoord, 0.0, 1.0),
};

GLuint cubeIndices[NumQuadPoints + 2] = { 0, 1, 2, 2, 3, 0 };

glm::vec2 cubeTexPoints[NumQuadPoints] =
{
    glm::vec2(0.0, 1.0),
    glm::vec2(0.0, 0.0),
    glm::vec2(1.0, 0.0),
    glm::vec2(1.0, 1.0)
};

int IX(int x, int y)
{
    return (x * DIMENSION) + y;
}

// OpenGL initialization
void
init()
{
    glLineWidth(2.0f);
    
    glGenTextures(2, texture);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    program = InitShader("v.glsl", "f.glsl");
    vPosition = glGetAttribLocation(program, "vPosition");
    texPosition = glGetAttribLocation(program, "texPosition"); 
    Window = glGetUniformLocation(program, "Window");
    Texture = glGetUniformLocation(program, "Texture");
    deltaTime = glGetUniformLocation(program, "deltaTime");
    velocityTexture = glGetUniformLocation(program, "velocityTexture");
    densityTexture = glGetUniformLocation(program, "densityTexture");

    
    for (int i = 0; i < DIMENSION * DIMENSION; i++)
    {
        velocityTexValues[i] = colour3(0.1, 0.1, 0.0);
    }

    /*int center = DIMENSION / 2;
    float t = 0.0;
    for (int i = 0; i < 10; i++)
    {
        float angle = t * 2 * 3.14;
        float vx = glm::cos(angle) * 1.0;
        float vy = glm::sin(angle) * 1.0;
        t += 0.01;
        velocityTexValues[IX(center, center)] = colour3(vx, vy, 0.0);
        
    }

    

    for (int i = -2; i < 2; i++)
    {
        for (int j = -2; j < 2; j++)
        {
            densityTexValues[IX(center + i, center + j)] = dis(gen);
        }
    }*/
    
    glUseProgram(program);
    glGenVertexArrays(1, vao);
    GLuint buffer;

    glBindVertexArray(vao[0]);
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertexPoints) + sizeof(cubeTexPoints), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cubeVertexPoints), cubeVertexPoints);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(cubeVertexPoints), sizeof(cubeTexPoints), cubeTexPoints);

    glGenBuffers(1, &indexBuff[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuff[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    glEnableVertexAttribArray(texPosition);
    glVertexAttribPointer(texPosition, 2, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(cubeVertexPoints)));

    
    
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.5, 0.3, 0.2, 1.0);
}


/*
    Function of dealing with situation with boundary cells.
    - b : int
    - x : float[]
*/
void set_bnd(int b, float* x) {
    for (int j = 1; j < DIMENSION - 1; j++) {
        x[IX(j, 0)] = b == 2 ? -x[IX(j, 1)] : x[IX(j, 1)];
        x[IX(j, DIMENSION - 1)] = b == 2 ? -x[IX(j, DIMENSION - 2)] : x[IX(j, DIMENSION - 2)];
    }
    for (int i = 1; i < DIMENSION - 1; i++) {
        x[IX(0, i)] = b == 1 ? -x[IX(1, i)] : x[IX(1, i)];
        x[IX(DIMENSION - 1, i)] = b == 1 ? -x[IX(DIMENSION - 2, i)] : x[IX(DIMENSION - 2, i)];
    }

    x[IX(0, 0)] = 0.5 * (x[IX(1, 0)] + x[IX(0, 1)]);
    x[IX(0, DIMENSION - 1)] = 0.5 * (x[IX(1, DIMENSION - 1)] + x[IX(0, DIMENSION - 2)]);
    x[IX(DIMENSION - 1, 0)] = 0.5 * (x[IX(DIMENSION - 2, 0)] + x[IX(DIMENSION - 1, 1)]);
    x[IX(DIMENSION - 1, DIMENSION - 1)] = 0.5 * (x[IX(DIMENSION - 2, DIMENSION - 1)] + x[IX(DIMENSION - 1, DIMENSION - 2)]);
}


/*
    Function of solving linear differential equation
    - b : int
    - x : float[]
    - x0 : float[]
    - a : float
    - c : float
*/
void lin_solve(int b, float* x, float* x0, float a, float c) {
    float  cRecip = 1.0 / c;
    for (int t = 0; t < 16; t++) {
        for (int i = 1; i < DIMENSION - 1; i++) {
            for (int j = 1; j < DIMENSION - 1; j++) {

                x[IX(i, j)] =
                    (x0[IX(i, j)] +
                        a *
                        (x[IX(i + 1, j)] +
                            x[IX(i - 1, j)] +
                            x[IX(i, j + 1)] +
                            x[IX(i, j - 1)])) *
                    cRecip;
            }
        }
        set_bnd(b, x);
    }
}

/*
    Function of diffuse
    - b : int
    - x : float[]
    - x0 : float[]
    - diff : float
    - dt : flaot
*/
void diffuse(int b, float* x, float* x0, float diff, float dt) {
    float a = dt * diff * (DIMENSION - 2) * (DIMENSION - 2);
    lin_solve(b, x, x0, a, 1 + 6 * a);
}


/*
    Function of project : This operation runs through all the cells and fixes them up so everything is in equilibrium.
    - velocX : float[]
    - velocY : float[]
    = p : float[]
    - div : float[]
*/
void project(float* velocX, float* velocY, float* p, float* div) {
    for (int i = 1; i < DIMENSION - 1; i++) {
        for (int j = 1; j < DIMENSION - 1; j++) {
            div[IX(i, j)] =
                (-0.5 *
                    (velocX[IX(i + 1, j)] -
                        velocX[IX(i - 1, j)] +
                        velocY[IX(i, j + 1)] -
                        velocY[IX(i, j - 1)])) /
                DIMENSION;
            p[IX(i, j)] = 0;
        }
    }

    set_bnd(0, div);
    set_bnd(0, p);
    lin_solve(0, p, div, 1, 6);

    for (int i = 1; i < DIMENSION - 1; i++) {
        for (int j = 1; j < DIMENSION - 1; j++) {
            velocX[IX(i, j)] -= 0.5 * (p[IX(i + 1, j)] - p[IX(i - 1, j)]) * DIMENSION;
            velocY[IX(i, j)] -= 0.5 * (p[IX(i, j + 1)] - p[IX(i, j - 1)]) * DIMENSION;
        }
    }

    set_bnd(1, velocX);
    set_bnd(2, velocY);
}

/*
    Function of advect: responsible for actually moving things around
    - b : int
    - d : float[]
    - d0 : float[]
    - velocX : float[]
    - velocY : float[]
    - velocZ : float[]
    - dt : float[]
*/
void advect(int b, float* d, float* d0, float* velocX, float* velocY, float dt) {
    float i0, i1, j0, j1;

    float dtx = dt * (DIMENSION - 2);
    float dty = dt * (DIMENSION - 2);

    float s0, s1, t0, t1;
    float tmp1, tmp2, tmp3, x, y;

    float Nfloat = DIMENSION - 2;
    float ifloat, jfloat;
    int i, j, k;

    for (i = 1, ifloat = 1; i < DIMENSION - 1; i++, ifloat++) {
        for (j = 1, jfloat = 1; j < DIMENSION - 1; j++, jfloat++) {
            tmp1 = dtx * velocX[IX(i, j)];
            tmp2 = dty * velocY[IX(i, j)];
            x = ifloat - tmp1;
            y = jfloat - tmp2;

            if (x < 0.5) x = 0.5;
            if (x > Nfloat + 0.5) x = Nfloat + 0.5;
            i0 = glm::floor(x);
            i1 = i0 + 1.0;
            if (y < 0.5) y = 0.5;
            if (y > Nfloat + 0.5) y = Nfloat + 0.5;
            j0 = glm::floor(y);
            j1 = j0 + 1.0;

            s1 = x - i0;
            s0 = 1.0 - s1;
            t1 = y - j0;
            t0 = 1.0 - t1;

            int i0i = i0;
            int i1i = i1;
            int j0i = j0;
            int j1i = j1;

            d[IX(i, j)] =
                s0 * (t0 * d0[IX(i0i, j0i)] + t1 * d0[IX(i0i, j1i)]) +
                s1 * (t0 * d0[IX(i1i, j0i)] + t1 * d0[IX(i1i, j1i)]);
        }
    }

    set_bnd(b, d);
}






//----------------------------------------------------------------------------

float t = 0.0;
void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1f(deltaTime, timeStamp);




    /*for (int i = 512; i < 512 + 20; i++)
    {
        densityTexValues[i] += 1.0f;
    }*/
   /* glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture[0]);
    glUniform1i(velocityTexture, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DIMENSION, DIMENSION, 0, GL_RGB, GL_FLOAT, velocityTexValues);*/


    
    int center = DIMENSION / 2;
    
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 1; j++)
        {
            float x = dis(gen);
            float temp = densityTexValues[IX(center + i, center + j)] + x;

            float finalValue = (float(int(temp) % 255) + 50.0f) / 255.0f;
            densityTexValues[IX(center + i, center + j)] = finalValue;
        }
    }

    for (int i = 0; i < 2; i++)
    {
        float angle = t * 4 * 3.14;
        float vx = glm::cos(angle) * 0.5;
        float vy = glm::sin(angle) * 0.5;
        t += 0.01;
        velocityX[IX(center, center)] += vx;
        velocityY[IX(center, center)] += vy;
    }

    diffuse(1, velocityX0, velocityX, viscocity, dt);
    diffuse(2, velocityY0, velocityY, viscocity, dt);

    project(velocityX0, velocityY0, velocityX, velocityY);

    advect(1, velocityX, velocityX0, velocityX0, velocityY0, dt);
    advect(2, velocityY, velocityY0, velocityX0, velocityY0, dt);

    project(velocityX, velocityY, velocityX0, velocityY0);
    diffuse(0, density0, densityTexValues, diffusion, dt);
    advect(0, densityTexValues, density0, velocityX, velocityY, dt);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glUniform1i(densityTexture, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, DIMENSION, DIMENSION, 0, GL_RED, GL_FLOAT, densityTexValues);
    
    

    glDrawElements(GL_TRIANGLES, NumQuadPoints + 2, GL_UNSIGNED_INT, 0);
    glutSwapBuffers();
}


//----------------------------------------------------------------------------

void
keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 033: // Escape Key
        exit(EXIT_SUCCESS);
        break;
    }
}


void
mouse(int button, int state, int x, int y)
{
    if (state == GLUT_DOWN) {

        switch (button) {
        case GLUT_LEFT_BUTTON:     break;
        case GLUT_MIDDLE_BUTTON:   break;
        case GLUT_RIGHT_BUTTON:    break;
        }
    }
}

//----------------------------------------------------------------------------


void
update(void)
{
    timeStamp += 0.01;
}

//----------------------------------------------------------------------------
int next_power_of_two(int v) {
    // to ensure a power-of-two texture, get the next highest power of two
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;

    return v;
}
int vp_width, vp_height, tex_width, tex_height;


void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    vp_width = width;
    vp_height = height;
    glUniform2f(Window, vp_width, vp_height);

    tex_width = next_power_of_two(vp_width);
    tex_height = next_power_of_two(vp_height);
    glUniform2f(Texture, tex_width, tex_height); 
}


