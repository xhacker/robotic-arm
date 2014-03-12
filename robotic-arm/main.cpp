//
//  main.cpp
//  robotic-arm
//
//  Created by Xhacker Liu on 3/8/14.
//  Copyright (c) 2014 Xhacker. All rights reserved.
//

#include "Angel.h"
#include <iostream>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

const int NumVertices = 36; //(6 faces)(2 triangles/face)(3 vertices/triangle)

point4 points[NumVertices];
color4 colors[NumVertices];

point4 vertices[8] = {
    point4(-0.5, -0.5,  0.5, 1.0), point4(-0.5,  0.5,  0.5, 1.0),
    point4( 0.5,  0.5,  0.5, 1.0), point4( 0.5, -0.5,  0.5, 1.0),
    point4(-0.5, -0.5, -0.5, 1.0), point4(-0.5,  0.5, -0.5, 1.0),
    point4( 0.5,  0.5, -0.5, 1.0), point4( 0.5, -0.5, -0.5, 1.0)
};

// RGBA olors
color4 vertex_colors[8] = { color4(0.0, 0.0, 0.0, 1.0), // black
                            color4(1.0, 0.0, 0.0, 1.0), // red
                            color4(1.0, 1.0, 0.0, 1.0), // yellow
                            color4(0.0, 1.0, 0.0, 1.0), // green
                            color4(0.0, 0.0, 1.0, 1.0), // blue
                            color4(1.0, 0.0, 1.0, 1.0), // magenta
                            color4(1.0, 1.0, 1.0, 1.0), // white
                            color4(0.0, 1.0, 1.0, 1.0) // cyan
};

// Parameters controlling the size of the Robot's arm
const GLfloat SPHERE_R = 0.5;
const GLfloat BASE_HEIGHT = 2.0;
const GLfloat BASE_WIDTH = 5.0;
const GLfloat LOWER_ARM_HEIGHT = 5.0;
const GLfloat LOWER_ARM_WIDTH = 0.5;
const GLfloat UPPER_ARM_HEIGHT = 5.0;
const GLfloat UPPER_ARM_WIDTH = 0.5;

// Shader transformation matrices
mat4 model_view;
GLuint ModelView, Projection;

// Array of rotation angles (in degrees) for each rotation axis
enum {
    Base = 0,
    LowerArm = 1,
    UpperArm = 2,
    NumAngles = 3
};
int Axis = Base;
GLfloat theta[NumAngles] = {0.0};
GLfloat old_theta[NumAngles] = {0.0};
GLfloat new_theta[NumAngles] = {0.0};

// Menu option values
const int Quit = 4;
const int SwitchView = 5;

bool is_side_view = true;
bool has_sphere = false;
double old_x, old_y, old_z, new_x, new_y, new_z;
timeval start_time;
int step = 0;
double progress = 0.0;

const int kStepTime = 1000;

enum {
    GoToOldBase = 0,
    GoToOldLower,
    GoToOldUpper,
    GoToNewBase,
    GoToNewLower,
    GoToNewUpper,
    GoToHomeBase,
    GoToHomeLower,
    GoToHomeUpper,
    AnimationFinished
};

int Index = 0;

void quad(int a, int b, int c, int d)
{
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[a];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[b];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[c];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[a];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[c];
    Index++;
    colors[Index] = vertex_colors[a];
    points[Index] = vertices[d];
    Index++;
}

void colorcube()
{
    quad(1, 0, 3, 2);
    quad(2, 3, 7, 6);
    quad(3, 0, 4, 7);
    quad(6, 5, 1, 2);
    quad(4, 5, 6, 7);
    quad(5, 4, 0, 1);
}

/* Define the four parts */
/* Note use of push/pop to return modelview matrix
 to its state before functions were entered and use
 rotation, translation, and scaling to create instances
 of symbols (cube and cylinder */

void sphere()
{
    mat4 instance = (Translate(old_x, old_y, old_z) * Scale(SPHERE_R, SPHERE_R, SPHERE_R));

    if (step >= GoToNewBase && step < GoToHomeBase) {
        instance = (Translate(0, LOWER_ARM_HEIGHT, 0) * Scale(SPHERE_R, SPHERE_R, SPHERE_R));
    }
    else if (step >= GoToHomeBase) {
        instance = (Translate(new_x, new_y, new_z) * Scale(SPHERE_R, SPHERE_R, SPHERE_R));
    }

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);

#ifdef __APPLE__
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
#endif

    glutSolidSphere(0.75, 20, 20);
}

void base()
{
    mat4 instance = (Translate(0.0, 0.5 * BASE_HEIGHT, 0.0) * Scale(BASE_WIDTH, BASE_HEIGHT, BASE_WIDTH));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);

    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
}

void upper_arm()
{
    mat4 instance = (Translate(0.0, 0.5 * UPPER_ARM_HEIGHT, 0.0) * Scale(UPPER_ARM_WIDTH, UPPER_ARM_HEIGHT, UPPER_ARM_WIDTH));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
}

void lower_arm()
{
    mat4 instance = (Translate(0.0, 0.5 * LOWER_ARM_HEIGHT, 0.0) * Scale(LOWER_ARM_WIDTH, LOWER_ARM_HEIGHT, LOWER_ARM_WIDTH));

    glUniformMatrix4fv(ModelView, 1, GL_TRUE, model_view * instance);
    glDrawArrays(GL_TRIANGLES, 0, NumVertices);
}

int elapsed()
{
    timeval t;
    gettimeofday(&t, NULL);
    double elapsedTime;
    elapsedTime = (t.tv_sec - start_time.tv_sec) * 1000.0;      // sec to ms
    elapsedTime += (t.tv_usec - start_time.tv_usec) / 1000.0;   // us to ms
    return elapsedTime;
}

void update_theta()
{
    step = elapsed() / kStepTime;
    progress = ((double)(elapsed() % kStepTime + 1)) / kStepTime;

    switch (step) {
        case GoToOldBase:
            theta[Base] = progress * old_theta[Base];
            break;
        case GoToOldLower:
            theta[LowerArm] = progress * old_theta[LowerArm];
            break;
        case GoToOldUpper:
            theta[UpperArm] = progress * old_theta[UpperArm];
            break;
        case GoToNewBase:
            theta[Base] = old_theta[Base] + progress * (new_theta[Base] - old_theta[Base]);
            break;
        case GoToNewLower:
            theta[LowerArm] = old_theta[LowerArm] + progress * (new_theta[LowerArm] - old_theta[LowerArm]);
            break;
        case GoToNewUpper:
            theta[UpperArm] = old_theta[UpperArm] + progress * (new_theta[UpperArm] - old_theta[UpperArm]);
            break;
        case GoToHomeBase:
            theta[Base] = (1 - progress) * new_theta[Base];
            break;
        case GoToHomeLower:
            theta[Base] = 0;
            theta[LowerArm] = (1 - progress) * new_theta[LowerArm];
            break;
        case GoToHomeUpper:
            theta[LowerArm] = 0;
            theta[UpperArm] = (1 - progress) * new_theta[UpperArm];
            break;
        case AnimationFinished:
            theta[UpperArm] = 0;
            break;
        default:
            break;
    }
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    update_theta();

    if (is_side_view) {
        model_view = RotateX(0);
    }
    else {
        model_view = (Translate(0, BASE_WIDTH, 0) * RotateX(90.0));
    }

    if (has_sphere && (step < GoToNewBase || step >= GoToHomeBase)) {
        sphere();
    }

    model_view *= RotateY(theta[Base]);
    base();

    model_view *= (Translate(0.0, BASE_HEIGHT, 0.0) * RotateZ(theta[LowerArm]));
    lower_arm();

    model_view *= (Translate(0.0, LOWER_ARM_HEIGHT, 0.0) * RotateZ(theta[UpperArm]));
    upper_arm();

    if (has_sphere && step >= GoToNewBase && step < GoToHomeBase) {
        sphere();
    }

    glutSwapBuffers();
}

void init(void)
{
    colorcube();

    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points) + sizeof(colors), NULL,
                 GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points), points);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points), sizeof(colors), colors);

    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    GLuint vPosition = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

    GLuint vColor = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0,
                          BUFFER_OFFSET(sizeof(points)));

    ModelView = glGetUniformLocation(program, "ModelView");
    Projection = glGetUniformLocation(program, "Projection");

    glEnable(GL_DEPTH);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    glClearColor(1.0, 1.0, 1.0, 1.0);
}

void mouse(int button, int state, int x, int y)
{
    if (has_sphere && step < AnimationFinished) {
        return;
    }

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Incrase the joint angle
        theta[Axis] += 5.0;
        if (theta[Axis] > 360.0) {
            theta[Axis] -= 360.0;
        }
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        // Decrase the joint angle
        theta[Axis] -= 5.0;
        if (theta[Axis] < 0.0) {
            theta[Axis] += 360.0;
        }
    }

    glutPostRedisplay();
}

void menu(int option)
{
    if (option == Quit) {
        exit(EXIT_SUCCESS);
    }
    else if (option == SwitchView) {
        if (is_side_view) {
            glutChangeToMenuEntry(4, "top view", SwitchView);
        }
        else {
            glutChangeToMenuEntry(4, "side view", SwitchView);
        }
        is_side_view = !is_side_view;

        glutPostRedisplay();
    }
    else {
        Axis = option;
    }
}

void reshape(int width, int height)
{
    glViewport(0, 0, width, height);

    GLfloat left = -10.0, right = 10.0;
    GLfloat bottom = -5.0, top = 15.0;
    GLfloat zNear = -10.0, zFar = 10.0;

    GLfloat aspect = GLfloat(width) / height;

    if (aspect > 1.0) {
        left *= aspect;
        right *= aspect;
    }
    else {
        bottom /= aspect;
        top /= aspect;
    }

    mat4 projection = Ortho(left, right, bottom, top, zNear, zFar);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    model_view = mat4(1.0); // An Identity matrix
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key) {
    case 033: // Escape Key
    case 'q':
    case 'Q':
        exit(EXIT_SUCCESS);
        break;
    }
}

void idle()
{
    usleep(200);
    glutPostRedisplay();
}

double arctan(double value)
{
    return atan(value) / (2 * M_PI) * 360;
}

double arccos(double value)
{
    return acos(value) / (2 * M_PI) * 360;
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        if (argc != 7 && argc != 8) {
            cout << "Invalid arguments." << endl;
            return 0;
        }

        has_sphere = true;

        old_x = atof(argv[1]);
        old_y = atof(argv[2]);
        old_z = atof(argv[3]);
        new_x = atof(argv[4]);
        new_y = atof(argv[5]);
        new_z = atof(argv[6]);

        if (old_x == 0) {
            old_x = 0.00001;
        }
        if (new_x == 0) {
            new_x = 0.00001;
        }

        if (argc == 8) {
            if (strncmp(argv[7], "-tv", 4) == 0) {
                is_side_view = false;
            }
            else if (strncmp(argv[7], "-sv", 4) == 0) {
                is_side_view = true;
            }
        }
    }

    gettimeofday(&start_time, NULL);

    if (has_sphere) {
        {
            old_theta[Base] = -arctan(old_z / old_x);

            double x = sqrt(pow(old_x, 2) + pow(old_z, 2));
            double y = old_y - BASE_HEIGHT;

            double distance = sqrt(pow(x, 2) + pow(y, 2));
            if (distance < 0 || distance > 10) {
                cout << "Not reachable." << endl;
                return 0;
            }

            // law of cosines, calculate the angle between two arms
            double angle = arccos((50 - (pow(x, 2) + pow(y, 2))) / 50);
            double beta = 180 - angle;
            double alpha = 90 - beta / 2 - arctan(y / x);

            if (old_x < 0) {
                beta = -beta;
                alpha = -alpha;
            }

            old_theta[UpperArm] = -beta;
            old_theta[LowerArm] = -alpha;
        }
        {
            new_theta[Base] = -arctan(new_z / new_x);

            double x = sqrt(pow(new_x, 2) + pow(new_z, 2));
            double y = new_y - BASE_HEIGHT;

            double distance = sqrt(pow(x, 2) + pow(y, 2));
            if (distance < 0 || distance > 10) {
                cout << "Not reachable." << endl;
                return 0;
            }

            // law of cosines, calculate the angle between two arms
            double angle = arccos((50 - (pow(x, 2) + pow(y, 2))) / 50);
            double beta = 180 - angle;
            double alpha = 90 - beta / 2 - arctan(y / x);

            if (new_x < 0) {
                beta = -beta;
                alpha = -alpha;
            }

            new_theta[UpperArm] = -beta;
            new_theta[LowerArm] = -alpha;
        }
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(512, 512);
    glutCreateWindow("robot");

#ifndef __APPLE__
    glewInit();
#endif

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutIdleFunc(idle);

    glutCreateMenu(menu);
    // Set the menu values to the relevant rotation axis values (or Quit)
    glutAddMenuEntry("base", Base);
    glutAddMenuEntry("lower arm", LowerArm);
    glutAddMenuEntry("upper arm", UpperArm);
    glutAddMenuEntry(is_side_view ? "side view" : "top view", SwitchView);
    glutAddMenuEntry("quit", Quit);
    glutAttachMenu(GLUT_MIDDLE_BUTTON);

    glutMainLoop();
    return 0;
}
