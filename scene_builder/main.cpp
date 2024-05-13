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

typedef vec4 color4;
typedef vec4 point4;

/* CAMERA */
bool right_click_holding = false;
bool right_click_released_once = false;

// Initialize mouse position as the middle of the screen
double last_mouse_pos_x = SCREEN_WIDTH / 2.0;
double last_mouse_pos_y = SCREEN_HEIGHT / 2.0;
double x_pos_diff;
double y_pos_diff;

// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a 
// direction vector pointing to the right so we initially rotate a bit to the left.
double yaw   = -90.0;	
double pitch =  0.0;
double last_yaw = yaw;
double last_pitch = pitch;

vec3 camera_coordinates(0.0, 0.0, -2.0);
vec3 camera_front(0.0f, 0.0f, -1.0f);
vec3 camera_up(0.0f, 1.0f, 0.0f);

float camera_speed = 0.05;
GLfloat fovy = 60.0;

/* OBJECT */
const int num_vertices_for_cube = 36;
float object_speed = 0.05;
float cube_half_side = 0.1;
float inital_z_placement = -2.8;

point4 points_cube[num_vertices_for_cube];
color4 colors_cube[num_vertices_for_cube];

int selected_model_index = 0; // first object model is empty and selected
int num_of_objects = 0;       // first object model is empty and selected

mat4 view, projection;
GLuint Model, View, Projection;
GLuint vao[2]; // TODO: make these dynamically allocated with every new object added to scene
GLuint buffers[2];

enum { Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3 };
enum ObjectType { Cube = 0, Sphere = 1, Imported = 2, Empty = 3};

struct object_model
{
    vec3 object_coordinates;
    mat4 model;
    GLfloat Theta[NumAxes];
    GLfloat Scaling[NumAxes];
    GLfloat Translation[NumAxes];

    GLuint *vao;
    GLuint *buffer;
    int vertices_num;

    point4 *points_array;
    color4 *colors_array;
    color4 *picking_colors_array;
    // color4 unique_color;

    bool is_selected;

    friend std::ostream& operator<<(std::ostream& os, const object_model& obj) {
        os << "object_coordinates: (" << obj.object_coordinates.x << ", " << obj.object_coordinates.y << 
            ", " << obj.object_coordinates.z << "), int vertices num: " << obj.vertices_num;
        return os;
    }
};

struct object_model **object_models;
int object_models_size = 10;


// Vertices of a unit cube centered at origin, sides aligned with axes
point4 cube_vertices[8] = {
    point4( -cube_half_side, -cube_half_side,  cube_half_side, 1.0 ),
    point4( -cube_half_side,  cube_half_side,  cube_half_side, 1.0 ),
    point4(  cube_half_side,  cube_half_side,  cube_half_side, 1.0 ),
    point4(  cube_half_side, -cube_half_side,  cube_half_side, 1.0 ),
    point4( -cube_half_side, -cube_half_side, -cube_half_side, 1.0 ),
    point4( -cube_half_side,  cube_half_side, -cube_half_side, 1.0 ),
    point4(  cube_half_side,  cube_half_side, -cube_half_side, 1.0 ),
    point4(  cube_half_side, -cube_half_side, -cube_half_side, 1.0 )
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

    std::fill_n(colors_cube, num_vertices_for_cube, color4(0.8, 0.8, 0.8, 1.0));
}

// add new cube object
void add_object(ObjectType object_type)
{
    num_of_objects++;
    if (num_of_objects > object_models_size-3)
    {
        object_models_size *= 2;
        object_models = (struct object_model **) realloc(object_models, object_models_size * sizeof(struct object_model *));
        if (object_models == nullptr) 
        {
            perror("Error allocating memory to object_model in add_object\n");
            return;
        }
    }

    struct object_model *obj = (struct object_model *) malloc(sizeof(struct object_model));
    if (obj == nullptr) 
    {
        perror("Error allocating memory to obj in add_object\n");
        return;
    }

    obj->object_coordinates = vec3(0.0f, 0.0f, 0.0f);
    obj->model = mat4(1.0f);
    std::fill_n(obj->Theta, 3, 0.0f);
    std::fill_n(obj->Scaling, 3, 0.0f);
    std::fill_n(obj->Translation, 3, 0.0f);
    obj->is_selected = false;

    switch (object_type)
    {
    case Cube:
        obj->vao = &(vao[0]);
        obj->buffer = &(buffers[0]);
        obj->vertices_num = num_vertices_for_cube;

        obj->points_array = points_cube;
        obj->colors_array = colors_cube;
        break;
    
    case Sphere:
        //TODO: do this for sphere
        break;

    case Imported:
        //TODO: GET THESE VALUES FROM JSON FILE!!!
        break;

    case Empty:
        obj->vao = NULL;
        obj->buffer = NULL;
        obj->vertices_num = 0;

        obj->points_array = nullptr;
        obj->colors_array = nullptr;
        obj->picking_colors_array = nullptr;
        break;
    }

    // create unique picking colors array for object
    // TODO: FIXME: CREATE UNIQUE COLOR ARRAYS (could be done using numObjects)
    // obj->picking_colors_array = new color4[obj->vertices_num];

    if (object_type != Empty)
    {
        obj->picking_colors_array = (color4 *) malloc(obj->vertices_num * sizeof(color4));
        if (obj->picking_colors_array == nullptr) 
        {
            perror("Error allocating memory to obj->picking_colors_array in add_object\n");
            return;
        }
        std::fill_n(obj->picking_colors_array, obj->vertices_num, color4(1.0, 0.0, 0.0, 1.0)); 
    }

    std::cout << "Object obj: " << *obj << std::endl;

    object_models[num_of_objects-1] = obj;

    std::cout << "Object oms: " << *(object_models[num_of_objects-1]) << std::endl;
}

void delete_all_objects() // FIXME: fix this
{
    struct object_model *obj;
    for (int i = 0; i < num_of_objects; i++)
    {
        object_models[i] = obj;
        free(obj->picking_colors_array);
        free(obj->colors_array);
    }
    free(object_models);
}

void print_all_objects()
{
    printf("==============================\n");
    for (int i = 0; i < num_of_objects; i++)
    {
        std::cout << "Object " << i << ": " << *(object_models[i]) << std::endl;
    }
    printf("==============================\n");
}

// create objects, vertex arrays and buffers
void init()
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Create vertex array objects
    glGenVertexArrays(2, vao);
    glGenBuffers(2, buffers);

    /* Cube */
    create_cube();

    glBindVertexArray(vao[0]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
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
    
    // Set projection matrix
    GLfloat aspect_ratio = (GLfloat) SCREEN_WIDTH / (GLfloat) SCREEN_HEIGHT;
    projection = Perspective(fovy, aspect_ratio, 0.1, 15.5);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);

    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 

    /////
    object_models = (struct object_model **) malloc(object_models_size * sizeof(struct object_model *));
    if (object_models == nullptr) 
        perror("Error allocating memory to object_model in init\n");
    else 
    {
        for (int i = 0; i < object_models_size; i++)  // FIXME: is this necessary?
            object_models[i] = nullptr;
    }

    add_object(Empty);
}

void draw_objects(bool with_picking)
{
    vec3 translation;
    mat4 rotation_matrix;
    mat4 scaling_matrix;
    struct object_model *curr_object_model;
    GLsizeiptr buf_offset, buf_size;

    for (int i = 1; i < num_of_objects; i++)
    {
        curr_object_model = object_models[i];

        // std::cout << "Object d_o: " << *curr_object_model << std::endl;

        if (curr_object_model == nullptr)
        {
            perror("Current object pointer is null in draw_objects\n");
            return;
        }

        // Matrices
        // TODO: these matrices should only change when object is selected and stored in the model otherwise,
        //      no need to do these again and again
        translation = curr_object_model->object_coordinates + vec3(0.0, 0.0, inital_z_placement) + 
                        vec3(curr_object_model->Translation[Xaxis], curr_object_model->Translation[Yaxis], curr_object_model->Translation[Zaxis]);

        rotation_matrix =   Translate(translation) * 
                            RotateX(curr_object_model->Theta[Xaxis]) * RotateY(curr_object_model->Theta[Yaxis]) * RotateZ(curr_object_model->Theta[Zaxis]) * 
                            Translate(-translation);
        
        scaling_matrix =    Translate(translation) * 
                            Scale((1.0 + curr_object_model->Scaling[Xaxis]), (1.0 + curr_object_model->Scaling[Yaxis]), (1.0 + curr_object_model->Scaling[Zaxis])) * 
                            Translate(-translation);

        curr_object_model->model = rotation_matrix * scaling_matrix * Translate(translation);
        glUniformMatrix4fv(Model, 1, GL_TRUE, curr_object_model->model); // FIXME: not sure if these should be here of before glDrawArrays

        view = LookAt(camera_coordinates, camera_coordinates + camera_front, camera_up); // FIXME: can this be out of for loop?
        glUniformMatrix4fv(View, 1, GL_TRUE, view);

        // color arrays and drawing according to selected, picking etc.
        glBindVertexArray(*(curr_object_model->vao)); //FIXME: FIXME: FIXME: GIVES SEGMENTATION FAULT!!!
        glBindBuffer(GL_ARRAY_BUFFER, *(curr_object_model->buffer));

        // glBindVertexArray(vao[0]); //FIXME: FIXME: FIXME: GIVES SEGMENTATION FAULT!!!
        // glBindBuffer(GL_ARRAY_BUFFER, buffers[0]);
        
        buf_offset = curr_object_model->vertices_num * sizeof(point4);
        buf_size = curr_object_model->vertices_num * sizeof(color4);

        if (with_picking == false)
        {
            // if no colors array then just draw object as gray
            if (curr_object_model->colors_array == NULL)
            {
                printf("object color array is null\n");
                // curr_object_model->colors_array = new color4[curr_object_model->vertices_num]; 
                curr_object_model->colors_array = (color4 *) malloc(curr_object_model->vertices_num * sizeof(color4)); 
                if (curr_object_model->colors_array == nullptr) 
                {
                    perror("Error allocating memory to curr_object_model->colors_array in draw_objects\n");
                    return;
                }
                std::fill_n(curr_object_model->colors_array, curr_object_model->vertices_num, color4(0.8, 0.8, 0.8, 1.0)); // FIXME: THIS NEEDS TO CHANGE FOR SHADER_EDITOR
            }
            glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, curr_object_model->colors_array);
        
            // if object not selected then just draw it solid with its own color
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, curr_object_model->vertices_num);

            // if object is selected then also draw its wireframe with black
            // printf("i: %d is_selected: %d\n", i, curr_object_model->is_selected);
            if (curr_object_model->is_selected)
            {
                std::fill_n(curr_object_model->colors_array, curr_object_model->vertices_num, color4(0.0, 0.0, 0.0, 1.0));
                glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, curr_object_model->colors_array);

                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                glDrawArrays(GL_TRIANGLES, 0, curr_object_model->vertices_num);

                std::fill_n(curr_object_model->colors_array, curr_object_model->vertices_num, color4(0.8, 0.8, 0.8, 1.0));
            }
        }
        else // if with picking then just draw all objects with their unique picking colors
        {
            glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, curr_object_model->picking_colors_array);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, curr_object_model->vertices_num);
        }
        
    }
}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_objects(false);

    glFlush();
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch(key) 
    {
    case GLFW_KEY_ESCAPE: //quit
        // delete_all_objects(); //FIXME:
        exit(EXIT_SUCCESS);
        break;

    case GLFW_KEY_C: // add new cube to screen
        if (action == GLFW_PRESS)
        {
            add_object(Cube);
        }
        break;
    case GLFW_KEY_V: // increase selected_model_index
        if (action == GLFW_PRESS)
        {
            object_models[selected_model_index]->is_selected = false;
            if (num_of_objects > 1)
                selected_model_index = (selected_model_index+1)%num_of_objects;
            object_models[selected_model_index]->is_selected = true;
        }
        break;
    case GLFW_KEY_P:
        if (action == GLFW_PRESS)
        {
            print_all_objects();
        }
        break;

    // WASD and Mouse for camera movement (while right mouse button is being pressed)
    // Arrow keys for selected object movement
    // Left-Right: Translate in X, Up-Down: Translate in Y, Shift+Up-Down: Translate in Z
    // 1, 2, 3: RotateX, RotateY, RotateZ (+Shift for reverse)
    // 4, 5, 6: Scale in x, y, z (+Shift for reverse)

    // Move camera:
    case GLFW_KEY_W:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates += camera_speed * camera_front;   
        break;
    case GLFW_KEY_S:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates -= camera_speed * camera_front;  
        break;
    case GLFW_KEY_A:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates -= normalize(cross(camera_front, camera_up)) * camera_speed;  
        break;
    case GLFW_KEY_D:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates += normalize(cross(camera_front, camera_up)) * camera_speed;
        break;
    case GLFW_KEY_Q:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates.y -= camera_speed;
        break;
    case GLFW_KEY_E:
        if ((action == GLFW_PRESS || action == GLFW_REPEAT) && right_click_holding) 
            camera_coordinates.y += camera_speed;
        break;

    // Move object:
    case GLFW_KEY_LEFT:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            object_models[selected_model_index]->Translation[Xaxis] += -object_speed; 
        break;
    case GLFW_KEY_RIGHT:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            object_models[selected_model_index]->Translation[Xaxis] += object_speed;   
        break;
    case GLFW_KEY_UP:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Translation[Zaxis] += -object_speed;
            else
                object_models[selected_model_index]->Translation[Yaxis] += object_speed;
        }
        break;
    case GLFW_KEY_DOWN:
        if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Translation[Zaxis] += object_speed;
            else
                object_models[selected_model_index]->Translation[Yaxis] += -object_speed;
        }
        break;

    // Rotate object
    case GLFW_KEY_1:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Theta[Xaxis] -= 1.0;
            else
                object_models[selected_model_index]->Theta[Xaxis] += 1.0;   
        break;
    case GLFW_KEY_2:  
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Theta[Yaxis] -= 1.0;   
            else
                object_models[selected_model_index]->Theta[Yaxis] += 1.0;   
        break;
    case GLFW_KEY_3:    
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Theta[Zaxis] -= 1.0;   
            else
                object_models[selected_model_index]->Theta[Zaxis] += 1.0;   
        break;

    // Scale object:
    case GLFW_KEY_4:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
        {
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Scaling[Xaxis] -= 0.01;
            else
                object_models[selected_model_index]->Scaling[Xaxis] += 0.01;
        }  
        break;
    case GLFW_KEY_5:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
        {
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Scaling[Yaxis] -= 0.01;
            else
                object_models[selected_model_index]->Scaling[Yaxis] += 0.01;
        }  
        break;
    case GLFW_KEY_6:
        if (action == GLFW_PRESS || action == GLFW_REPEAT) 
        {
            if (mods & GLFW_MOD_SHIFT)
                object_models[selected_model_index]->Scaling[Zaxis] -= 0.01;
            else
                object_models[selected_model_index]->Scaling[Zaxis] += 0.01;
        } 
        break;
    }
    
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        //For picking
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // draw_rubiks_cube(true);
        glFlush(); 
        
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        
        x*=(fb_width/win_width);
        y*=(fb_height/win_height);
        y = fb_height - y;
                
        float pixel[4];
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, pixel);

        // float a = pixel[2] *100;
        // int b = (int) a;

        // if (random_end <= 0 && b != 14)
        // {
        //     chosen_cubes_arr[click_num] = (int)(pixel[2] *10);

        //     click_num++;

        //     if (click_num > 3) 
        //     {
        //         std::sort(chosen_cubes_arr, chosen_cubes_arr+4);
        //         int face = chosen_cube_face(chosen_cubes_arr);
        //         if (face != -1)
        //             enter_input(face, 0);

        //         click_num = 0;
        //     }
        // }
    }

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

    double mouse_sensitivity = 0.5;
    x_pos_diff *= mouse_sensitivity;
    y_pos_diff *= mouse_sensitivity;

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

    if (pitch > 89.9)
        pitch = 89.9;
    if (pitch < -89.9)
        pitch = -89.9;
  
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
    GLfloat fovy_changed;

    if (width < height)
        fovy_changed = atan(tan(fovy * (M_PI/180)/2) * (1/aspect_ratio)) * (180/M_PI) * 2;
    else
        fovy_changed = fovy;

    projection = Perspective(fovy_changed, aspect_ratio, 1.8, 5.5);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection);
}

int main(int argc, char *argv[])
{
    if (argc == 2) 
        object_speed = abs(std::stof(argv[1]));

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

