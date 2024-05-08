#include "Angel.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>

// ui library: dearimgui

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

typedef vec4  color4;
typedef vec4  point4;

bool right_click_holding = false;
bool right_click_released_once = false;

const int num_vertices_for_cube = 36;

float speed = 0.05; 
float camera_speed = 0.05;
float scale_amount = 0.00;
float radius = 0.1; // cube has side = 2*radius
float frame_cube_half_side_length = 1.05;
float inital_z_placement = -2.8;

point4 points_cube[num_vertices_for_cube];
color4 colors_cube[num_vertices_for_cube];

vec3 object_coordinates(0.3, 0.2, inital_z_placement);
const vec3 placement_frame(0.0, 0.0, inital_z_placement);
vec3 object_scale(1.0, 1.0, 1.0);

vec3 camera_coordinates(0.0, 0.0, -2.0);
vec3 camera_front(0.0f, 0.0f, -1.0f);
vec3 camera_up(0.0f, 1.0f, 0.0f);

// Initialize mouse position as the middle of the screen
double last_mouse_pos_x = SCREEN_WIDTH / 2.0;
double last_mouse_pos_y = SCREEN_HEIGHT / 2.0;

double x_pos_diff;
double y_pos_diff;

double yaw   = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
double pitch =  0.0f;
double lastX =  512 / 2.0;
double lastY =  512 / 2.0;
double fov   =  45.0f;

double last_yaw = yaw;
double last_pitch = pitch;

mat4 model, view, projection;
GLuint  Model, View, Projection;

GLuint vao[2];
GLuint cube_buffer;

// Array of rotation angles (in degrees) for each coordinate axis
enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
GLfloat Theta[NumAxes] = { 0.0, 0.0, 0.0 };
GLfloat Translation[NumAxes] = { 0.0, 0.0, 0.0 };

// Vertices of a unit cube centered at origin, sides aligned with axes
point4 cube_vertices[8] = {
    point4( -radius, -radius,  radius, 1.0 ),
    point4( -radius,  radius,  radius, 1.0 ),
    point4(  radius,  radius,  radius, 1.0 ),
    point4(  radius, -radius,  radius, 1.0 ),
    point4( -radius, -radius, -radius, 1.0 ),
    point4( -radius,  radius, -radius, 1.0 ),
    point4(  radius,  radius, -radius, 1.0 ),
    point4(  radius, -radius, -radius, 1.0 )
};

// quad generates two triangles for each face and assigns colors_cube to the vertices
int index_cube = 0;
void quad( int a, int b, int c, int d )
{
    int lst[] = {a, b, c, a, c, d};

    for (int i=0; i<6; i++)
    {
        points_cube[index_cube+i] = cube_vertices[lst[i]];
    }
    index_cube += 6;
}

// generate 12 triangles: 36 vertices and 36 colors_cube
void create_cube()
{
    index_cube = 0;
    quad( 1, 0, 3, 2 );
    quad( 2, 3, 7, 6 );
    quad( 3, 0, 4, 7 );
    quad( 6, 5, 1, 2 );
    quad( 4, 5, 6, 7 );
    quad( 5, 4, 0, 1 );
}

// create objects, vertex arrays and buffers
void init()
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Create vertex array objects
    glGenVertexArrays(2, vao);

    create_cube();

    glBindVertexArray(vao[0]);
    glGenBuffers(1, &cube_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, cube_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_cube) + sizeof(colors_cube), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points_cube), points_cube);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_cube), sizeof(colors_cube), colors_cube);

    // cube attribute object
    GLuint vPosition_cube = glGetAttribLocation(program, "vPosition");
    glEnableVertexAttribArray(vPosition_cube);
    glVertexAttribPointer(vPosition_cube, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    GLuint vColor_cube = glGetAttribLocation(program, "vColor");
    glEnableVertexAttribArray(vColor_cube);
    glVertexAttribPointer(vColor_cube, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points_cube)));
        
    // Retrieve transformation uniform variable locations
    Model = glGetUniformLocation( program, "Model" );
    View = glGetUniformLocation( program, "View" );
    Projection = glGetUniformLocation( program, "Projection" );

    model = Translate(object_coordinates);
    view = identity();
    
    // Set projection matrix
    GLfloat aspect_ratio = (GLfloat) SCREEN_WIDTH / (GLfloat) SCREEN_HEIGHT;
    projection = Perspective(60.0, aspect_ratio, 0.1, 15.5);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 
}

void draw_object()
{

}

void draw_selected_object()
{
    
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 translation(Translation[Xaxis], Translation[Yaxis], Translation[Zaxis]);
    vec3 scaling((object_scale.x + scale_amount), (object_scale.y + scale_amount), (object_scale.z + scale_amount));

    mat4 rotation_matrix =  Translate(object_coordinates) * 
                            RotateX( Theta[Xaxis] ) * RotateY( Theta[Yaxis] ) * RotateZ( Theta[Zaxis] ) * 
                            Translate(-object_coordinates);

    mat4 scaling_matrix = Translate(object_coordinates) * Scale(scaling) * Translate(-object_coordinates);

    // model = model * Translate(translation) * 
    //                 rotation_matrix * 
    //                 scaling_matrix;

    // TODO: EVERY TIME THE OBJECT MOVES ITS NEW POSITION OBJECT COORDINATES MUST BE GIVEN TO THE ROTATION AND SCALING MATRICES!!!!

    model = Translate(translation) * 
            rotation_matrix * 
            scaling_matrix * 
            model;
    
    // model =   Translate(translation + object_coordinates) * 
    //             rotation_matrix * 
    //             scaling_matrix;

    view = LookAt(camera_coordinates, camera_coordinates + camera_front, camera_up);

    glUniformMatrix4fv(Model, 1, GL_TRUE, model);
    glUniformMatrix4fv(View, 1, GL_TRUE, view);

    // Draw cube being moved
    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, cube_buffer);

    // Draw solid object
    std::fill_n(colors_cube, num_vertices_for_cube, color4(0.8, 0.8, 0.8, 1.0));
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_cube), sizeof(colors_cube), colors_cube);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_for_cube);

    // Draw same object with wireframe
    std::fill_n(colors_cube, num_vertices_for_cube, color4(0.0, 0.0, 0.0, 1.0));
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_cube), sizeof(colors_cube), colors_cube);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices_for_cube);


    // Generate different model-view matrix for all the stationary objects
    // mat4 model2 = Translate(placement_frame);
    //                 //* LookAt(camera_coordinates, camera_coordinates + camera_front, camera_up);
    // glUniformMatrix4fv(Model, 1, GL_TRUE, model2);

    // // Draw cube frame
    // glBindVertexArray(vao[0]);
    // glDrawArrays(GL_TRIANGLES, 0, num_vertices_for_cube);

    glFlush();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch(key) 
    {
        case GLFW_KEY_ESCAPE: //quit
            exit(EXIT_SUCCESS);
            break;

        // WASD and Mouse for camera movement
        // Arrow keys for selected object movement
        // Left-Right: Translate in X, Up-Down: Translate in Y, Shift+Up-Down: Translate in Z
        // 1, 2, 3: RotateX, RotateY, RotateZ
        // Shift + 1, 2, 3: Scale

        // Move camera:
        case GLFW_KEY_W:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                camera_coordinates += camera_speed * camera_front;   
            break;
        case GLFW_KEY_S:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                camera_coordinates -= camera_speed * camera_front;  
            break;
        case GLFW_KEY_A:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                camera_coordinates -= normalize(cross(camera_front, camera_up)) * camera_speed;  
            break;
        case GLFW_KEY_D:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                camera_coordinates += normalize(cross(camera_front, camera_up)) * camera_speed;
            break;
        case GLFW_KEY_Q:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                // camera_coordinates.y -= camera_speed;   
            break;
        case GLFW_KEY_E:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                // camera_coordinates.y -= camera_speed;   
            break;

        // Translate object:
        case GLFW_KEY_LEFT:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                Translation[Xaxis] = -speed;   
            if (action == GLFW_RELEASE)
                Translation[0] = Translation[1] = Translation[2] = 0.0;
            break;
        case GLFW_KEY_RIGHT:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                Translation[Xaxis] = speed;   
            if (action == GLFW_RELEASE)
                Translation[0] = Translation[1] = Translation[2] = 0.0;
            break;
        case GLFW_KEY_UP:
            if (action == GLFW_PRESS || action == GLFW_REPEAT)
            {
                if (mods & GLFW_MOD_SHIFT)
                    Translation[Zaxis] = -speed;
                else
                    Translation[Yaxis] = speed;
            }
            if (action == GLFW_RELEASE)
                Translation[0] = Translation[1] = Translation[2] = 0.0;
            break;
        case GLFW_KEY_DOWN:
            if (action == GLFW_PRESS || action == GLFW_REPEAT)
            {
                if (mods & GLFW_MOD_SHIFT)
                    Translation[Zaxis] = speed;
                else
                    Translation[Yaxis] = -speed;
            }
            if (action == GLFW_RELEASE)
                Translation[0] = Translation[1] = Translation[2] = 0.0;
            break;

        // Rotate object around its origin:
        case GLFW_KEY_1:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                if (mods & GLFW_MOD_SHIFT)
                    Theta[Xaxis] = -1.0;
                else
                    Theta[Xaxis] = 1.0;   
            if (action == GLFW_RELEASE)
                Theta[0] = Theta[1] = Theta[2] = 0.0;
            break;
        case GLFW_KEY_2:  
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                if (mods & GLFW_MOD_SHIFT)
                    Theta[Yaxis] = -1.0;   
                else
                    Theta[Yaxis] = 1.0;   
            if (action == GLFW_RELEASE)
                Theta[0] = Theta[1] = Theta[2] = 0.0;
            break;
        case GLFW_KEY_3:    
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
                if (mods & GLFW_MOD_SHIFT)
                    Theta[Zaxis] = -1.0;   
                else
                    Theta[Zaxis] = 1.0;   
            if (action == GLFW_RELEASE)
                Theta[0] = Theta[1] = Theta[2] = 0.0;
            break;

        // Scale object:
        case GLFW_KEY_4:
            if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            {
                if (mods & GLFW_MOD_SHIFT)
                    scale_amount = -0.01;
                else
                    scale_amount = 0.01;
            }  
            if (action == GLFW_RELEASE)
                scale_amount = 0.00;
            break;
    }
    
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        right_click_holding = true;
        right_click_released_once = false;
    }
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        right_click_holding = false;
        if (!right_click_released_once)
        {
            last_yaw = yaw;
            last_pitch = pitch;
        }
        right_click_released_once = true;
        
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    // Current mouse position is (xposIn, yposIn) 
    // We need the previous mouse position so that 
    // we can take the difference to know how the mouse moved
    x_pos_diff = xposIn - last_mouse_pos_x;
    y_pos_diff = yposIn - last_mouse_pos_y;

    // We store these to calculate the difference for the next time
    last_mouse_pos_x = xposIn;
    last_mouse_pos_y = yposIn;
       
    if (right_click_released_once)
    {
        yaw = last_yaw + x_pos_diff;
        pitch = last_pitch - y_pos_diff;
    }
    else
    {
        yaw += x_pos_diff;
        pitch -= y_pos_diff;
    }

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
  
    if (right_click_holding)
    {
        vec3 front;
        front.x = cos(yaw * (M_PI / 180.0)) * cos(pitch * (M_PI / 180.0));
        front.y = sin(pitch * (M_PI / 180.0));
        front.z = sin(yaw * (M_PI / 180.0)) * cos(pitch * (M_PI / 180.0));
        camera_front = normalize(front);       
    }
        
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    GLfloat aspect_ratio = (GLfloat) width / (GLfloat) height;

    if (width < height)
        projection = Perspective(atan(tan(60.0 * (M_PI/180)/2) * (1/aspect_ratio)) * (180/M_PI) * 2, aspect_ratio, 1.8, 5.5);
    else
        projection = Perspective(60.0, aspect_ratio, 1.8, 5.5);

    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

int main(int argc, char *argv[])
{
    if (argc == 2) 
        speed = abs(std::stof(argv[1]));

    if (!glfwInit())
        exit(EXIT_FAILURE);
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Scene Builder", NULL, NULL);
    glfwMakeContextCurrent(window);
    
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    
    init();

    double frameRate = 60, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        currentTime = glfwGetTime();
        if (currentTime - previousTime >= 1/frameRate)
        {
            previousTime = currentTime;
        }
        
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

