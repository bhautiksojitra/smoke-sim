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
std::uniform_real_distribution<> dis(150.0, 250.0); // Range [0.0, 1.0)

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

glm::vec2 velocity0[NUMVALUES];
glm::vec2 velocity[NUMVALUES];

glm::vec3 color_arr[NUMVALUES];

float diffusion = 0.0f;
float viscocity = 0.0000001;
float dt = 0.2;

int centerX = 70;
int centerY = 70;
int size_object = 10;


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

void define_rect(int centerX, int centerY, int size)
{
    int half_cube_size =  size / 2;
    assert(centerX > 4);
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


void advect_v2(float dt, float * prev_grid, float * curr_grid)
{
    for (int i = 0; i < DIMENSION - 1; i++)
    {
        for (int j = 0; j < DIMENSION - 1; j++)
        {
            float tracedBackX = i - (dt * 100 * velocityX[IX(i, j)]);
            float tracedBackY = j - (dt * 100 * velocityY[IX(i, j)]);

            tracedBackX = std::max(0.5f, tracedBackX);
            tracedBackX = std::min(DIMENSION - 0.5f, tracedBackX);

            tracedBackY = std::max(0.5f, tracedBackY);
            tracedBackY = std::min(DIMENSION - 0.5f, tracedBackY);
            int x0 = (int)tracedBackX;
            int x1 = x0 + 1;

            int y0 = (int)tracedBackY;
            int y1 = y0 + 1;

            float dx = tracedBackX - x0;
            float dy = tracedBackY - y0;

            double value00 = prev_grid[IX(x0, y0)];
            double value01 = prev_grid[IX(x0, y1)];
            double value10 = prev_grid[IX(x1, y0)];
            double value11 = prev_grid[IX(x1, y1)];

            curr_grid[IX(i, j)] = (1.0f - dx) * ((1.0f - dy) * value00 + dy * value01) +
                dx * ((1 - dy) * value10 + dy * value11);

        }
    }
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

   /* for (int j = 1; j < DIMENSION - 1; j++) {
        x[IX(j, 0)] = -x[IX(j, 1)];
        x[IX(j, DIMENSION - 1)] = -x[IX(j, DIMENSION - 2)];
    }
    for (int i = 1; i < DIMENSION - 1; i++) {
        x[IX(0, i)] = -x[IX(1, i)];
        x[IX(DIMENSION - 1, i)] = -x[IX(DIMENSION - 2, i)];
    }*/

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

void set_boundaries_for_object(glm::vec2* velocity, int centerX, int centerY, int size)
{
    int leftX = centerX - (size / 2);
    int rightX = leftX + size;

    int downY = centerY - (size / 2);
    int upY = downY + size;

    for (int i = 1; i < DIMENSION - 1; i++) {
       /* velocity[IX(i, downY)].y = -velocity[IX(i, downY + 1)].y;
        velocity[IX(i, upY)].y = -velocity[IX(i, upY - 1)].y;
        velocity[IX(i, downY)].x = velocity[IX(i, downY + 1)].x;
        velocity[IX(i, upY)].x = velocity[IX(i, upY - 1)].x;

        velocity[IX(leftX, i)].y = velocity[IX(leftX + 1, i)].y;
        velocity[IX(rightX, i)].y = velocity[IX(rightX - 1, i)].y;
        velocity[IX(leftX, i)].x = -velocity[IX(leftX + 1, i)].x;
        velocity[IX(rightX, i)].x = -velocity[IX(rightX - 1, i)].x;*/
    }

    for (int i = 0; i < DIMENSION - 1; i++)
    {
        for (int j = 0; j < DIMENSION -1; j++)
        {
            if (i == leftX && j < upY && j > downY)
            {
                velocity[IX(leftX, j)] = -velocity[IX(leftX - 1, j)];
            }
            if (i == rightX && j < upY && j > downY)
            {
                velocity[IX(rightX, j)] = -velocity[IX(rightX + 1, j)];
            }
            if (j == upY && i > leftX && i < rightX)
            {
                velocity[IX(i, upY)] = -velocity[IX(i, upY + 1)];
            }
            if (j == downY && i > leftX && i < rightX)
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
       // set_bnd(b, x);
    }
}


/*
    Function of solving linear differential equation
    - b : int
    - x : float[]
    - x0 : float[]
    - a : float
    - c : float
*/
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
        set_boundaries_for_object(velocity, centerX, centerY, size_object);
       // set_bnd(b, x);
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
void diffuse_velocity(float diff, float dt) {
    float a = dt * diff * 100;
    lin_solve_for_velocity(a, 1 + 6 * a);
}

void diffuse(int b, float* x, float* x0, float diff, float dt) {
    float a = dt * diff * 100;
    lin_solve(b, x, x0, a, 1 + 6 * a);
}


/*
    Function of project : This operation runs through all the cells and fixes them up so everything is in equilibrium.
    - velocX : float[]
    - velocY : float[]
    = p : float[]
    - div : float[]
*/
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
            /*p[IX(i, j)] =
                (-0.5 *
                    (velocX[IX(i + 1, j)] -
                        velocX[IX(i - 1, j)] +
                        velocY[IX(i, j + 1)] -
                        velocY[IX(i, j - 1)])) /
                DIMENSION;*/
            //velocity_arr2[IX(i, j)].y = 0;
        }
    }

    //for (int t = 0; t < 16; t++) {
    //    for (int i = 1; i < DIMENSION - 1; i++) {
    //        for (int j = 1; j < DIMENSION - 1; j++) {

    //            velocity_arr2[IX(i, j)].y =
    //                (velocity_arr2[IX(i, j)].y +
    //                    
    //                    (velocity_arr2[IX(i, j)].x +
    //                        velocity_arr2[IX(i - 1, j)].x +
    //                        velocity_arr2[IX(i, j + 1)].x +
    //                        velocity_arr2[IX(i, j - 1)])).x *
    //                (1/6);
    //        }
    //    }


    set_boundaries_for_velocity(velocity_arr2);
   set_boundaries_for_object(velocity_arr2, centerX, centerY, size_object);
    //    // set_bnd(b, x);
    //}
    //set_bnd(0, div);
    //set_bnd(0, p);
    //lin_solve(0, p, div, 1, 6);

  for (int i = 1; i < DIMENSION - 1; i++) {
        for (int j = 1; j < DIMENSION - 1; j++) {
            velocity_arr1[IX(i, j)].x -= 0.5 * (velocity_arr2[IX(i + 1, j)].x - velocity_arr2[IX(i - 1, j)].x) * DIMENSION;
            velocity_arr1[IX(i, j)].y -= 0.5 * (velocity_arr2[IX(i, j + 1)].x - velocity_arr2[IX(i, j - 1)].x) * DIMENSION;
        }
  }

  set_boundaries_for_velocity(velocity_arr1);
  set_boundaries_for_object(velocity_arr1, centerX, centerY, size_object);
   //set_bnd(1, velocX);
   //set_bnd(2, velocY);
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
   set_boundaries_for_object(vel, centerX, centerY, size_object);
   //set_bnd(b, d);
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
    //set_bnd(b, d);
}






//----------------------------------------------------------------------------

float t = 0.0;
void
display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUniform1f(deltaTime, timeStamp);


    
    int center = DIMENSION / 2;
    
    

    if (t < 5)
    {

        for (int i = -2; i < 2; i++)
        {
            for (int j = -2; j < 2; j++)
            {
                float x = dis(gen);
                float temp = densityTexValues[IX(center + i, center + j)] + x;

                float finalValue = glm::clamp(temp, 0.0f, 1.0f);
                densityTexValues[IX(center + i, center + j)] = finalValue;
            }
        }
        for (int i = 0; i < 1; i++)
        {
            float angle = t * 4 * 3.14;
            float vx = glm::cos(angle) * 1.6;
            float vy = glm::sin(angle) * 1.6;
            t += 0.01;
            velocity[IX(center, center)] += glm::vec2(rand() % 2, -rand() % 3);
        }
    }
    else
    {
        for (int i = 0; i < DIMENSION - 1; i++)
        {
            for (int j = 0; j < DIMENSION - 1; j++)
            {
                densityTexValues[IX(i, j)] -= 0.001;
            }
        }

    }
   
        
    

    

    

    diffuse_velocity(viscocity, dt);

    //diffuse(1, velocityX0, velocityX, viscocity, dt);
    //diffuse(2, velocityY0, velocityY, viscocity, dt);

    project(velocity0, velocity);

    //advect_v2(dt, velocityX, velocityX);
    //advect_v2(dt, velocityY0, velocityY);
    advect_velocity(true, velocity, velocity0,  dt);
    advect_velocity(false, velocity, velocity0, dt);

   project(velocity, velocity0);

    /*for (int i = 1; i < DIMENSION - 1; i++) {
        velocityY[IX(i, 0)] = -velocityY[IX(i, 1)];
        velocityY[IX(i, DIMENSION - 1)] = -velocityY[IX(i, DIMENSION - 2)];
        velocityX[IX(i, 0)] = velocityX[IX(i, 1)];
        velocityX[IX(i, DIMENSION - 1)] = velocityX[IX(i, DIMENSION - 2)];

        velocityY[IX(0, i)] = velocityY[IX(1, i)];
        velocityY[IX(DIMENSION - 1, i)] = velocityY[IX(DIMENSION - 2, i)];
        velocityX[IX(0, i)] = -velocityX[IX(1, i)];
        velocityX[IX(DIMENSION - 1, i)] = -velocityX[IX(DIMENSION - 2, i)];
    }

    velocityX[IX(0, 0)] = 0.5 * (velocityX[IX(1, 0)] + velocityX[IX(0, 1)]);
    velocityX[IX(0, DIMENSION - 1)] = 0.5 * (velocityX[IX(1, DIMENSION - 1)] + velocityX[IX(0, DIMENSION - 2)]);
    velocityX[IX(DIMENSION - 1, 0)] = 0.5 * (velocityX[IX(DIMENSION - 2, 0)] + velocityX[IX(DIMENSION - 1, 1)]);
    velocityX[IX(DIMENSION - 1, DIMENSION - 1)] = 0.5 * (velocityX[IX(DIMENSION - 2, DIMENSION - 1)] + velocityX[IX(DIMENSION - 1, DIMENSION - 2)]);

    velocityY[IX(0, 0)] = 0.5 * (velocityY[IX(1, 0)] + velocityY[IX(0, 1)]);
    velocityY[IX(0, DIMENSION - 1)] = 0.5 * (velocityY[IX(1, DIMENSION - 1)] + velocityY[IX(0, DIMENSION - 2)]);
    velocityY[IX(DIMENSION - 1, 0)] = 0.5 * (velocityY[IX(DIMENSION - 2, 0)] + velocityY[IX(DIMENSION - 1, 1)]);
    velocityY[IX(DIMENSION - 1, DIMENSION - 1)] = 0.5 * (velocityY[IX(DIMENSION - 2, DIMENSION - 1)] + velocityY[IX(DIMENSION - 1, DIMENSION - 2)]);*/
    
    diffuse(0, density0, densityTexValues, diffusion, dt);
    //advect_v2(dt, densityTexValues, density0);    
    advect(densityTexValues, density0, velocity, dt);

    

    densityTexValues[IX(0, 0)] = 0.5 * (densityTexValues[IX(1, 0)] + densityTexValues[IX(0, 1)]);
    densityTexValues[IX(0, DIMENSION - 1)] = 0.5 * (densityTexValues[IX(1, DIMENSION - 1)] + densityTexValues[IX(0, DIMENSION - 2)]);
    densityTexValues[IX(DIMENSION - 1, 0)] = 0.5 * (densityTexValues[IX(DIMENSION - 2, 0)] + densityTexValues[IX(DIMENSION - 1, 1)]);
    densityTexValues[IX(DIMENSION - 1, DIMENSION - 1)] = 0.5 * (densityTexValues[IX(DIMENSION - 2, DIMENSION - 1)] + densityTexValues[IX(DIMENSION - 1, DIMENSION - 2)]);


    for (int i = 0; i < NUMVALUES; i++)
    {
        float color_val = densityTexValues[i];
        color_arr[i] = glm::vec3(color_val, color_val, color_val);
    }

    define_rect(centerX, centerY, size_object);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture[1]);
    glUniform1i(densityTexture, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, DIMENSION, DIMENSION, 0, GL_RGB, GL_FLOAT, color_arr);
    
    

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


