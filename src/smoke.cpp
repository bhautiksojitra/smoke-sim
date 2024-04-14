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

GLuint program, vPosition, texture, texPosition, vao[1], indexBuff[1];
GLuint velocityTexture, densityTexture;

std::random_device rd;
std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
std::uniform_real_distribution<> dis(150.0, 250.0); // Range [0.0, 1.0)

// Generate a random floating-point value
double randomValue = dis(gen);

float timeStamp = 0.0;

const int NumQuadPoints = 4;
const float QuadSize = 2.0;
const float EdgeCoord = QuadSize / 2.0;

const int DIMENSION = 100;
const int NUMVALUES = DIMENSION * DIMENSION;


float constant_random_color1 = float(rand() % 10) / 100.0;
float constant_random_color2 =  float(rand() % 10) / 100.0;
float constant_random_color3 = float(rand() % 10) / 100.0;

bool doSimulate = false;


float densityTexValues[NUMVALUES];
float density0[NUMVALUES];
glm::vec2 velocity0[NUMVALUES];
glm::vec2 velocity[NUMVALUES];
glm::vec3 color_arr[NUMVALUES];

float diffusion = 0.0f;
float viscocity = 0.0000001;
float dt = 0.2;

int object_center_x = 70;
int object_center_y = 70;
int object_size = 10;

int origin_x, origin_y;
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

void fill_object(int centerX, int centerY, int size)
{
    int half_cube_size =  size / 2;

    for (int i = centerX - half_cube_size; i < centerX + half_cube_size; i++)
    {
        for (int j = centerY - half_cube_size; j < centerY + half_cube_size; j++)
        {
            color_arr[IX(i, j)] = glm::vec3(1.0, 0.0, 0.0);
        }
    }
}

// OpenGL initialization
void
init()
{
    glLineWidth(1.0f);
    
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    program = InitShader("v.glsl", "f.glsl");
    vPosition = glGetAttribLocation(program, "vPosition");
    texPosition = glGetAttribLocation(program, "texPosition"); 
    velocityTexture = glGetUniformLocation(program, "velocityTexture");
    densityTexture = glGetUniformLocation(program, "densityTexture");

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

void set_boundaries_for_velocity(glm::vec2 *velocity)
{
    for (int i = 1; i < DIMENSION - 1; i++) {
        velocity[IX(i, 0)].y = -velocity[IX(i, 1)].y;
        velocity[IX(i, DIMENSION - 1)].y = -velocity[IX(i, DIMENSION - 2)].y;
        velocity[IX(i, 0)].x = velocity[IX(i, 1)].x;
        velocity[IX(i, DIMENSION - 1)].x = velocity[IX(i, DIMENSION - 2)].x;

        velocity[IX(0, i)].y = velocity[IX(1, i)].y;
        velocity[IX(DIMENSION - 1, i)].y = velocity[IX(DIMENSION -2, i)].y;
        velocity[IX(0, i)].x = -velocity[IX(1, i)].x;
        velocity[IX(DIMENSION - 1, i)].x = -velocity[IX(DIMENSION -2, i)].x;
    }

    velocity[IX(0, 0)] = 0.5f * (velocity[IX(1, 0)] + velocity[IX(0, 1)]);
    velocity[IX(0, DIMENSION - 1)] = 0.5f * (velocity[IX(1, DIMENSION - 1)] + velocity[IX(0, DIMENSION - 2)]);
    velocity[IX(DIMENSION - 1, 0)] = 0.5f * (velocity[IX(DIMENSION - 2, 0)] + velocity[IX(DIMENSION - 1, 1)]);
    velocity[IX(DIMENSION - 1, DIMENSION - 1)] = 0.5f * (velocity[IX(DIMENSION - 2, DIMENSION - 1)] + velocity[IX(DIMENSION - 1, DIMENSION - 2)]);

    

}

void set_boundaries_for_object(glm::vec2* velocity)
{
    int leftX = object_center_x - (object_size / 2);
    int rightX = object_center_x + (object_size / 2);

    int downY = object_center_y - (object_size / 2);
    int upY = object_center_y + (object_size / 2);

    for (int i = 0; i < DIMENSION - 1; i++)
    {
        for (int j = 0; j < DIMENSION -1; j++)
        {
            if (i >= leftX && j < upY && j > downY)
            {
                velocity[IX(leftX, j)] = -velocity[IX(leftX - 1, j)];
            }
            if (i <= rightX && j < upY && j > downY)
            {
                velocity[IX(rightX, j)] = -velocity[IX(rightX + 1, j)];
            }
            if (j <= upY && i > leftX && i < rightX)
            {
                velocity[IX(i, upY)] = -velocity[IX(i, upY + 1)];
            }
            if (j >= downY && i > leftX && i < rightX)
            {
                velocity[IX(i, downY)] = -velocity[IX(i, downY - 1)];
            }

        }
    }

    velocity[IX(leftX, downY)] = (velocity[IX(leftX + 1, downY)] + velocity[IX(leftX, downY + 1)]) * 0.5f;
    velocity[IX(leftX, upY)] = 0.5f * (velocity[IX(leftX + 1, upY)] + velocity[IX(leftX, upY - 1)]);
    velocity[IX(rightX, downY)] = 0.5f * (velocity[IX(rightX - 1, downY)] + velocity[IX(rightX, downY + 1)]);
    velocity[IX(rightX, upY)] = 0.5f * (velocity[IX(rightX - 1, upY)] + velocity[IX(rightX, upY - 1)]);
}

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
    }
}


void lin_solve_for_velocity(float a, float c) {
    float  cRecip = 1.0 / c;
    for (int t = 0; t < 16; t++) {
        for (int i = 1; i < DIMENSION - 1; i++) {
            for (int j = 1; j < DIMENSION - 1; j++) {

                float xValue =
                    (velocity[IX(i, j)].x +
                        a *
                        (velocity0[IX(i + 1, j)].x +
                            velocity0[IX(i - 1, j)].x +
                            velocity0[IX(i, j + 1)].x +
                            velocity0[IX(i, j - 1)])).x *
                    cRecip;

                float yValue =
                    (velocity[IX(i, j)].y +
                        a *
                        (velocity0[IX(i + 1, j)].y +
                            velocity0[IX(i - 1, j)].y +
                            velocity0[IX(i, j + 1)].y +
                            velocity0[IX(i, j - 1)])).y *
                    cRecip;

                velocity0[IX(i, j)] = glm::vec2(xValue, yValue);                
            }
        }
        set_boundaries_for_velocity(velocity);
        set_boundaries_for_object(velocity);
    }
}

void diffuse_velocity(float diff, float dt) {
    float a = dt * diff * 100;
    lin_solve_for_velocity(a, 1 + 6 * a);
}

void diffuse(int b, float* x, float* x0, float diff, float dt) {
    float a = dt * diff * 100;
    lin_solve(b, x, x0, a, 1 + 6 * a);
}

void project(glm::vec2 * velocity_arr1, glm::vec2* velocity_arr2) {
    for (int i = 1; i < DIMENSION - 1; i++) {
        for (int j = 1; j < DIMENSION - 1; j++) {
            velocity_arr2[IX(i, j)].x =
                (-0.5 *
                    (velocity_arr1[IX(i + 1, j)].x -
                        velocity_arr1[IX(i - 1, j)].x +
                        velocity_arr1[IX(i, j + 1)].y -
                        velocity_arr1[IX(i, j - 1)]).y) /
                DIMENSION;
           velocity_arr2[IX(i, j)].y =
                (-0.5 *
                    (velocity_arr1[IX(i + 1, j)].x -
                        velocity_arr1[IX(i - 1, j)].x +
                        velocity_arr1[IX(i, j + 1)].y -
                        velocity_arr1[IX(i, j - 1)]).y) /
                DIMENSION;
        }
    }

    set_boundaries_for_velocity(velocity_arr2);
    set_boundaries_for_object(velocity_arr2);
    

    for (int i = 1; i < DIMENSION - 1; i++) {
        for (int j = 1; j < DIMENSION - 1; j++) {
            velocity_arr1[IX(i, j)].x -= 0.5 * (velocity_arr2[IX(i + 1, j)].x - velocity_arr2[IX(i - 1, j)].x) * DIMENSION;
            velocity_arr1[IX(i, j)].y -= 0.5 * (velocity_arr2[IX(i, j + 1)].x - velocity_arr2[IX(i, j - 1)].x) * DIMENSION;
        }
    }

    set_boundaries_for_velocity(velocity_arr1);
    set_boundaries_for_object(velocity_arr1);
}


void advect_velocity(bool isY, glm::vec2 *vel, glm::vec2 * velocity_arr, float dt) {
    float i0, i1, j0, j1;

    float dtx = dt * (FRAME_RATE_MS);
    float dty = dt * (FRAME_RATE_MS);

    float s0, s1, t0, t1;
    float x, y;

    int i, j, k;

    for (i = 1; i < DIMENSION - 1; i++) {
        for (j = 1; j < DIMENSION - 1; j++) {

            x = i - (dtx * velocity_arr[IX(i, j)].x);
            y = j - (dtx * velocity_arr[IX(i, j)].y);

            if (x < 0.5) x = 0.5;
            if (x > (DIMENSION - 2) + 0.5) x = (DIMENSION - 2) + 0.5;
            i0 = glm::floor(x);
            i1 = i0 + 1.0;
            if (y < 0.5) y = 0.5;
            if (y > (DIMENSION - 2) + 0.5) y = (DIMENSION - 2) +0.5;
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

            if (isY == false)
            {
                vel[IX(i, j)].x =
                    s0 * (t0 * velocity_arr[IX(i0i, j0i)].x + t1 * velocity_arr[IX(i0i, j1i)].x) +
                    s1 * (t0 * velocity_arr[IX(i1i, j0i)].x + t1 * velocity_arr[IX(i1i, j1i)].x);
            }
            else
            {
                vel[IX(i, j)].y =
                    s0 * (t0 * velocity_arr[IX(i0i, j0i)].y + t1 * velocity_arr[IX(i0i, j1i)].y) +
                    s1 * (t0 * velocity_arr[IX(i1i, j0i)].y + t1 * velocity_arr[IX(i1i, j1i)].y);
            }
            
        }
    }
    set_boundaries_for_velocity(vel);
    set_boundaries_for_object(vel);
}

void advect(float *d, float *d0, glm::vec2* velocity_arr, float dt) {
    float i0, i1, j0, j1;

    float dtx = dt * (FRAME_RATE_MS);
    float dty = dt * (FRAME_RATE_MS);

    float s0, s1, t0, t1;
    float x, y;

    int i, j, k;

    for (i = 1; i < DIMENSION - 1; i++) {
        for (j = 1; j < DIMENSION - 1; j++) {

            x = i - (dtx * velocity_arr[IX(i, j)].x);
            y = j - (dtx * velocity_arr[IX(i, j)].y);

            if (x < 0.5) x = 0.5;
            if (x > (DIMENSION - 2) + 0.5) x = (DIMENSION - 2) + 0.5;
            i0 = glm::floor(x);
            i1 = i0 + 1.0;
            if (y < 0.5) y = 0.5;
            if (y > (DIMENSION - 2) + 0.5) y = (DIMENSION - 2) + 0.5;
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
    set_bnd(0, d);
}


//----------------------------------------------------------------------------

void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    int center = DIMENSION / 2;

    if (doSimulate)
    {
        for (int i = 0; i < 1; i++)
        {
            for (int j = 0; j < 1; j++)
            {
                float x = dis(gen);
                float temp = densityTexValues[IX(origin_x + i, origin_y + j)] + x;

                float real_value = glm::clamp(temp, 0.0f, 1.7f);
                densityTexValues[IX(origin_x + i, origin_y + j)] += real_value;
            }
        }
        for (int i = 0; i < 1; i++)
        {
            velocity[IX(origin_x, origin_y)] += glm::vec2(rand() % 4 + 2, rand() % 3 + 1);
        }


       
    }
    
    diffuse_velocity(viscocity, dt);



    project(velocity0, velocity);


    advect_velocity(true, velocity, velocity0, dt);
    advect_velocity(false, velocity, velocity0, dt);

    project(velocity, velocity0);



    diffuse(0, density0, densityTexValues, diffusion, dt);

    advect(densityTexValues, density0, velocity, dt);


    for (int i = 0; i < NUMVALUES; i++)
    {
        float color_val = densityTexValues[i];
        color_arr[i] = glm::vec3(color_val - constant_random_color1, color_val - constant_random_color2, color_val - constant_random_color3);
    }

    fill_object(object_center_x, object_center_y, object_size);
    
    
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DIMENSION, DIMENSION, 0, GL_RGB, GL_FLOAT, color_arr);
    
    

    glDrawElements(GL_TRIANGLES, NumQuadPoints + 2, GL_UNSIGNED_INT, 0);
    glutSwapBuffers();
}


//----------------------------------------------------------------------------

void update_obstacle()
{
    object_size = (rand() % 20) + 8;
    object_center_x = (rand() % (DIMENSION - (object_size / 2))) + ((object_size / 2));
    object_center_y = (rand() % (DIMENSION - (object_size / 2))) + ((object_size / 2));
}

void reset_sim()
{
    for (int i = 0; i < DIMENSION - 1; i++)
    {
        for (int j = 0; j < DIMENSION - 1; j++)
        {
            velocity[IX(i, j)] = glm::vec2(0, 0);
            velocity0[IX(i, j)] = glm::vec2(0, 0);
            density0[IX(i, j)] = 0.0f;
            densityTexValues[IX(i, j)] = 0.0f;

        }
    }
    doSimulate = false;
}

void
keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 033: // Escape Key
        exit(EXIT_SUCCESS);
        break;
    case 'n': case 'N':
        update_obstacle();
        break;
    case 'r': case 'R':
        reset_sim();
    }
}


void
mouse(int button, int state, int x, int y)
{
    origin_y = (DIMENSION * x) / 512;
    origin_x = (DIMENSION * (512 - y)) / 512;
    if (state == GLUT_DOWN) {

        switch (button) {
        case GLUT_LEFT_BUTTON: 
            doSimulate = true;
            std::cout << origin_x << " " << origin_y << std::endl;
            break;
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


void
reshape(int width, int height)
{
    glViewport(0, 0, width, height);
}


