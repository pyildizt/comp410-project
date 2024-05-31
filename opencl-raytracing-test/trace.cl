

float area(float2 c1, float2 c2, float2 c3);
bool inside(float2 uvhit, float2 uv1, float2 uv2, float2 uv3);


float area(float2 c1, float2 c2, float2 c3)
{

    return 0.5f * fabs(
                      (c1.x * (c2.y - c3.y)) +
                      (c2.x * (c3.y - c1.y)) +
                      (c3.x * (c1.y - c2.y)));
}

bool inside(float2 uvhit, float2 uv1, float2 uv2, float2 uv3)
{
    return fabs(area(uv1, uv2, uv3) -
                (area(uvhit, uv1, uv2) + area(uvhit, uv1, uv3) + area(uvhit, uv2, uv3))) < 0.001;
}




float my_ceil(double x) {
    int i = (int)x;
    return (x > (double)i) ? (double)(i + 1) : (double)i;
}
float my_floor(double x) {
    int i = (int)x;
    return (x < (double)i) ? (double)(i - 1) : (double)i;
}

double random_double_between_0_1_giver(uint* seed) {
    const uint a = 1664525;  
    const uint c = 1013904223;  
    const uint m = 0xFFFFFFFF;  




    // Initialize the PRNG with the given seed
    uint state = *seed;

    state = (a * state + c) & m;
    *seed=state;

    return (double)(state) / (double)(m);;

    
}


typedef struct {
    double x;
    double y;
} Vector2D;

typedef struct {
    double x;
    double y;
    double Z;
} Vector3D;

typedef struct {
    Vector2D* array;
    int number_of_elements;
} Vector2D_array;

typedef struct {
    Vector3D* array;
    int number_of_elements;
} Vector3D_array;




Vector2D randomPointInRange(Vector2D vector, double r,uint* seed) {
    double M_PI=3.14159265358979323846264338327;
    double angle = 2 * M_PI * random_double_between_0_1_giver(seed);


    double distance = r + (random_double_between_0_1_giver(seed) * r);

    Vector2D point;
    point.x = vector.x + distance * cos((float)angle);
    point.y = vector.y + distance * sin((float)angle);

    return point;
}

void array_element_remover(Vector2D* array, int index, int array_total_number_of_elemets) {
  for (int a=0; a<array_total_number_of_elemets-index-1; a++) {
    array[index+a]=array[index+a+1];

  }

}
void populate_the_random_k_points(Vector2D given, double min, Vector2D* to_be_populated, uint* seed) {
  for (int a=0; a<30; a++) {
    to_be_populated[a]=randomPointInRange(given, min, seed);

  }
}

double distance_between(Vector2D v1, Vector2D v2) {
  return (double)sqrt((v1.x-v2.x)*(v1.x-v2.x)+(v1.y-v2.y)*(v1.y-v2.y));

}


int randomIntInRange(int n, int m) {
    return n + rand() % (m - n);
}



int fitness_checker(Vector2D current_point, Vector2D* location_array, int rows, int cols, int background_array[rows][cols], double length, double width, int m, int n, double r, double tek_kare_uzunluk) {

  if (current_point.x>0 && current_point.x<length && current_point.y>0 && current_point.y<width  ) {
      int x0=my_floor(current_point.x/tek_kare_uzunluk);

      int y0=my_floor(current_point.y/tek_kare_uzunluk);



      int i0 = max(y0 - 1, 0);
      int i1 = min(y0 + 1, m - 1);

      int j0 = max(x0 - 1, 0);
      int j1 = min(x0 + 1, n - 1);




      for (int i = i0; i <= i1; i++) {
        for (int j = j0; j <= j1; j++) {
            if (background_array[i][j]>-1 || (i==y0 && j==x0)) {



                int our_index=background_array[i][j];



                double dista=(sqrt(pow(location_array[our_index].x - current_point.x, 2) + pow(location_array[our_index].y - current_point.y, 2)));

                if (x0==6 && y0==1) {


                }




                if (dista < r)
                    return 0;
            }
        }
    }

  }
  else {
    return 0;
  }

  return 1;


}



Vector2D_array random_points_giver(double width, double length, double minimum_distance, uint* seed) {

  

  double tek_kare_uzunluk=minimum_distance/sqrt((float)2);
  int n=my_ceil(length/tek_kare_uzunluk);
  int m=my_ceil(width/tek_kare_uzunluk);

  //int background_array[m][n];
  int background_array[1000][1000];

  for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            background_array[i][j] = -1;
        }
  }
  Vector2D initial_random_vector;
  initial_random_vector.x=random_double_between_0_1_giver(seed)*length;
  initial_random_vector.y=random_double_between_0_1_giver(seed)*width;






  int x0=my_floor(initial_random_vector.x/tek_kare_uzunluk);

  int y0=my_floor(initial_random_vector.y/tek_kare_uzunluk);





  background_array[x0][y0]=0;






  int active_array_current_index=0;
  int location_array_index=0;

  //Vector2D active_array[n*m];
  //Vector2D location_array[n*m];

  Vector2D active_array[1000*1000];
  Vector2D location_array[1000*1000];



  active_array[active_array_current_index]=initial_random_vector;
  location_array[location_array_index]=initial_random_vector;
  active_array_current_index++;
  location_array_index+=1;






  while (active_array_current_index>0) {




    Vector2D random_k_points[30];
    int j=randomIntInRange(0,active_array_current_index);




    populate_the_random_k_points(active_array[j], minimum_distance,random_k_points, seed);
    for (int i=0; i<30; i++) {

      Vector2D current_point=random_k_points[i];


      int j=fitness_checker(current_point, location_array, m,n ,background_array, length, width, m,n, minimum_distance, tek_kare_uzunluk);


      if (j==1) {
        active_array[active_array_current_index]=current_point;
        active_array_current_index+=1;
          int x1=my_floor(current_point.x/tek_kare_uzunluk);

          int y1=my_floor(current_point.y/tek_kare_uzunluk);



        background_array[y1][x1]=location_array_index;



          location_array[location_array_index]=current_point;
        location_array_index+=1;

        break;
      }
      if (i==29) {
        array_element_remover(active_array, j, active_array_current_index);
        active_array_current_index-=1;


      }





    }





  }
  Vector2D_array to_be_returned;
  to_be_returned.array=location_array;
  to_be_returned.number_of_elements=location_array_index;
  return to_be_returned;

}

Vector3D_array convert_to_3d_plane(Vector2D_array to_be_returned, double camera_plane_x_coordinate,double camera_Plane_y_coordinate, double camera_Plane_z_coordinate, double camera_plane_x_angle,double camera_Plane_y_angle, double camera_Plane_z_angle) {

}


__kernel void trace(double camera_plane_x_coordinate,double camera_Plane_y_coordinate, double camera_Plane_z_coordinate, double camera_plane_x_angle,double camera_Plane_y_angle, double camera_Plane_z_angle, 
int  num_of_rays_per_pixel, int which_pixel_x_coord, int which_pixel_y_coord, int random_number_generator_seed, double width, double length, double minimum_distance)

{
    uint our_random_number=(uint)random_number_generator_seed;

    Vector2D_array to_be_returned=random_points_giver(width, length, minimum_distance ,&our_random_number);
    printf("The integer final final is: %d\n", to_be_returned.number_of_elements);
    Vector3D_array final_plane=convert_to_3d_plane(to_be_returned,camera_plane_x_coordinate,camera_Plane_y_coordinate,camera_Plane_z_coordinate,camera_plane_x_angle,camera_Plane_y_angle,camera_Plane_z_angle);
    

    


}
