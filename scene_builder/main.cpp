#include "Angel.h"
#include "data_types.hpp"
#include "load_model.hpp"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#if defined(__linux__) || defined(_WIN32)
    #include <GL/gl.h>
#elif defined(__APPLE__)
#endif

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600
#define VAO_NUM 5 //TODO: change as necessary

#define INT_TO_UNIQUE_COLOR(x)                                                 \
  vec4(((GLfloat)(x & 0xFF)) / 255.0, ((GLfloat)((x >> 8) & 0xFF)) / 255.0,    \
       ((GLfloat)((x >> 16) & 0xFF)) / 255.0, 1.0)

#define UNIQUE_COLOR_TO_INT(c)                                                 \
  ((int)(c.x * 255) + ((int)(c.y * 255) << 8) + ((int)(c.z * 255) << 16))

typedef vec4 color4;
typedef vec4 point4;

bool display_picking = false;

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
float object_speed = 0.01;
float inital_z_placement = -2.8;
int selected_model_index = 0; // first object model is empty and selected
int num_of_objects = 0;       // first object model is empty and selected

mat4 view_matrix, projection_matrix;
GLuint Model, View, Projection;
GLuint vao[VAO_NUM]; // TODO: make these dynamically allocated with every new object added to scene
GLuint buffers[VAO_NUM];
GLuint vPosition;
GLuint vColor;

enum {Xaxis = 0, Yaxis = 1, Zaxis = 2, NumAxes = 3};
enum ObjectType {Cube, Sphere, Imported, PointLight, Empty};

struct object_model
{
    enum ObjectType object_type;
    struct model *model;

    vec3 object_coordinates;
    mat4 model_matrix;
    GLfloat Theta[NumAxes];
    GLfloat Scaling[NumAxes];
    GLfloat Translation[NumAxes];

    GLuint *vao;
    GLuint *buffer;
    int vertices_num;

    point4 *points_array;
    color4 *colors_array;
    color4 *picking_colors_array;
    color4 unique_id_color;

    bool is_selected;

    friend std::ostream& operator<<(std::ostream& os, const object_model& obj) {
        os << "is_selected: " << obj.is_selected << ", object_type: " << obj.object_type << ", coords: (" << obj.object_coordinates.x << ", " << obj.object_coordinates.y << 
            ", " << obj.object_coordinates.z << "), v_num: " << obj.vertices_num << ", id_color: (" << obj.unique_id_color.x <<
            ", " << obj.unique_id_color.y << ", " << obj.unique_id_color.z << ")";
        return os;
    }
};

struct object_model *empty_object;
struct object_model **object_models;
int object_models_size = 10;
struct model **models;
int models_size = 10;
int num_of_models = 0;

/* CUBE, SPHERE */
const int num_vertices_for_cube = 36;
const int num_vertices_for_sphere = 3*(4*((4*4*4*4))); // corresponds to create_sphere(4)
const int num_vertices_for_point_light = 3*(4*((4*4*4))); // corresponds to create_sphere(3)

float cube_half_side = 0.1;
float radius = 0.1;
int k = 0;

point4 points_sphere[num_vertices_for_sphere];
color4 colors_sphere[num_vertices_for_sphere];

point4 points_point_light[num_vertices_for_point_light];
color4 colors_point_light[num_vertices_for_point_light];

point4 points_cube[num_vertices_for_cube];
color4 colors_cube[num_vertices_for_cube];

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
}

// following 5 lines are excerpted from the textbook
point4 v[4]= {
    vec4(0.0, 0.0, 1.0, 1.0),
    vec4(0.0, 0.942809, -0.333333, 1.0),
    vec4(-0.816497, -0.471405, -0.333333, 1.0),
    vec4(0.816497, -0.471405, -0.333333, 1.0)
};

// we normalize a 4-d vector so that its eucledian-metric length will be 1 and thus touch the surface of the unit-sphere
point4 four_d_normalizer(const point4& p)
{
    float len = (p.x * p.x + p.y * p.y + p.z * p.z);
    point4 point;
    point = p / sqrt(len);
    point.w = 1.0;
    return point;
}

// following 3 functions are excerpted from the textbook
void triangle(point4 a, point4 b, point4 c, ObjectType obj_type)
{
    if (obj_type == Sphere)
    {
        points_sphere[k++] = a;
        points_sphere[k++] = b;
        points_sphere[k++] = c;
    }
    else 
    {
        points_point_light[k++] = a;
        points_point_light[k++] = b;
        points_point_light[k++] = c;
    }
}

void divide_triangle(point4 a, point4 b, point4 c, int n, ObjectType obj_type)
{
    if (n == 0) 
    {
        triangle(a, b, c, obj_type);
        return;
    }
    // Compute midpoints of edges
    point4 v1 = four_d_normalizer(a + b);
    point4 v2 = four_d_normalizer(a + c);
    point4 v3 = four_d_normalizer(b + c);

    // Recursively subdivide new triangles
    divide_triangle(a, v1, v2, n - 1, obj_type);
    divide_triangle(c, v2, v3, n - 1, obj_type);
    divide_triangle(b, v3, v1, n - 1, obj_type);
    divide_triangle(v1, v3, v2, n - 1, obj_type);
}

void create_sphere(int n, ObjectType obj_type)
{
    k = 0;
    divide_triangle(v[0], v[1], v[2], n, obj_type);
    divide_triangle(v[3], v[2], v[1], n, obj_type);
    divide_triangle(v[0], v[3], v[1], n, obj_type);
    divide_triangle(v[0], v[2], v[3], n, obj_type);
}

void create_object_matrices(struct object_model *obj)
{
    if (obj != nullptr)
    {
        if (obj->object_type != Empty)
        {
            vec3 translation;
            mat4 rotation_matrix = mat4(1.0f);
            mat4 scaling_matrix = mat4(1.0f);;
            
            translation = obj->object_coordinates + vec3(0.0, 0.0, inital_z_placement) + 
                            vec3(obj->Translation[Xaxis], obj->Translation[Yaxis], obj->Translation[Zaxis]);

            // PointLight cannot be scaled or rotated
            if (obj->object_type == PointLight)
            {
                obj->model_matrix = Translate(translation);
            }
            else
            {
                rotation_matrix =   Translate(translation) * 
                                    RotateX(obj->Theta[Xaxis]) * 
                                    RotateY(obj->Theta[Yaxis]) *
                                    RotateZ(obj->Theta[Zaxis]) * 
                                    Translate(-translation);
                
                scaling_matrix =    Translate(translation) * 
                                    Scale((1.0 + obj->Scaling[Xaxis]), 
                                        (1.0 + obj->Scaling[Yaxis]), 
                                        (1.0 + obj->Scaling[Zaxis])) * 
                                    Translate(-translation);

                obj->model_matrix = rotation_matrix * scaling_matrix * Translate(translation);
            }
        }
    }   
}

struct model load_model(std::string filename) 
{
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();
    file.close();

    serializable_model smodel;
    smodel.zax_from_json(json.c_str());

    printf("model loaded\n");

    // std::cout << "Loaded Model: " << smodel << std::endl;

    return to_model(smodel);
}

struct object_model *add_object(ObjectType obj_type)
{
    num_of_objects++;

    // reallocate memory to objects_model if necessary
    if (num_of_objects > object_models_size-3)
    {
        object_models_size *= 2;
        object_models = (struct object_model **) realloc(object_models, object_models_size * sizeof(struct object_model *));
        if (object_models == nullptr) 
        {
            perror("Error allocating memory to object_model in add_object\n");
            return nullptr;
        }
    }

    // allocate memory to object_model
    struct object_model *obj = (struct object_model *) malloc(sizeof(struct object_model));
    if (obj == nullptr) 
    {
        perror("Error allocating memory to obj in add_object\n");
        return nullptr;
    }

    // initialize object_model variables
    obj->object_type = obj_type;
    obj->model = nullptr;
    obj->object_coordinates = vec3(0.0f, 0.0f, 0.0f);
    obj->model_matrix = Translate(0.0, 0.0, inital_z_placement);
    std::fill_n(obj->Theta, 3, 0.0f);
    std::fill_n(obj->Scaling, 3, 0.0f);
    std::fill_n(obj->Translation, 3, 0.0f);
    obj->is_selected = false;

    struct model imported_model;
    float point_scaler;

    // initialize other varibles depending on object type
    switch (obj_type)
    {
    case Cube:
        obj->vao = &(vao[0]);
        obj->buffer = &(buffers[0]);
        obj->vertices_num = num_vertices_for_cube;

        obj->points_array = points_cube;
        obj->colors_array = colors_cube;
        break;
    
    case Sphere:
        obj->vao = &(vao[1]);
        obj->buffer = &(buffers[1]);
        obj->vertices_num = num_vertices_for_sphere;

        obj->points_array = points_sphere;
        obj->colors_array = colors_sphere;
        break;

    case PointLight:
        obj->vao = &(vao[2]);
        obj->buffer = &(buffers[2]);
        obj->vertices_num = num_vertices_for_point_light;

        obj->points_array = points_point_light;
        obj->colors_array = colors_point_light;
        break;

    case Imported:
        num_of_models++;
        if (num_of_models > models_size-3)
        {
            models_size *= 2;
            models = (struct model **) realloc(models, models_size * sizeof(struct model *));
            if (models == nullptr) 
            {
                perror("Error allocating memory to models in add_object\n");
                return nullptr;
            }
        }

        imported_model = load_model("arrow.json"); //FIXME:
        models[num_of_models-1] = &imported_model;
        obj->model = &imported_model;

        //////////////////////////////////////
        vec4 *vertices;
        int len;
        to_point_array(&vertices, &len, &imported_model.triangles);
        
        point_scaler = 0.1;
        for(int i=0; i<len; i++)
            vertices[i] = point4(vertices[i].x * point_scaler, vertices[i].y * point_scaler, vertices[i].z * point_scaler, 1.0);

        //////////////////////////////////////

        obj->vao = &(vao[4]); //FIXME: vao must dynamically allocated array
        obj->buffer = &(buffers[4]);
        obj->vertices_num = len;

        obj->points_array = vertices;

        //TODO: GET THESE VALUES FROM JSON FILE!!!
        obj->colors_array = (color4 *) malloc(obj->vertices_num * sizeof(color4)); 
        if (obj->colors_array == nullptr) 
            perror("Error allocating memory to curr_object_model->colors_array in draw_objects\n");
        else
            std::fill_n(obj->colors_array, obj->vertices_num, color4(0.8, 0.8, 0.8, 1.0)); // FIXME: THIS NEEDS TO CHANGE FOR SHADER_EDITOR


        /////////////

        glBindVertexArray(vao[4]);
        glBindBuffer(GL_ARRAY_BUFFER, buffers[4]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(point4) * obj->vertices_num + sizeof(color4) * obj->vertices_num, NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(point4) * obj->vertices_num, obj->points_array);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(point4) * obj->vertices_num, sizeof(color4) * obj->vertices_num, obj->colors_array);

        // cube attribute
        glEnableVertexAttribArray(vPosition);
        glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
        
        glEnableVertexAttribArray(vColor);
        glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(point4) * obj->vertices_num));
            
        /////////////

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

    // create unique picking colors array for object if not empty
    if (obj_type != Empty)
    {
        obj->picking_colors_array = (color4 *) malloc(obj->vertices_num * sizeof(color4));
        if (obj->picking_colors_array == nullptr) 
            perror("Error allocating memory to obj->picking_colors_array in add_object\n");
        else
        {
            obj->unique_id_color = INT_TO_UNIQUE_COLOR(num_of_objects);
            std::fill_n(obj->picking_colors_array, obj->vertices_num, obj->unique_id_color); 
        }   
    }

    create_object_matrices(obj);

    object_models[num_of_objects-1] = obj;
    std::cout << "Object #" << (num_of_objects-1) << " added: " << *(object_models[num_of_objects-1]) << std::endl;

    return obj;
}

void delete_all_objects()
{
    struct object_model *obj;
    for (int i = 0; i < num_of_objects; i++)
    {
        object_models[i] = obj;
        if (obj != nullptr)
        {
            if (obj->picking_colors_array != nullptr)
                free(obj->picking_colors_array);
            if (obj->colors_array != nullptr && obj->object_type == Imported)
                free(obj->colors_array);
            free(obj);
        }   
    }
    free(object_models);
}

void delete_selected_object()
{
    struct object_model *obj = object_models[selected_model_index];
    if (obj->object_type == Empty)
        return;

    if (obj != nullptr)
    {
        if (obj->picking_colors_array != nullptr)
            free(obj->picking_colors_array);
        if (obj->colors_array != nullptr && obj->object_type == Imported)
            free(obj->colors_array);
        free(obj);
    }   
    object_models[selected_model_index] = nullptr;

    selected_model_index = 0;
}

void print_all_objects()
{
    printf("==============================\n");
    for (int i = 0; i < num_of_objects; i++)
    {
        if (object_models[i] != nullptr)
            std::cout << "Object " << i << ": " << *(object_models[i]) << std::endl;
    }
    printf("==============================\n");
}

void create_object_arrows()
{

}

// create objects, vertex arrays and buffers
void init()
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader("vshader.glsl", "fshader.glsl");
    glUseProgram(program);

    // Create vertex array objects, buffers etc.
    glGenVertexArrays(VAO_NUM, vao);
    glGenBuffers(VAO_NUM, buffers);
    
    vPosition = glGetAttribLocation(program, "vPosition");
    vColor = glGetAttribLocation(program, "vColor");

    int index = 0;

    /* Cube */
    create_cube();
    std::fill_n(colors_cube, num_vertices_for_cube, color4(0.8, 0.8, 0.8, 1.0));

    glBindVertexArray(vao[index]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[index]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_cube) + sizeof(colors_cube), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points_cube), points_cube);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_cube), sizeof(colors_cube), colors_cube);

    // cube attribute
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points_cube)));
        
    /* Sphere */
    index++;
    float sphere_scaler = 0.1;
    create_sphere(4, Sphere);
    std::fill_n(colors_sphere, num_vertices_for_sphere, color4(0.8, 0.8, 0.8, 1.0));

    for(int i=0; i<num_vertices_for_sphere; i++)
        points_sphere[i] = point4(points_sphere[i].x * sphere_scaler, points_sphere[i].y * sphere_scaler, points_sphere[i].z * sphere_scaler, 1.0);

    glBindVertexArray(vao[index]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[index]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_sphere) + sizeof(colors_sphere),NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points_sphere), points_sphere);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_sphere), sizeof(colors_sphere), colors_sphere);
    
    // sphere attribute   
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points_sphere)));
           
    /* PointLight */
    index++;
    float point_light_scaler = 0.02;
    create_sphere(3, PointLight);
    std::fill_n(colors_point_light, num_vertices_for_point_light, color4(1.0, 1.0, 0.0, 1.0));

    for(int i=0; i<num_vertices_for_point_light; i++)
        points_point_light[i] = point4(points_point_light[i].x * point_light_scaler, 
            points_point_light[i].y * point_light_scaler, points_point_light[i].z * point_light_scaler, 1.0);


    glBindVertexArray(vao[index]);
    glBindBuffer(GL_ARRAY_BUFFER, buffers[index]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(points_point_light) + sizeof(colors_point_light),NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(points_point_light), points_point_light);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(points_point_light), sizeof(colors_point_light), colors_point_light);
    
    // point light attribute object    
    glEnableVertexAttribArray(vPosition);
    glVertexAttribPointer(vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));
    
    glEnableVertexAttribArray(vColor);
    glVertexAttribPointer(vColor, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(sizeof(points_point_light)));
        
    // Retrieve transformation uniform variable locations
    Model = glGetUniformLocation( program, "Model" );
    View = glGetUniformLocation( program, "View" );
    Projection = glGetUniformLocation( program, "Projection" );
    
    // Set projection matrix
    GLfloat aspect_ratio = (GLfloat) SCREEN_WIDTH / (GLfloat) SCREEN_HEIGHT;
    projection_matrix = Perspective(fovy, aspect_ratio, 0.1, 15.5);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection_matrix);

    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 

    // allocate memory for object_models array containing pointers to all object_models
    object_models = (struct object_model **) malloc(object_models_size * sizeof(struct object_model *));
    if (object_models == nullptr) 
        perror("Error allocating memory to object_models in init\n");
    else 
    {
        for (int i = 0; i < object_models_size; i++)
            object_models[i] = nullptr;
    }

    models = (struct model **) malloc(object_models_size * sizeof(struct model *));
    if (models == nullptr) 
        perror("Error allocating memory to models in init\n");
    else 
    {
        for (int i = 0; i < object_models_size; i++)
            models[i] = nullptr;
    }

    // create first empty object, this will be selected when background is clicked 
    empty_object = add_object(Empty);
}

void draw_objects(bool with_picking)
{
    // camera view matrix
    view_matrix = LookAt(camera_coordinates, camera_coordinates + camera_front, camera_up);
    glUniformMatrix4fv(View, 1, GL_TRUE, view_matrix);

    struct object_model *curr_object_model;
    GLsizeiptr buf_offset, buf_size;
    color4 *selected_color_array;

    for (int i = 1; i < num_of_objects; i++)
    {
        curr_object_model = object_models[i];
        if (curr_object_model != nullptr)
        {
            // color arrays and drawing according to selected, picking etc.
            glBindVertexArray(*(curr_object_model->vao));
            glBindBuffer(GL_ARRAY_BUFFER, *(curr_object_model->buffer));
            
            buf_offset = curr_object_model->vertices_num * sizeof(point4);
            buf_size = curr_object_model->vertices_num * sizeof(color4);

            // object model matrix
            glUniformMatrix4fv(Model, 1, GL_TRUE, curr_object_model->model_matrix);

            if (with_picking == false)
            {
                glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, curr_object_model->colors_array);
            
                // if object not selected then just draw it solid with its own color
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
                glDrawArrays(GL_TRIANGLES, 0, curr_object_model->vertices_num);

                // if object is selected then also draw its wireframe with black color
                if (curr_object_model->is_selected)
                {
                    selected_color_array = (color4 *) malloc(curr_object_model->vertices_num * sizeof(color4));
                    if (selected_color_array == nullptr)
                    {
                        perror("Error allocating memory to selected_color_array");
                        return;
                    }
                    std::fill_n(selected_color_array, curr_object_model->vertices_num, color4(0.0, 0.0, 0.0, 1.0));
                    glBufferSubData(GL_ARRAY_BUFFER, buf_offset, buf_size, selected_color_array);

                    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                    glDrawArrays(GL_TRIANGLES, 0, curr_object_model->vertices_num);
                    
                    free(selected_color_array);
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
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    draw_objects(false);

    glFlush();
}

void update()
{
    // only update the model matrix of the selected object
    create_object_matrices(object_models[selected_model_index]); 
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch(key) 
    {
    case GLFW_KEY_ESCAPE: //delete all objects and quit
        delete_all_objects(); 
        exit(EXIT_SUCCESS);
        break;

    case GLFW_KEY_C: // add new cube to screen
        if (action == GLFW_PRESS)
            add_object(Cube);
        break;
    case GLFW_KEY_V: // add new sphere to screen
        if (action == GLFW_PRESS)
            add_object(Sphere);
        break;
    case GLFW_KEY_B: // add new point light to screen
        if (action == GLFW_PRESS)
            add_object(PointLight);
        break;
    case GLFW_KEY_N: // add new imported object to screen
        if (action == GLFW_PRESS)
            add_object(Imported);
        break;
    case GLFW_KEY_SPACE: // increase selected_model_index
        if (action == GLFW_PRESS)
        {
            if (object_models[selected_model_index] != nullptr)
                object_models[selected_model_index]->is_selected = false;
            if (num_of_objects > 1)
            {
                // while (object_models[selected_model_index] != nullptr)
                    selected_model_index = (selected_model_index+1)%num_of_objects;
            }
            if (object_models[selected_model_index] != nullptr)
                object_models[selected_model_index]->is_selected = true;
        }
        break;

    case GLFW_KEY_P:
        if (action == GLFW_PRESS)
            print_all_objects();
        break;
    case GLFW_KEY_BACKSPACE: //delete selected object
        if (action == GLFW_PRESS)
            delete_selected_object();
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
        draw_objects(true);
        glFlush();
        
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        
        xpos*=(fb_width/win_width);
        ypos*=(fb_height/win_height);
        ypos = fb_height - ypos;
                
        unsigned char pixel[4];
        glReadPixels(xpos, ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        vec4 color = vec4(pixel[0] / 255.0, pixel[1] / 255.0, pixel[2] / 255.0, 1.0);
        int index = UNIQUE_COLOR_TO_INT(color);

        // printf("index: %d, selected_model_index before: %d, color is: (%.4f, %.4f, %.4f)\n", index, selected_model_index, color.x, color.y, color.z);
        if (0 <= index-1 && index-1 < num_of_objects)
        {
            object_models[selected_model_index]->is_selected = false;
            selected_model_index = index-1;
            object_models[selected_model_index]->is_selected = true;
        }
        // printf("index: %d, selected_model_index after:  %d, color is: (%.4f, %.4f, %.4f)\n", index, selected_model_index, color.x, color.y, color.z);
        
        if (display_picking)
            glfwSwapBuffers(window);
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

    projection_matrix = Perspective(fovy_changed, aspect_ratio, 1.8, 5.5);
    glUniformMatrix4fv(Projection, 1, GL_TRUE, projection_matrix);
}

int main(int argc, char *argv[])
{
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

    double frameRate = 60.0, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        currentTime = glfwGetTime();
        if (currentTime - previousTime > 1.0/frameRate)
        {
            previousTime = currentTime;
            // update();
        }
        update();

        display();
        if (!display_picking)
            glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}