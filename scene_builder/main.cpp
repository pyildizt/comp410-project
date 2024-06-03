#include "Angel.h"
#include "data_types.hpp"
#include "load_model.hpp"
#include "shaded_object.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

#if defined(__linux__) || defined(_WIN32)
    #include <GL/gl.h>
#elif defined(__APPLE__)
#endif

#define INT_TO_UNIQUE_COLOR(x)                                                 \
  vec4(((GLfloat)(x & 0xFF)) / 255.0, ((GLfloat)((x >> 8) & 0xFF)) / 255.0,    \
       ((GLfloat)((x >> 16) & 0xFF)) / 255.0, 1.0)

#define UNIQUE_COLOR_TO_INT(c)                                                 \
  ((int)(c.x * 255) + ((int)(c.y * 255) << 8) + ((int)(c.z * 255) << 16))

#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 600

#define SHADER_EDITOR_EXE_PATH "../shader_editor/shader_editor"

#define CUBE_PATH "../test/cube.json"
#define SPHERE_PATH "../test/tetrahedron.json"
#define POINTLIGHT_PATH "../test/tetrahedron.json" //FIXME: smaller yellow sphere
#define ARROW_PATH_1 "../test/arrow.json"
#define ARROW_PATH_2 "../test/arrow.json" //FIXME: different colored arrows
#define ARROW_PATH_3 "../test/arrow.json"

typedef vec4 color4;
typedef vec4 point4;

bool display_picking = false;

/* CAMERA */
bool left_click_holding = false;
bool right_click_holding = false;
bool right_click_released_once = false;

// Initialize mouse position as the middle of the screen
double last_mouse_pos_x = SCREEN_WIDTH / 2.0;
double last_mouse_pos_y = SCREEN_HEIGHT / 2.0;
double x_pos_diff;
double y_pos_diff;
double yaw = -90.0;	
double pitch =  0.0;
double last_yaw = yaw;
double last_pitch = pitch;

vec3 camera_coordinates(0.0, 0.0, -2.0f);
vec3 camera_front(0.0f, 0.0f, -1.0f);
vec3 camera_up(0.0f, 1.0f, 0.0f);
vec3 camera_at(0.0, 0.0, -3.0f);

float camera_speed = 0.05;
GLfloat fovy = 60.0;

/* OBJECT */
bool object_arrow_selected = false;
int object_arrow_selected_axis; //x=1, y=2, z=3

float object_speed = 0.005;
float inital_z_placement = -2.8;
int selected_model_index = 0; // first object model is empty and selected
int num_of_objects = 0;       // first object model is empty and selected
int num_of_models = 4;

std::map<std::string, GLuint> texturePointers;

GLuint program, picker_program;
mat4 view_matrix, projection_matrix;
mat4 arrow_model_matrices[3];
vec4 light_position(1.0, 1.0, 1.0, 1.0); // light position for the shaded_object.display_real function
// TODO: light poisiton might be tweaked

enum SelectedAction {NoAction, TranslateObject, ScaleObject, RotateObject};
SelectedAction selected_action = NoAction;

sobj::shaded_object *cube_shaded_object;
sobj::shaded_object *sphere_shaded_object;
sobj::shaded_object *pointLight_shaded_object;
sobj::shaded_object *arrow_shaded_objects[3];
sobj::shaded_object **shaded_objects;
int shaded_objects_size = 10;

struct object_model *empty_object;
struct object_model *pointLight_object;
struct object_model **object_models;
int object_models_size = 10;

sobj::shaded_object* get_empty_object()
{
    sobj::shaded_object obj = sobj::shaded_object();
    auto ptr = (sobj::shaded_object *) malloc(sizeof(sobj::shaded_object));
    memcpy(ptr, &obj, sizeof(sobj::shaded_object));
    return ptr;
}

void create_object_matrices(struct object_model *obj)
{
    if (obj != nullptr)
    {
        if (obj->object_type != Empty)
        {
            mat4 rotation_matrix = mat4(1.0f);
            mat4 scaling_matrix = mat4(1.0f);

            // TODO: changed this but did not check it
            obj->object_coordinates += vec3(obj->Translation[Xaxis], obj->Translation[Yaxis], obj->Translation[Zaxis]);

            // PointLight cannot be scaled or rotated
            if (obj->object_type == PointLight)
            {
                obj->model_matrix = Translate(obj->object_coordinates);
            }
            else
            {
                rotation_matrix =   RotateX(obj->Theta[Xaxis]) * 
                                    RotateY(obj->Theta[Yaxis]) *
                                    RotateZ(obj->Theta[Zaxis]);
                
                scaling_matrix =    Scale((1.0 + obj->Scaling[Xaxis]), 
                                        (1.0 + obj->Scaling[Yaxis]), 
                                        (1.0 + obj->Scaling[Zaxis]));
                
                // This is to make rotation around fixed global axes
                obj->rotation_matrix = rotation_matrix * obj->rotation_matrix;
                obj->model_matrix = Translate(obj->object_coordinates) * obj->rotation_matrix * scaling_matrix;
            }

            // make matrices of x, y, z arrows respectively
            float distance_from_object = 0.2f;
            if (obj->object_type == PointLight)
                distance_from_object = 0.05f;
                
            switch (selected_action)
            {
            case NoAction:
                break;
            
            case TranslateObject:
                arrow_model_matrices[0] = Translate(obj->object_coordinates) * Translate(distance_from_object, 0, 0) * RotateZ(-90.0f);
                arrow_model_matrices[1] = Translate(obj->object_coordinates) * Translate(0, distance_from_object, 0);
                arrow_model_matrices[2] = Translate(obj->object_coordinates) * Translate(0, 0, distance_from_object) * RotateX(90.0f);
                break;

            case ScaleObject:
                arrow_model_matrices[1] = Translate(obj->object_coordinates) * Translate(0, distance_from_object, 0);
                break;

            case RotateObject:
                arrow_model_matrices[0] = Translate(obj->object_coordinates) * Translate(distance_from_object, 0, 0);
                arrow_model_matrices[1] = Translate(obj->object_coordinates) * Translate(0, 0, distance_from_object) * RotateZ(-90.0f);
                arrow_model_matrices[2] = Translate(obj->object_coordinates) * Translate(0, distance_from_object, 0) * RotateZ(-90.0f);
                break;
            }
        }
    }   
}

void draw_object_arrows(struct object_model *obj, bool with_picking)
{
    if (selected_action == NoAction)
        return;

    // y-axis arrow
    if (with_picking)
        arrow_shaded_objects[1]->display_picker(view_matrix * arrow_model_matrices[1], projection_matrix);
    else
        arrow_shaded_objects[1]->display_real(view_matrix * arrow_model_matrices[1], projection_matrix, light_position);

    if (selected_action == ScaleObject)
        return;

    // x-axis arrow
    if (with_picking)
        arrow_shaded_objects[0]->display_picker(view_matrix * arrow_model_matrices[0], projection_matrix);
    else
        arrow_shaded_objects[0]->display_real(view_matrix * arrow_model_matrices[0], projection_matrix, light_position);

    // z-axis arrow
    if (with_picking)
        arrow_shaded_objects[2]->display_picker(view_matrix * arrow_model_matrices[2], projection_matrix);
    else
        arrow_shaded_objects[2]->display_real(view_matrix * arrow_model_matrices[2], projection_matrix, light_position);
}


// Add object button function, if obj_type == Imported then filename string "exmaple.json" with path must be provided
struct object_model *add_object(ObjectType obj_type, const char *filename)
{
    // If obj_type == Imported and filename is incorrect then give error and return
    if (obj_type == Imported)
    {
        std::ifstream file2(filename);
        if (!file2) 
        {
            std::cerr << "Error: Unable to open file " << filename << std::endl;
            return nullptr;
        }
    }

    num_of_objects++;

    // reallocate memory to objects_model if necessary
    if (num_of_objects > object_models_size)
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
    obj->object_coordinates = vec3(0.0, 0.0, inital_z_placement);
    obj->model_matrix = Translate(0.0, 0.0, inital_z_placement);
    obj->rotation_matrix = mat4(1.0f);
    std::fill_n(obj->Theta, 3, 0.0f);
    std::fill_n(obj->Scaling, 3, 0.0f);
    std::fill_n(obj->Translation, 3, 0.0f);
    obj->is_selected = false;

    sobj::shaded_object *shaded_obj;
    // initialize other varibles depending on object type
    switch (obj_type)
    {
    // If objects are cube, sphere then no need to load new model
    case Cube:
        obj->shaded_object_index = 0;
        break;

    case Sphere:
        obj->shaded_object_index = 1;
        break;

    case PointLight:
        obj->shaded_object_index = 2;
        break;

    case Imported:
        num_of_models++;

        //reallocate memory to shaded_objects if necessary
        if (num_of_models > shaded_objects_size)
        {
            shaded_objects_size *= 2;
            shaded_objects = (sobj::shaded_object **) realloc(shaded_objects, shaded_objects_size * sizeof(sobj::shaded_object *));
            if (shaded_objects == nullptr) 
            {
                perror("Error allocating memory to shaded_objects in add_object\n");
                return nullptr;
            }
        }

        // allocate memory to new shaded_object to be loaded
        shaded_obj = get_empty_object();
        if (shaded_obj == nullptr) 
        {
            perror("Error allocating memory to shaded_obj in add_object\n");
            return nullptr;
        }
        // load shaded object model
        shaded_obj->load_model_from_json(filename, texturePointers);
        shaded_obj->Program = program;
        shaded_obj->PickerProgram = picker_program;
        shaded_obj->initModel();
        // to keep track if object is duplicated later
        obj->shaded_object_index = num_of_models-1;
        // add the new shaded object to list
        shaded_objects[num_of_models-1] = shaded_obj;
        break;

    case Empty:
        // obj->shaded_object_index = -1;
        break;
    }

    // if not empty then give programs and also create unique color
    if (obj_type != Empty)
    {
        obj->unique_id_color = INT_TO_UNIQUE_COLOR(num_of_objects + 4); // 0: Empty, 1-3: reserved for arrows so +4

        shaded_obj->Program = program;
        shaded_obj->PickerProgram = picker_program;
        shaded_obj->unique_color = obj->unique_id_color;
    }

    // add object to object_models array
    object_models[num_of_objects-1] = obj;
    std::cout << "Object #" << (num_of_objects-1) << " added: " << *(object_models[num_of_objects-1]) << std::endl;

    return obj;
}

struct object_model *duplicate_selected_object()
{
    // if selected_object_index == 0 (no selected object) do nothing
    if (selected_model_index <= 0)
        return nullptr;

    num_of_objects++;
    // reallocate memory to objects_model if necessary
    if (num_of_objects > object_models_size-3)
    {
        object_models_size *= 2;
        object_models = (struct object_model **) realloc(object_models, object_models_size * sizeof(struct object_model *));
        if (object_models == nullptr) 
        {
            perror("Error allocating memory to object_model in duplicate_selected_object\n");
            return nullptr;
        }
    }

    // allocate memory to object_model
    struct object_model *new_obj = (struct object_model *) malloc(sizeof(struct object_model));
    if (new_obj == nullptr) 
    {
        perror("Error allocating memory to new_obj in duplicate_selected_object\n");
        return nullptr;
    }

    struct object_model *old_obj = object_models[selected_model_index];
    // Do not duplicate Point Light
    if (old_obj->object_type == PointLight)
        return nullptr;

    // Point to original shaded_object (index in shaded_objects array)
    // Since no need to import shaded object when only new matrices are enough to draw it again
    new_obj->shaded_object_index = old_obj->shaded_object_index;
    
    // Duplicate object information
    new_obj->object_type = old_obj->object_type;
    new_obj->object_coordinates = old_obj->object_coordinates;
    new_obj->model_matrix = old_obj->model_matrix;
    new_obj->rotation_matrix = old_obj->rotation_matrix;
    std::copy_n(old_obj->Theta, 3, new_obj->Theta);
    std::copy_n(old_obj->Scaling, 3, new_obj->Scaling);
    std::copy_n(old_obj->Translation, 3, new_obj->Translation);

    // add object to object_models array
    object_models[num_of_objects-1] = old_obj;
    std::cout << "Object #" << (num_of_objects-1) << " duplicated: " << *(object_models[num_of_objects-1]) << std::endl;

    // Select new object
    old_obj->is_selected = false;
    new_obj->is_selected = true;
    return new_obj;
}

// free object memory at deletion
void delete_selected_object()
{
    struct object_model *obj = object_models[selected_model_index];
    if (obj->object_type == Empty || obj->object_type == PointLight)
        return;

    if (obj != nullptr)
        free(obj);
 
    // also remove object from object_models array but do not change num_of_objects
    object_models[selected_model_index] = nullptr;

    // make object selection empty
    selected_model_index = 0;
}

// for debugging
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

// load and create cube, sphere and the arrow model for object transformation
void create_basic_objects()
{
    
    cube_shaded_object = get_empty_object();
    cube_shaded_object->load_model_from_json(CUBE_PATH, texturePointers);
    cube_shaded_object->Program = program;
    cube_shaded_object->PickerProgram = picker_program;
    cube_shaded_object->initModel();
    printf("cube loaded\n");

    sphere_shaded_object = get_empty_object();
    sphere_shaded_object->load_model_from_json(SPHERE_PATH, texturePointers);
    sphere_shaded_object->Program = program;
    sphere_shaded_object->PickerProgram = picker_program;
    sphere_shaded_object->initModel();
    printf("sphere loaded\n");

    pointLight_shaded_object =  get_empty_object();
    pointLight_shaded_object->load_model_from_json(POINTLIGHT_PATH, texturePointers);
    pointLight_shaded_object->Program = program;
    pointLight_shaded_object->PickerProgram = picker_program;
    pointLight_shaded_object->initModel();
    printf("pointLight loaded\n");

    arrow_shaded_objects[0] =  get_empty_object();
    arrow_shaded_objects[0]->load_model_from_json(ARROW_PATH_1, texturePointers);
    arrow_shaded_objects[0]->Program = program;
    arrow_shaded_objects[0]->PickerProgram = picker_program;
    arrow_shaded_objects[0]->initModel();
    arrow_shaded_objects[0]->unique_color = INT_TO_UNIQUE_COLOR(1);
    printf("arrow loaded\n");

    arrow_shaded_objects[1] =  get_empty_object();
    arrow_shaded_objects[1]->load_model_from_json(ARROW_PATH_2, texturePointers);
    arrow_shaded_objects[1]->Program = program;
    arrow_shaded_objects[1]->PickerProgram = picker_program;
    arrow_shaded_objects[1]->initModel();
    arrow_shaded_objects[1]->unique_color = INT_TO_UNIQUE_COLOR(2);

    arrow_shaded_objects[2] =  get_empty_object();
    arrow_shaded_objects[2]->load_model_from_json(ARROW_PATH_3, texturePointers);
    arrow_shaded_objects[2]->Program = program;
    arrow_shaded_objects[2]->PickerProgram = picker_program;
    arrow_shaded_objects[2]->initModel();
    arrow_shaded_objects[2]->unique_color = INT_TO_UNIQUE_COLOR(3);
}

// Convert current objects to scene struct
struct scene convert_to_scene()
{
    struct scene scene;

    struct object_model *curr_object_model;
    sobj::shaded_object *curr_shaded_object;

    for (int i = 0; i < num_of_objects; i++)
    {
        curr_object_model = object_models[i];
        if (curr_object_model != nullptr)
        {
            if (curr_object_model->object_type != Empty || curr_object_model->object_type != PointLight)
            {
                curr_shaded_object = shaded_objects[curr_object_model->shaded_object_index];
                if (curr_shaded_object != nullptr)
                {
                    // add object model to scene models
                    scene.models.push_back(curr_shaded_object->inner_model);

                    // add object transform matrix to scene transforms
                    scene.transforms.push_back(curr_object_model->model_matrix);
                }
            }
            else if (curr_object_model->object_type == PointLight)
            {
                curr_shaded_object = shaded_objects[curr_object_model->shaded_object_index];
                if (curr_shaded_object != nullptr)
                {
                    // This is an object so change it to light and give that to the scene
                    // We only need the position information from this object
                    struct light light;
                    light.position = vec4(curr_object_model->object_coordinates.x, 
                        curr_object_model->object_coordinates.y, curr_object_model->object_coordinates.z, 1.0);
                    scene.scene_light = light;
                }
            }
        }
    }
    // Add current view_matrix to scene
    scene.view = view_matrix;

    return scene;
}

void export_current_scene_as_json(char *filename)
{
    serializable_scene s_scene = to_serializable_scene(convert_to_scene());

    std::ofstream file(filename);
    file << s_scene.zax_to_json();
    file.close();

    std::cout << "Scene saved in file " << filename << std::endl;
}

// create objects, vertex arrays and buffers
void init()
{
    // Load shaders and use the resulting shader program
    program = InitShader("vshader_phong.glsl", "fshader_phong.glsl");
    picker_program = InitShader("vshader_picker.glsl", "fshader_picker.glsl");
    glUseProgram(program);

    // Create cube, sphere, pointLight, arrow shaded_objects
    create_basic_objects(); //FIXME: LOAD OBJECT GIVES ERROR

    // Create projection matrix
    GLfloat aspect_ratio = (GLfloat) SCREEN_WIDTH / (GLfloat) SCREEN_HEIGHT;
    projection_matrix = Perspective(fovy, aspect_ratio, 0.1, 15.5);

    glEnable( GL_DEPTH_TEST );
    glClearColor( 1.0, 1.0, 1.0, 1.0 ); 

    // allocate memory for shaded_objects array containing pointers to all shaded_objects
    shaded_objects = (sobj::shaded_object **) malloc(shaded_objects_size * sizeof(sobj::shaded_object *));
    if (shaded_objects == nullptr) 
        perror("Error allocating memory to shaded_objects in init\n");
    else 
    {
        // add cube, sphere, pointLight and arrows
        shaded_objects[0] = cube_shaded_object;
        shaded_objects[1] = sphere_shaded_object;
        shaded_objects[2] = pointLight_shaded_object;
        shaded_objects[3] = *arrow_shaded_objects;
        for (int i = 4; i < shaded_objects_size; i++)
            shaded_objects[i] = nullptr;
    }

    // allocate memory for object_models array containing pointers to all object_models
    object_models = (struct object_model **) malloc(object_models_size * sizeof(struct object_model *));
    if (object_models == nullptr) 
        perror("Error allocating memory to object_models in init\n");
    else 
        for (int i = 0; i < object_models_size; i++)
            object_models[i] = nullptr;      

    // create first empty object, this will be selected when background is clicked 
    empty_object = add_object(Empty, "");
    // also create the point light object as a small yellow sphere
    // pointLight_object = add_object(PointLight, POINTLIGHT_PATH); //FIXME: LOAD OBJECT GIVES ERROR
    add_object(Cube, "");
}

void draw_objects(bool with_picking)
{
    // camera view matrix
    camera_at = camera_coordinates + camera_front;
    view_matrix = LookAt(camera_coordinates, camera_at, camera_up);

    struct object_model *curr_object_model;
    sobj::shaded_object *curr_shaded_object;
    mat4 transform;

    for (int i = 1; i < num_of_objects; i++)
    {
        curr_object_model = object_models[i];
        if (curr_object_model != nullptr)
        {
            curr_shaded_object = shaded_objects[curr_object_model->shaded_object_index];
            // color arrays and drawing according to selected, picking etc.
            glBindVertexArray(curr_shaded_object->VAO);
            glBindBuffer(GL_ARRAY_BUFFER, curr_shaded_object->VBO);
            
            // object model matrix
            transform = view_matrix * curr_object_model->model_matrix;

            // if not picking draw objects normally
            if (with_picking == false)
            {
                // if object not selected then just draw it solid with its own color

                //FIXME: check if works, what should light_position be?
                curr_shaded_object->display_real(transform, projection_matrix, light_position);

                // if object is selected then also draw its wireframe with black color
                if (curr_object_model->is_selected)
                {
                    //TODO: can we also draw object with GL_LINE here

                    // draw 3 arrows on the object
                    draw_object_arrows(curr_object_model, with_picking);
                }
            }
            // if with picking then just draw all objects with their unique picking colors
            else 
            {
                // Since shaded_objects have their own unique color but we reuse them at duplication,
                // we need to instead change it to the current object_model's unique_color at this proces
                curr_shaded_object->unique_color = curr_object_model->unique_id_color;
                curr_shaded_object->display_picker(transform, projection_matrix);

                // if object selected draw 3 arrows on the object with unique picking colors also
                if (curr_object_model->is_selected)
                    draw_object_arrows(curr_object_model, with_picking);
            }
        }
    }
}

// This is used to find the direction of the arrows on the object depending on the viewers perspective
// It will either return 1 or -1 to be multiplied with the x_mouse_difference
int find_arrow_direction(bool x_axis)
{
    // No need to do it for the y-axis arrow (green one) as it will always be the up vector for the viewer

    // For the x-axis arrow (red one) if camera_z_diff > 0 then x-axis arrow is in same general direction as x-axis
    if (x_axis)
    {
        if (camera_coordinates.z - camera_at.z >= 0)
            return 1;
        else
            return -1;
    }
    // For the z-axis arrow (blue one) if camera_x_diff > 0 then z-axis arrow is in same general direction as z-axis
    else
    {
        if (camera_coordinates.x - camera_at.x >= 0)
            return -1;
        else
            return 1;
    }
}

void translate_object_using_arrows(double x_pos_diff, double y_pos_diff)
{
    int multiplier;
    switch (object_arrow_selected_axis)
    {
    case 1: // x-axis
        multiplier = find_arrow_direction(true);
        object_models[selected_model_index]->Translation[Xaxis] += object_speed * multiplier * x_pos_diff;
        break;
    
    case 2: // y-axis
        multiplier = -1;
        object_models[selected_model_index]->Translation[Yaxis] += object_speed * multiplier * y_pos_diff;
        break;

    case 3: // z-axis
        multiplier = find_arrow_direction(false);
        object_models[selected_model_index]->Translation[Zaxis] += object_speed * multiplier * x_pos_diff;
        break;
    }
}

void rotate_object_using_arrows(double x_pos_diff, double y_pos_diff)
{
    int multiplier;
    switch (object_arrow_selected_axis)
    {
    case 1: // x-axis (red arrow)
        multiplier = find_arrow_direction(true);
        object_models[selected_model_index]->Theta[Xaxis] = multiplier * y_pos_diff;  
        break;
    
    case 2: // y-axis (green arrow)
        multiplier = -1 * find_arrow_direction(false);
        object_models[selected_model_index]->Theta[Yaxis] = multiplier * x_pos_diff;  
        break;

    case 3: // z-axis (blue arrow)
        multiplier = -1 * find_arrow_direction(true);
        object_models[selected_model_index]->Theta[Zaxis] = multiplier * x_pos_diff;  
        break;
    }
}

void scale_object_using_arrows(double y_pos_diff)
{
    if (object_arrow_selected_axis != 2) //if not y-axis
        return;

    object_models[selected_model_index]->Scaling[Xaxis] += 0.01 * -y_pos_diff;
    object_models[selected_model_index]->Scaling[Yaxis] += 0.01 * -y_pos_diff;
    object_models[selected_model_index]->Scaling[Zaxis] += 0.01 * -y_pos_diff;
}

void transform_object_with_arrows(double x_diff, double y_diff)
{
    switch (selected_action)
    {
    case NoAction:
        break;
    
    case TranslateObject:
        translate_object_using_arrows(x_diff, y_diff);
        break;

    case ScaleObject:
        scale_object_using_arrows(y_diff);
        break;
    
    case RotateObject:
        rotate_object_using_arrows(x_diff, y_diff);
        break;
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

    object_models[selected_model_index]->Theta[Xaxis] = 0.0;
    object_models[selected_model_index]->Theta[Yaxis] = 0.0;
    object_models[selected_model_index]->Theta[Zaxis] = 0.0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch(key) 
    {
    case GLFW_KEY_ESCAPE: // quit
        exit(EXIT_SUCCESS);
        break;

    case GLFW_KEY_P: //alt+P (option+P on mac) for debugging
        if (action == GLFW_PRESS && mods == GLFW_MOD_ALT)
            print_all_objects();
        break;

    // WASD and Mouse for camera movement (while right mouse button is being pressed)
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
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (action == GLFW_PRESS && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        left_click_holding = true;

        // For picking
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        draw_objects(true);
        glFlush();
        
        // get mouse position
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        int fb_width, fb_height;
        glfwGetFramebufferSize(window, &fb_width, &fb_height);
        int win_width, win_height;
        glfwGetWindowSize(window, &win_width, &win_height);
        
        xpos*=(fb_width/win_width);
        ypos*=(fb_height/win_height);
        ypos = fb_height - ypos;
                
        // get pixel color at mouse position
        unsigned char pixel[4];
        glReadPixels(xpos, ypos, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
        vec4 color = vec4(pixel[0] / 255.0, pixel[1] / 255.0, pixel[2] / 255.0, 1.0);
        int index = UNIQUE_COLOR_TO_INT(color) - 4; // 0: Empty, 1-3: reserved for arrows so -4

        object_arrow_selected = false;

        // if an object is selected change selected_model_index
        if (0 <= index-1 && index-1 < num_of_objects)
        {
            selected_action = NoAction;
            object_models[selected_model_index]->is_selected = false;
            selected_model_index = index-1;
            object_models[selected_model_index]->is_selected = true;
            std::cout << "Selected model index: " << selected_model_index << " model is: " << object_models[selected_model_index] << "\n";
        }
        // if object arrows selected
        else if (0 <= index+4 && index+4 <= 3) // 0: Empty, 1-3: reserved for arrows so +4
        {
            object_arrow_selected = true;
            object_arrow_selected_axis = index+4; //x=1, y=2, z=3
            std::cout << "Selected arrow index: " << object_arrow_selected << "\n";
        }
        
        if (display_picking)
            glfwSwapBuffers(window);
    }
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_LEFT)
    {
        left_click_holding = false;
    }
    if ((action == GLFW_PRESS || action == GLFW_REPEAT) && button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        right_click_holding = true;
        right_click_released_once = false;
    }
    else if (action == GLFW_RELEASE && button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        right_click_holding = false;
        object_arrow_selected = false;
        if (!right_click_released_once)
        {
            // remember last view rotations
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
        // remember last view rotations
        yaw = last_yaw + x_pos_diff;
        pitch = last_pitch - y_pos_diff;
    }
    else
    {
        yaw += x_pos_diff;
        pitch -= y_pos_diff;
    }

    // so that camera does not turn upside down
    if (pitch > 89.9)
        pitch = 89.9;
    if (pitch < -89.9)
        pitch = -89.9;
  
    // only change view if right_click_holding
    if (right_click_holding)
    {
        vec3 front;
        front.x = cos(yaw * (M_PI / 180.0)) * cos(pitch * (M_PI / 180.0));
        front.y = sin(pitch * (M_PI / 180.0));
        front.z = sin(yaw * (M_PI / 180.0)) * cos(pitch * (M_PI / 180.0));
        camera_front = normalize(front);       
    }

    if (object_arrow_selected && left_click_holding)
    {
        transform_object_with_arrows(x_pos_diff, y_pos_diff);
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

    projection_matrix = Perspective(fovy_changed, aspect_ratio, .1, 15.5);
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

#if defined(__linux__) || defined(_WIN32)
    glewExperimental = GL_TRUE;
    glewInit();
#elif defined(__APPLE__)
    // Code for macOS
#endif

    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    // Setup Dear ImGui style
    ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    
    init();

    ImGui_ImplOpenGL3_Init();

    double frameRate = 60.0, currentTime, previousTime = 0.0;
    while (!glfwWindowShouldClose(window))
    {
        update();
        display();

        // ImGUI start

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int width, height;

        glfwGetWindowSize(window, &width, &height);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(300, height));
        ImGui::Begin("Button Window", nullptr,
                    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoSavedSettings);

        ImGui::BeginChild("Load Model Box", ImVec2(-1, 100), true,
                        ImGuiWindowFlags_HorizontalScrollbar);
        ImGui::TextWrapped("Model filename:");
        static char filename[256] = "";
        ImGui::InputText("File path", filename, sizeof(filename));
        
        if (ImGui::Button("Load Model"))
        {
            add_object(Imported, filename);
        }
        ImGui::EndChild();

        ImGui::BeginChild("Add Cube and Sphere Box", ImVec2(-1, 60), true,
                        ImGuiWindowFlags_HorizontalScrollbar);
        if (ImGui::Button("Add Cube"))
        {
            add_object(Cube, "");
        }
        if (ImGui::Button("Add Sphere"))
        {
            add_object(Sphere, "");
        }
        ImGui::EndChild();

        ImGui::BeginChild("Transform Object Box", ImVec2(-1, 200), true,
                        ImGuiWindowFlags_HorizontalScrollbar);

        if (ImGui::Button("Edit object in Shader Editor"))
        {
            // TODO: this requires shader_editor shaders to have "../shader_editor/vhsader.glsl" etc.
            system("../shader_editor/shader_editor");
            // TODO: Also how do we give model information to shader_editor??
        }
        if (ImGui::Button("Duplicate Object"))
        {
            duplicate_selected_object();
        }
        if (ImGui::Button("Delete Object"))
        {
            delete_selected_object();
        }

        ImGui::TextWrapped("\nTransform object:");
        if (ImGui::Button("Move Object"))
        {
            selected_action = TranslateObject;
        }
        if (ImGui::Button("Scale Object"))
        {
            selected_action = ScaleObject;
        }
        if (ImGui::Button("Rotate Object"))
        {
            selected_action = RotateObject;
        }
        ImGui::EndChild();

        ImGui::BeginChild("Save Scene Box", ImVec2(-1, 100), true,
                        ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::TextWrapped("Save Scene Path:");
        static char filename2[256] = "";
        ImGui::InputText("File path", filename2, sizeof(filename2));
        if (ImGui::Button("Save Scene")) 
        {
            export_current_scene_as_json(filename2); 
        }

        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // ImGUI end

        if (!display_picking)
            glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
