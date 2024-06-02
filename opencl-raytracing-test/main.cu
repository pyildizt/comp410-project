/*

Today's goal is to visualize a simple raytracer::triangle with no lighting

*/

//---------------------------------------------------------

#include <cstdlib>
#define CL_RUN(fnc, ...)                                                       \
  fnc(__VA_ARGS__, &err);                                                      \
  if (err < 0) {                                                               \
    printf("CL ERROR AT LINE %d\n", __LINE__);                                 \
    perror("err");                                                             \
    exit(1);                                                                   \
  }

#define CL_RUN3(exp)                                                           \
  exp;                                                                         \
  if (err < 0) {                                                               \
    printf("CL ERROR AT LINE %d, err: %d\n", __LINE__, err);                   \
    perror("err");                                                             \
    exit(1);                                                                   \
  }

#define CL_RUN2(fnc)                                                           \
  do {                                                                         \
    cl_int err = fnc;                                                          \
    if (err < 0) {                                                             \
      printf("CL ERROR AT LINE %d %d\n", __LINE__, err);                       \
      perror("err");                                                           \
      exit(1);                                                                 \
    }                                                                          \
  } while (0);
//---------------------------------------------------------

// C standard includes
#include <stdio.h>

// OpenCL includes
#include <cuda.h>
#include <cuda_runtime.h>
#include <math.h>
#include <png.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define NUM_PIXELS_X 32
#define NUM_PIXELS_Y 32
#define OUT_BUFFER_LEN (NUM_PIXELS_X * NUM_PIXELS_Y)

typedef unsigned char uchar;

//-----------------------PNG-----------------------------

void save_png(const char *filename, unsigned char *data, int width,
              int height) {
  FILE *fp = fopen(filename, "wb");
  if (!fp) {
    fprintf(stderr, "Error: Unable to open file %s for writing\n", filename);
    return;
  }

  png_structp png_ptr =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fprintf(stderr, "Error: png_create_write_struct failed\n");
    fclose(fp);
    return;
  }

  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    fprintf(stderr, "Error: png_create_info_struct failed\n");
    png_destroy_write_struct(&png_ptr, NULL);
    fclose(fp);
    return;
  }

  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGBA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (int y = 0; y < height; y++) {
    row_pointers[y] =
        (png_bytep)&data[y * width * 4]; // Assuming 4 channels (raytracer::RGB)
  }

  png_init_io(png_ptr, fp);
  png_set_rows(png_ptr, info_ptr, row_pointers);
  png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

  // Cleanup
  png_destroy_write_struct(&png_ptr, &info_ptr);
  fclose(fp);
  free(row_pointers);
}

float my_ceil(double x);
float my_ceil(double x) {
  int i = (int)x;
  return (x > (double)i) ? (double)(i + 1) : (double)i;
}
float my_floor(double x);
float my_floor(double x) {
  int i = (int)x;
  return (x < (double)i) ? (double)(i - 1) : (double)i;
}
double random_double_between_0_1_giver(uint *seed);
double random_double_between_0_1_giver(uint *seed) {
  const uint a = 1664525;
  const uint c = 1013904223;
  const uint m = 0xFFFFFFFF;

  // Initialize the PRNG with the given seed
  uint state = *seed;

  state = (a * state + c) & m;
  *seed = state;

  return (double)(state) / (double)(m);
  ;
}
namespace raytracer {
typedef struct {
  uchar R;
  uchar G;
  uchar B;
} RGB;

typedef struct {
  double x;
  double y;
} Vector2D;

typedef struct {
  double x;
  double y;
  double z;
} Vector3D;

typedef struct {
  Vector2D *array;
  int number_of_elements;
} Vector2D_array;

typedef struct {
  Vector3D array[50 * 50];
  int number_of_elements;
} Vector3D_array;

#define SPECTRUM_SIZE (((700 - 380) / 5 + 1))

typedef struct {
  double spectrum[(int)SPECTRUM_SIZE];
  int number_of_elements;

} spectrum_of_light;

typedef struct {
  raytracer::spectrum_of_light surface_spectrum;
  double diffusion_coefficient;

} surface;

typedef struct {
  raytracer::surface surface_array[50]; // we have at maximum 50 different
                                        // raytracer::surface propoerties

} surface_array;

typedef struct {
  Vector3D coordinates1;
  Vector3D coordinates2;
  Vector3D coordinates3;
  int surface_number;

} triangle;

typedef struct {
  triangle all_triangles[2000]; // we have at maximum 2000 distinct
                                // raytracer::triangles
  int total_number_of_triangles;

} triangle_array;

typedef struct {
  Vector3D ray_current_coordinates;
  Vector3D ray_direction;
  raytracer::spectrum_of_light ray_spectrum;
  int recursion_index;

} ray;

} // namespace raytracer

typedef raytracer::Vector3D Vector3D;
typedef raytracer::ray ray;

raytracer::Vector3D normalize(raytracer::Vector3D v) {
  double length = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return (raytracer::Vector3D){v.x / length, v.y / length, v.z / length};
}

void add_second_spectrum_to_first_one(raytracer::spectrum_of_light *s1,
                                      raytracer::spectrum_of_light s2);
void add_second_spectrum_to_first_one(raytracer::spectrum_of_light *s1,
                                      raytracer::spectrum_of_light s2) {
  for (int i = 0; i < s1->number_of_elements; i++) {
    s1->spectrum[i] += s2.spectrum[i];
  }
}

raytracer::Vector3D give_the_normal_of_a_triangle(
    raytracer::triangle
        input) { // Source:
                 // https://stackoverflow.com/questions/19350792/calculate-normal-of-a-single-raytracer::triangle-in-3d-space

  double Nx = (input.coordinates2.y - input.coordinates1.y) *
                  (input.coordinates3.z - input.coordinates1.z) -
              (input.coordinates2.z - input.coordinates1.z) *
                  (input.coordinates3.y - input.coordinates1.y);
  double Ny = (input.coordinates2.z - input.coordinates1.z) *
                  (input.coordinates3.x - input.coordinates1.x) -
              (input.coordinates2.x - input.coordinates1.x) *
                  (input.coordinates3.z - input.coordinates1.z);
  double Nz = (input.coordinates2.x - input.coordinates1.x) *
                  (input.coordinates3.y - input.coordinates1.y) -
              (input.coordinates2.y - input.coordinates1.y) *
                  (input.coordinates3.x - input.coordinates1.x);

  raytracer::Vector3D to_be_returned;
  to_be_returned.x = Nx;
  to_be_returned.y = Ny;
  to_be_returned.z = Nz;

  return to_be_returned;
}

raytracer::Vector3D copy_given3d(raytracer::Vector3D a) {
  raytracer::Vector3D h;
  h.x = a.x;
  h.y = a.y;
  h.z = a.z;
  return h;
}

raytracer::Vector3D vec3_sub(raytracer::Vector3D a, raytracer::Vector3D b);
raytracer::Vector3D vec3_sub(raytracer::Vector3D a, raytracer::Vector3D b) {
  raytracer::Vector3D result = {a.x - b.x, a.y - b.y, a.z - b.z};
  return result;
}
raytracer::Vector3D cross_p(raytracer::Vector3D a, raytracer::Vector3D b) {
  raytracer::Vector3D result = {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                                a.x * b.y - a.y * b.x};
  return result;
}

double dot_product(raytracer::Vector3D a, raytracer::Vector3D b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
bool does_triangle_and_ray_intersect_correctly(
    raytracer::ray R,
    raytracer::triangle
        tri) { // source is //
               // https://stackoverflow.com/questions/42740765/intersection-between-line-and-raytracer::triangle-in-3d
  double t;
  double u;
  double v;
  raytracer::Vector3D N;
  raytracer::Vector3D E1 = vec3_sub(tri.coordinates2, tri.coordinates1);
  raytracer::Vector3D E2 = vec3_sub(tri.coordinates3, tri.coordinates1);

  // printf(" Nx: %f \n", R.ray_current_coordinates.x);
  // printf(" Ny %f \n", R.ray_current_coordinates.y);
  // printf(" Nz %f \n", R.ray_current_coordinates.z);

  N = cross_p(E1, E2);
  double det = -dot_product(R.ray_direction, N);
  if (det < 1e-6)
    return false;
  double invdet = 1.0 / det;
  raytracer::Vector3D AO =
      vec3_sub(R.ray_current_coordinates, tri.coordinates1);
  raytracer::Vector3D DAO = cross_p(AO, R.ray_direction);
  u = dot_product(E2, DAO) * invdet;
  v = -dot_product(E1, DAO) * invdet;
  t = dot_product(AO, N) * invdet;
  // printf("Boolean value: %d\n", (int)(t >= 0 && u >= 0 && v >= 0 && (u + v)
  // <= 1.0));
  return (t >= 0 && u >= 0 && v >= 0 && (u + v) <= 1.0);
}
raytracer::Vector3D intersectLinePlane(raytracer::Vector3D P0,
                                       raytracer::Vector3D v, double A,
                                       double B, double C, double D) {
  raytracer::Vector3D intersection;
  double t =
      (-D - A * P0.x - B * P0.y - C * P0.z) / (A * v.x + B * v.y + C * v.z);
  intersection.x = P0.x + t * v.x;
  intersection.y = P0.y + t * v.y;
  intersection.z = P0.z + t * v.z;
  return intersection;
}
double distanceBetweenPoints(raytracer::Vector3D p1, raytracer::Vector3D p2);
double distanceBetweenPoints(raytracer::Vector3D p1, raytracer::Vector3D p2) {
  double dx = p2.x - p1.x;
  double dy = p2.y - p1.y;
  double dz = p2.z - p1.z;
  return sqrt(dx * dx + dy * dy + dz * dz);
}

double magnitude(raytracer::Vector3D v);
double magnitude(raytracer::Vector3D v) {
  return sqrt((float)(v.x * v.x + v.y * v.y + v.z * v.z));
}

double compute_distance_with_a_triangle_and_ray(raytracer::ray R,
                                                raytracer::triangle tri) {
  raytracer::Vector3D normal = give_the_normal_of_a_triangle(tri);
  double D = -(normal.x * tri.coordinates1.x + normal.y * tri.coordinates1.y +
               normal.z * tri.coordinates1.z);
  raytracer::Vector3D intersection =
      intersectLinePlane(R.ray_current_coordinates, R.ray_direction, normal.x,
                         normal.y, normal.z, D);
  double distance =
      distanceBetweenPoints(intersection, R.ray_current_coordinates);
  double mag = magnitude(R.ray_direction);
  return distance * mag;
}
raytracer::Vector3D vectorFromPoints(raytracer::Vector3D A,
                                     raytracer::Vector3D B) {
  raytracer::Vector3D result;
  result.x = B.x - A.x;
  result.y = B.y - A.y;
  result.z = B.z - A.z;
  return result;
}

// Function to calculate the minimum distance between a point and a line in 3D
double minimumDistance(raytracer::Vector3D A, raytracer::Vector3D d,
                       raytracer::Vector3D P) {

  raytracer::Vector3D lin = vectorFromPoints(A, P);

  raytracer::Vector3D lin_cross_d = cross_p(lin, d);

  double magnitude_lin_cross_d = magnitude(lin_cross_d);
  double magnitude_d = magnitude(d);

  double distance = magnitude_lin_cross_d / magnitude_d;

  double dot = dot_product(lin, d);
  if (dot < 0) {
    distance = -1.0;
  }

  return distance;
}

double
compute_distance_with_a_ray_and_a_point(raytracer::ray current_ray,
                                        raytracer::Vector3D light_position) {

  return minimumDistance(current_ray.ray_current_coordinates,
                         current_ray.ray_direction, light_position);
}
raytracer::Vector3D
find_intersection_of_a_triangle_with_a_ray(raytracer::ray R,
                                           raytracer::triangle tri);
raytracer::Vector3D find_intersection_of_a_triangle_with_a_ray(
    raytracer::ray R,
    raytracer::triangle tri) { // written with the help of ChatGPT
  double x0 = R.ray_current_coordinates.x, y0 = R.ray_current_coordinates.y,
         z0 = R.ray_current_coordinates.z;
  double dx = R.ray_direction.x, dy = R.ray_direction.y, dz = R.ray_direction.z;
  raytracer::Vector3D normal = give_the_normal_of_a_triangle(tri);
  double D1 = -(normal.x * tri.coordinates1.x + normal.y * tri.coordinates1.y +
                normal.z * tri.coordinates1.z);
  double A = normal.x, B = normal.y, C = normal.z, D = D1;

  double denominator = A * dx + B * dy + C * dz;

  double numerator = -(A * x0 + B * y0 + C * z0 + D);
  double t = numerator / denominator;

  raytracer::Vector3D intersection;

  intersection.x = x0 + t * dx;
  intersection.y = y0 + t * dy;
  intersection.z = z0 + t * dz;

  return intersection;
}
Vector3D give_specular_direction(ray current_ray, raytracer::triangle tri);
Vector3D give_specular_direction(ray current_ray, raytracer::triangle tri) {
  Vector3D planeNormal = give_the_normal_of_a_triangle(tri);
  Vector3D incidentRay = current_ray.ray_direction;

  float cosTheta_i = dot_product(incidentRay, planeNormal) /
                     (magnitude(incidentRay) * magnitude(planeNormal));

  // Calculate the reflected ray direction
  Vector3D reflectedRay = {incidentRay.x - 2 * cosTheta_i * planeNormal.x,
                           incidentRay.y - 2 * cosTheta_i * planeNormal.y,
                           incidentRay.z - 2 * cosTheta_i * planeNormal.z};

  // Normalize the reflected ray direction
  float mag = magnitude(reflectedRay);
  reflectedRay.x /= mag;
  reflectedRay.y /= mag;
  reflectedRay.z /= mag;

  return reflectedRay;
}

Vector3D random_direction(Vector3D normal1, uint *seed) {
  // Ensure the normal vector is normalized
  Vector3D normal = normalize(normal1);

  // Generate a random point on the unit hemisphere using spherical coordinates
  double phi = 2.0 * M_PI * random_double_between_0_1_giver(seed);
  double cosTheta = random_double_between_0_1_giver(seed);
  double sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  Vector3D randomDirection = {sinTheta * cos(phi), sinTheta * sin(phi),
                              cosTheta};

  // Create a coordinate system (u, v, w) with w aligned with the normal
  Vector3D w = normal;
  Vector3D u = fabs(w.x) > 0.0001 ? (Vector3D){0, 1, 0} : (Vector3D){1, 0, 0};
  u = normalize(cross_p(u, w));
  Vector3D v = cross_p(w, u);

  // Transform the random direction to align with the normal
  Vector3D result = {randomDirection.x * u.x + randomDirection.y * v.x +
                         randomDirection.z * w.x,
                     randomDirection.x * u.y + randomDirection.y * v.y +
                         randomDirection.z * w.y,
                     randomDirection.x * u.z + randomDirection.y * v.z +
                         randomDirection.z * w.z};

  return result;
}

Vector3D give_diffusive_direction(ray current_ray, raytracer::triangle tri,
                                  uint *seed);
Vector3D give_diffusive_direction(ray current_ray, raytracer::triangle tri,
                                  uint *seed) {
  Vector3D planeNormal = give_the_normal_of_a_triangle(tri);

  return random_direction(planeNormal, seed);
}

raytracer::Vector2D randomPointInRange(raytracer::Vector2D vector, double r,
                                       uint *seed);
raytracer::Vector2D randomPointInRange(raytracer::Vector2D vector, double r,
                                       uint *seed) {

  double angle = 2 * M_PI * random_double_between_0_1_giver(seed);

  double distance = r + (random_double_between_0_1_giver(seed) * r);

  raytracer::Vector2D point;
  point.x = vector.x + distance * cos((float)angle);
  point.y = vector.y + distance * sin((float)angle);

  return point;
}
void array_element_remover(raytracer::Vector2D *array, int index,
                           int array_total_number_of_elemets);
void array_element_remover(raytracer::Vector2D *array, int index,
                           int array_total_number_of_elemets) {
  for (int a = 0; a < array_total_number_of_elemets - index - 1; a++) {
    array[index + a] = array[index + a + 1];
  }
}
void populate_the_random_k_points(raytracer::Vector2D given, double min,
                                  raytracer::Vector2D *to_be_populated,
                                  uint *seed);
void populate_the_random_k_points(raytracer::Vector2D given, double min,
                                  raytracer::Vector2D *to_be_populated,
                                  uint *seed) {
  for (int a = 0; a < 30; a++) {
    to_be_populated[a] = randomPointInRange(given, min, seed);
  }
}
double distance_between(raytracer::Vector2D v1, raytracer::Vector2D v2);
double distance_between(raytracer::Vector2D v1, raytracer::Vector2D v2) {
  return (double)sqrt(
      (float)((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y)));
}

int randomIntInRange(int n, int m, uint *seed);
int randomIntInRange(int n, int m, uint *seed) {
  return n + (int)(random_double_between_0_1_giver(seed) * (double)(m - n - 1));
}

int fitness_checker(raytracer::Vector2D current_point,
                    raytracer::Vector2D *location_array, int rows, int cols,
                    int background_array[50][50], double length, double width,
                    int m, int n, double r, double tek_kare_uzunluk);
int fitness_checker(raytracer::Vector2D current_point,
                    raytracer::Vector2D *location_array, int rows, int cols,
                    int background_array[50][50], double length, double width,
                    int m, int n, double r, double tek_kare_uzunluk) {
  if (current_point.x > 0 && current_point.x < length && current_point.y > 0 &&
      current_point.y < width) {
    int x0 = my_floor(current_point.x / tek_kare_uzunluk);

    int y0 = my_floor(current_point.y / tek_kare_uzunluk);

    int i0 = fmax(y0 - 1, 0);
    int i1 = fmin(y0 + 1, m - 1);

    int j0 = fmax(x0 - 1, 0);
    int j1 = fmin(x0 + 1, n - 1);

    for (int i = i0; i <= i1; i++) {
      for (int j = j0; j <= j1; j++) {
        if (background_array[i][j] > -1 || (i == y0 && j == x0)) {

          int our_index = background_array[i][j];

          double dista = (sqrt(
              pow((float)(location_array[our_index].x - current_point.x), 2) +
              pow((float)(location_array[our_index].y - current_point.y), 2)));

          if (x0 == 6 && y0 == 1) {
          }

          if (dista < r)
            return 0;
        }
      }
    }

  } else {
    return 0;
  }

  return 1;
}

raytracer::Vector2D_array random_points_giver(double width, double length,
                                              double minimum_distance,
                                              uint *seed);
raytracer::Vector2D_array random_points_giver(double width, double length,
                                              double minimum_distance,
                                              uint *seed) {

  double tek_kare_uzunluk = minimum_distance / sqrt((float)2);
  int n = my_ceil(length / tek_kare_uzunluk);
  int m = my_ceil(width / tek_kare_uzunluk);

  // int background_array[m][n];
  int background_array[50][50];

  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      background_array[i][j] = -1;
    }
  }
  raytracer::Vector2D initial_random_vector;
  initial_random_vector.x = random_double_between_0_1_giver(seed) * length;
  initial_random_vector.y = random_double_between_0_1_giver(seed) * width;

  int x0 = my_floor(initial_random_vector.x / tek_kare_uzunluk);

  int y0 = my_floor(initial_random_vector.y / tek_kare_uzunluk);

  background_array[x0][y0] = 0;

  int active_array_current_index = 0;
  int location_array_index = 0;

  // raytracer::Vector2D active_array[n*m];
  // raytracer::Vector2D location_array[n*m];

  raytracer::Vector2D active_array[50 * 50];
  raytracer::Vector2D location_array[50 * 50];

  active_array[active_array_current_index] = initial_random_vector;
  location_array[location_array_index] = initial_random_vector;
  active_array_current_index++;
  location_array_index += 1;

  while (active_array_current_index > 0) {

    raytracer::Vector2D random_k_points[30];
    int j = randomIntInRange(0, active_array_current_index, seed);

    populate_the_random_k_points(active_array[j], minimum_distance,
                                 random_k_points, seed);
    for (int i = 0; i < 30; i++) {

      raytracer::Vector2D current_point = random_k_points[i];

      int j = fitness_checker(current_point, location_array, m, n,
                              background_array, length, width, m, n,
                              minimum_distance, tek_kare_uzunluk);

      if (j == 1) {
        active_array[active_array_current_index] = current_point;
        active_array_current_index += 1;
        int x1 = my_floor(current_point.x / tek_kare_uzunluk);

        int y1 = my_floor(current_point.y / tek_kare_uzunluk);

        background_array[y1][x1] = location_array_index;

        location_array[location_array_index] = current_point;

        location_array_index += 1;

        break;
      }
      if (i == 29) {
        array_element_remover(active_array, j, active_array_current_index);
        active_array_current_index -= 1;
      }
    }
  }
  raytracer::Vector2D_array to_be_returned;
  to_be_returned.array = location_array;
  to_be_returned.number_of_elements = location_array_index;
  return to_be_returned;
}
raytracer::Vector3D_array
place_it_to_new_location(raytracer::Vector2D_array *input,
                         double new_x_coorinate, double new_y_coordiante);
raytracer::Vector3D_array
place_it_to_new_location(raytracer::Vector2D_array *input,
                         double new_x_coorinate, double new_y_coordiante) {
  raytracer::Vector3D_array to_be_returned;
  memset(to_be_returned.array, 0, sizeof(Vector3D));
  to_be_returned.number_of_elements = input->number_of_elements;
  for (int i = 0; i < input->number_of_elements; i++) {
    Vector3D point;
    point.x = input->array[i].x + new_x_coorinate;
    point.y = input->array[i].y + new_y_coordiante;
    point.z = (double)1;
    to_be_returned.array[i] = point;
  }
  return to_be_returned;
}

// below are the functions to convert a spectrum a an raytracer::RGB value

typedef struct {
  double wavelength;
  double x_bar;
  double y_bar;
  double z_bar;
} CIE1931;
// Placeholder function for CIE 1931 color matching functions
void wavelength_to_xyz(double wavelength, double *x_bar, double *y_bar,
                       double *z_bar);
void wavelength_to_xyz(
    double wavelength, double *x_bar, double *y_bar,
    double *z_bar) { // this function and data are obtained from chatgpt
  // Replace this function with actual CIE 1931 color matching functions
  // Example: return x_bar, y_bar, z_bar for the given wavelength
  // This example uses placeholder values and should be replaced with real data.
  CIE1931 cie_data[] = {
      {380, 0.001368, 0.000039, 0.006450}, {385, 0.002236, 0.000064, 0.010550},
      {390, 0.004243, 0.000120, 0.020050}, {395, 0.007650, 0.000217, 0.036210},
      {400, 0.014310, 0.000396, 0.067850}, {405, 0.023190, 0.000640, 0.110200},
      {410, 0.043510, 0.001210, 0.207400}, {415, 0.077630, 0.002180, 0.371300},
      {420, 0.134380, 0.004000, 0.645600}, {425, 0.214770, 0.007300, 1.039050},
      {430, 0.283900, 0.011600, 1.385600}, {435, 0.328500, 0.016840, 1.622960},
      {440, 0.348280, 0.023000, 1.747060}, {445, 0.348060, 0.029800, 1.782600},
      {450, 0.336200, 0.038000, 1.772110}, {455, 0.318700, 0.048000, 1.744100},
      {460, 0.290800, 0.060000, 1.669200}, {465, 0.251100, 0.073900, 1.528100},
      {470, 0.195360, 0.090980, 1.287640}, {475, 0.142100, 0.112600, 1.041900},
      {480, 0.095640, 0.139020, 0.812950}, {485, 0.057950, 0.169300, 0.616200},
      {490, 0.032010, 0.208020, 0.465180}, {495, 0.014700, 0.258600, 0.353300},
      {500, 0.004900, 0.323000, 0.272000}, {505, 0.002400, 0.407300, 0.212300},
      {510, 0.009300, 0.503000, 0.158200}, {515, 0.029100, 0.608200, 0.111700},
      {520, 0.063270, 0.710000, 0.078250}, {525, 0.109600, 0.793200, 0.057250},
      {530, 0.165500, 0.862000, 0.042160}, {535, 0.225750, 0.914850, 0.029840},
      {540, 0.290400, 0.954000, 0.020300}, {545, 0.359700, 0.980300, 0.013400},
      {550, 0.433450, 0.994950, 0.008750}, {555, 0.512050, 1.000000, 0.005750},
      {560, 0.594500, 0.995000, 0.003900}, {565, 0.678400, 0.978600, 0.002750},
      {570, 0.762100, 0.952000, 0.002100}, {575, 0.842500, 0.915400, 0.001800},
      {580, 0.916300, 0.870000, 0.001650}, {585, 0.978600, 0.816300, 0.001400},
      {590, 1.026300, 0.757000, 0.001100}, {595, 1.056700, 0.694900, 0.001000},
      {600, 1.062200, 0.631000, 0.000800}, {605, 1.045600, 0.566800, 0.000600},
      {610, 1.002600, 0.503000, 0.000340}, {615, 0.938400, 0.441200, 0.000240},
      {620, 0.854450, 0.381000, 0.000190}, {625, 0.751400, 0.321000, 0.000100},
      {630, 0.642400, 0.265000, 0.000050}, {635, 0.541900, 0.217000, 0.000030},
      {640, 0.447900, 0.175000, 0.000020}, {645, 0.360800, 0.138200, 0.000010},
      {650, 0.283500, 0.107000, 0.000000}, {655, 0.218700, 0.081600, 0.000000},
      {660, 0.164900, 0.061000, 0.000000}, {665, 0.121200, 0.044580, 0.000000},
      {670, 0.087400, 0.032000, 0.000000}, {675, 0.063600, 0.023200, 0.000000},
      {680, 0.046770, 0.017000, 0.000000}, {685, 0.032900, 0.011920, 0.000000},
      {690, 0.022700, 0.008210, 0.000000}, {695, 0.015840, 0.005723, 0.000000},
      {700, 0.011359, 0.004102, 0.000000},
  };
  // Find the closest wavelength in the CIE data
  int i = 0;
  while (cie_data[i].wavelength < wavelength) {
    i++;
    if (i >= sizeof(cie_data) / sizeof(cie_data[0])) {
      i = sizeof(cie_data) / sizeof(cie_data[0]) - 1;
      break;
    }
  }
  // the following 15 lines are taken from ChatGPT
  // Interpolate to get the corresponding x_bar, y_bar, z_bar
  if (i == 0) {
    *x_bar = cie_data[0].x_bar;
    *y_bar = cie_data[0].y_bar;
    *z_bar = cie_data[0].z_bar;
  } else if (i >= sizeof(cie_data) / sizeof(cie_data[0])) {
    *x_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].x_bar;
    *y_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].y_bar;
    *z_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].z_bar;
  } else {
    double ratio = (wavelength - cie_data[i - 1].wavelength) /
                   (cie_data[i].wavelength - cie_data[i - 1].wavelength);
    *x_bar = cie_data[i - 1].x_bar +
             (cie_data[i].x_bar - cie_data[i - 1].x_bar) * ratio;
    *y_bar = cie_data[i - 1].y_bar +
             (cie_data[i].y_bar - cie_data[i - 1].y_bar) * ratio;
    *z_bar = cie_data[i - 1].z_bar +
             (cie_data[i].z_bar - cie_data[i - 1].z_bar) * ratio;
  }
}

// the following 4 functions ro convert a spectrum to an raytracer::RGB value
// are taken from ChatGPT
void spectral_to_xyz(double *spd, double *wavelengths, int length, double *X,
                     double *Y, double *Z);
void spectral_to_xyz(double *spd, double *wavelengths, int length, double *X,
                     double *Y, double *Z) {
  // Normalize SPD values if not already normalized

  double max_spd = 0.0;
  for (int i = 0; i < length; ++i) {
    if (spd[i] > max_spd) {

      max_spd = spd[i];
    }
  }
  if (max_spd > 1.0) {
    for (int i = 0; i < length; ++i) {
      spd[i] /= max_spd;
    }
  }

  // Initialize XYZ values
  *X = 0;
  *Y = 0;
  *Z = 0;
  for (int i = 0; i < length; ++i) {
    if (spd[i] != spd[i]) {

    } else {

      double lambda = wavelengths[i];
      double x_bar, y_bar, z_bar;
      wavelength_to_xyz(lambda, &x_bar, &y_bar, &z_bar);
      *X += spd[i] * x_bar;
      *Y += spd[i] * y_bar;
      *Z += spd[i] * z_bar;
    }
  }
}
void xyz_to_RGB(double X, double Y, double Z, double *R, double *G, double *B);
void xyz_to_RGB(double X, double Y, double Z, double *R, double *G, double *B) {
  // Define the transformation matrix from XYZ to linear sraytracer::RGB

  double M[3][3] = {{3.2406, -1.5372, -0.4986},
                    {-0.9689, 1.8758, 0.0415},
                    {0.0557, -0.2040, 1.0570}};
  *R = M[0][0] * X + M[0][1] * Y + M[0][2] * Z;
  *G = M[1][0] * X + M[1][1] * Y + M[1][2] * Z;
  *B = M[2][0] * X + M[2][1] * Y + M[2][2] * Z;
}
double gamma_correct(double value);
double gamma_correct(double value) {

  if (value <= 0.0031308) {
    return 12.92 * value;
  } else {
    return 1.055 * pow((float)value, 1 / 2.4) - 0.055;
  }
}
void spectral_to_RGB(double *spd, double *wavelengths, int length, int *R,
                     int *G, int *B);
void spectral_to_RGB(double *spd, double *wavelengths, int length, int *R,
                     int *G, int *B) {
  // Convert the spectral distribution to XYZ
  double X, Y, Z;
  spectral_to_xyz(spd, wavelengths, length, &X, &Y, &Z);

  // Convert XYZ to linear raytracer::RGB
  double RGB_linear[3];
  xyz_to_RGB(X, Y, Z, &RGB_linear[0], &RGB_linear[1], &RGB_linear[2]);

  // Apply gamma correction
  double RGB_corrected[3];
  for (int i = 0; i < 3; ++i) {
    RGB_corrected[i] = gamma_correct(RGB_linear[i]);
  }

  // Clamp and scale to [0, 255]

  *R = (int)(fmax((float)0.0, fmin((float)1.0, RGB_corrected[0])) * 255);
  *G = (int)(fmax((float)0.0, fmin((float)1.0, RGB_corrected[1])) * 255);
  *B = (int)(fmax((float)0.0, fmin((float)1.0, RGB_corrected[2])) * 255);
}

// ray tracing computations are below
raytracer::spectrum_of_light
compute_the_ray(ray current_ray, raytracer::surface_array *all_surfaces,
                raytracer::triangle_array *all_faces, Vector3D light_position,
                double light_radius, uint *seed, int max_recursion);
raytracer::spectrum_of_light
compute_the_ray(ray current_ray, raytracer::surface_array *all_surfaces,
                raytracer::triangle_array *all_faces, Vector3D light_position,
                double light_radius, uint *seed, int max_recursion) {

  if (current_ray.recursion_index + 1 > max_recursion) {

    return current_ray.ray_spectrum;
  }

  int triangle_index = -1;
  double minimum_distance_of_ray_to_a_traingle = 999999999;

  for (int a = 0; a < all_faces->total_number_of_triangles; a++) {
    if (does_triangle_and_ray_intersect_correctly(
            current_ray, all_faces->all_triangles[a])) {
      double distance = compute_distance_with_a_triangle_and_ray(
          current_ray, all_faces->all_triangles[a]);
      // printf(" distance %f  \n", distance);
      if (a == 0 || (distance < minimum_distance_of_ray_to_a_traingle)) {
        triangle_index = a;

        minimum_distance_of_ray_to_a_traingle = distance;
      }
    }
  }

  double distance_to_light =
      compute_distance_with_a_ray_and_a_point(current_ray, light_position);

  // it directly intersects with the light

  if (distance_to_light <= light_radius && distance_to_light >= 0) {

    return current_ray.ray_spectrum;
  }

  if (triangle_index == -1) {
    raytracer::spectrum_of_light zero_one;
    zero_one.number_of_elements = (700 - 380) / 5 + 1;
    for (int y = 0; y < zero_one.number_of_elements; y++) {
      zero_one.spectrum[y] = 0;
    }
    return zero_one;
  }

  ray new_ray_specular;
  raytracer::spectrum_of_light spectrum_of_current_ray;
  spectrum_of_current_ray.number_of_elements = (700 - 380) / 5 + 1;

  int surface_index = all_faces->all_triangles[triangle_index].surface_number;

  for (int o = 0; o < spectrum_of_current_ray.number_of_elements; o++) {
    double a1 = current_ray.ray_spectrum.spectrum[o];
    raytracer::surface a2 = all_surfaces->surface_array[surface_index];
    double a3 = a2.surface_spectrum.spectrum[o];
    double a4 =
        (1 -
         (all_surfaces->surface_array[surface_index]).diffusion_coefficient);

    spectrum_of_current_ray.spectrum[o] = a1 * a3 * a4;
  }
  new_ray_specular.ray_spectrum = spectrum_of_current_ray;

  ray new_ray_diffusion;
  raytracer::spectrum_of_light spectrum_of_current_ray2;
  spectrum_of_current_ray2.number_of_elements = (700 - 380) / 5 + 1;

  for (int o = 0; o < spectrum_of_current_ray2.number_of_elements; o++) {
    double a1 = current_ray.ray_spectrum.spectrum[o];
    raytracer::surface a2 = all_surfaces->surface_array[surface_index];
    double a3 = a2.surface_spectrum.spectrum[o];
    double a4 =
        ((all_surfaces->surface_array[surface_index]).diffusion_coefficient);
    spectrum_of_current_ray2.spectrum[o] = a1 * a3 * a4;
  }

  new_ray_diffusion.ray_spectrum = spectrum_of_current_ray2;
  Vector3D new_location = find_intersection_of_a_triangle_with_a_ray(
      current_ray, all_faces->all_triangles[triangle_index]);
  Vector3D new_specular_direction = give_specular_direction(
      current_ray, all_faces->all_triangles[triangle_index]);
  Vector3D new_diffusive_direction = give_diffusive_direction(
      current_ray, all_faces->all_triangles[triangle_index], seed);

  new_ray_specular.ray_current_coordinates = new_location;
  new_ray_diffusion.ray_current_coordinates = new_location;

  new_ray_specular.ray_direction = new_specular_direction;
  new_ray_diffusion.ray_direction = new_diffusive_direction;

  new_ray_specular.recursion_index = current_ray.recursion_index + 1;
  new_ray_diffusion.recursion_index = current_ray.recursion_index + 1;

  raytracer::spectrum_of_light specular_spectrum =
      compute_the_ray(new_ray_specular, all_surfaces, all_faces, light_position,
                      light_radius, seed, max_recursion);
  raytracer::spectrum_of_light diffusive_spectrum =
      compute_the_ray(new_ray_diffusion, all_surfaces, all_faces,
                      light_position, light_radius, seed, max_recursion);

  add_second_spectrum_to_first_one(&specular_spectrum, diffusive_spectrum);

  return specular_spectrum;
}

raytracer::RGB trace(int which_pixel_x_coord, int which_pixel_y_coord,
                     double camara_plane_x_width, double camera_plane_y_width,
                     int number_of_x_pixels, int number_of_y_pixels,
                     int random_number_generator_seed, double minimum_distance,
                     raytracer::surface_array all_surfaces,
                     raytracer::triangle_array all_faces,
                     Vector3D light_position, double light_radius,
                     int max_recursion) {
  uint our_random_number = (uint)random_number_generator_seed;

  raytracer::spectrum_of_light resultant_spectrum;
  resultant_spectrum.number_of_elements = (700 - 380) / 5 + 1;
  for (int t = 0; t < resultant_spectrum.number_of_elements; t++) {
    resultant_spectrum.spectrum[t] = 0.0;
  }

  raytracer::Vector2D_array to_be_returned =
      random_points_giver(camera_plane_y_width / ((double)(number_of_y_pixels)),
                          camara_plane_x_width / ((double)(number_of_x_pixels)),
                          minimum_distance, &our_random_number);

  // printf("The integer final final is: %d\n",
  // to_be_returned.number_of_elements);

  raytracer::Vector3D_array final_plane =
      place_it_to_new_location(&to_be_returned,
                               which_pixel_x_coord * camara_plane_x_width /
                                       ((double)(number_of_x_pixels)) -
                                   (camara_plane_x_width / 2),
                               which_pixel_y_coord * camera_plane_y_width /
                                       ((double)(number_of_y_pixels)) -
                                   (camera_plane_y_width / 2));

  for (int i = 0; i < final_plane.number_of_elements; i++) {

    ray current_ray;
    current_ray.recursion_index = 0;
    raytracer::spectrum_of_light spectrum_of_current_ray;
    spectrum_of_current_ray.number_of_elements = (700 - 380) / 5 + 1;
    // initialize the white light

    for (int o = 0; o < spectrum_of_current_ray.number_of_elements; o++) {
      spectrum_of_current_ray.spectrum[o] = (double)1;
    }
    current_ray.ray_spectrum = spectrum_of_current_ray;
    current_ray.ray_direction = copy_given3d(
        final_plane.array[i]); // The COP is assumed to be at the origin
    current_ray.ray_current_coordinates = copy_given3d(final_plane.array[i]);

    raytracer::spectrum_of_light the_new_spectrum =
        compute_the_ray(current_ray, &all_surfaces, &all_faces, light_position,
                        light_radius, &our_random_number, max_recursion);

    add_second_spectrum_to_first_one(&resultant_spectrum, the_new_spectrum);
  }

  for (int j = 0; j < resultant_spectrum.number_of_elements; j++) {
    resultant_spectrum.spectrum[j] /= (double)final_plane.number_of_elements;
  }

  // code to convert the final resultant light spectrum into raytracer::RGB
  // values
  int length = (int)((700 - 380) / 5 + 1);
  double wavelengths[length];
  double spd[length];

  for (int i = 0; i < length; ++i) {
    wavelengths[i] =
        380 + 5 * i; // Example wavelengths from 380nm to 780nm in 5nm steps
    spd[i] = resultant_spectrum.spectrum[i];
    if (spd[i] != 0.0) {
      // printf("spectrum %f \n",spd[i]);
    }
  }

  int R, G, B;

  spectral_to_RGB(spd, wavelengths, length, &R, &G, &B);
  // printf("raytracer::RGB: (%d, %d, %d)\n", R, G, B);

  raytracer::RGB colour;
  colour.R = (uchar)R;
  colour.G = (uchar)G;
  colour.B = (uchar)B;

  return colour;
}

#define VEC3TOFLOAT4ONEIDX(vec3, float4, i) float4.i = vec3.i
#define VEC3TOFLOAT4(vec3, float4)                                             \
  VEC3TOFLOAT4ONEIDX(vec3, float4, x);                                         \
  VEC3TOFLOAT4ONEIDX(vec3, float4, y);                                         \
  VEC3TOFLOAT4ONEIDX(vec3, float4, z);

#define CHECK_ERROR(x) CL_RUN3(;)

#define RANDOM_POINTS_GIVER_ARRAY_ONE_DIM (2)
#define RANDOM_POINTS_GIVER_ARRAY_LENGTH                                       \
  (RANDOM_POINTS_GIVER_ARRAY_ONE_DIM * RANDOM_POINTS_GIVER_ARRAY_ONE_DIM)

int main() {

  raytracer::surface yellow_surface;
  raytracer::spectrum_of_light yellow_spectrum;
  yellow_spectrum.number_of_elements = (int)((700 - 380) / 5 + 1);
  for (int a = 380; a < 700 + 1; a += 5) {
    if (a >= 560 && a <= 590) {
      yellow_spectrum.spectrum[(int)((a - 380) / 5)] = 0.9;

    } else {
      yellow_spectrum.spectrum[(int)((a - 380) / 5)] = 0.1;
    }
  }
  yellow_surface.diffusion_coefficient = 0.6;

  raytracer::surface_array all_surfaces;
  yellow_surface.surface_spectrum = yellow_spectrum;
  all_surfaces.surface_array[0] = yellow_surface;

  raytracer::triangle yellow_triangle;
  yellow_triangle.surface_number = 0;
  Vector3D v1;
  v1.x = -1.0;
  v1.y = 0.0;
  v1.z = 3.0;
  Vector3D v2;
  v2.x = 0.0;
  v2.y = 1.0;
  v2.z = 3.0;
  Vector3D v3;
  v3.x = 1.0;
  v3.y = 0.0;
  v3.z = 3.0;

  yellow_triangle.coordinates1 = v1;
  yellow_triangle.coordinates2 = v2;
  yellow_triangle.coordinates3 = v3;

  raytracer::triangle_array all_tris;
  all_tris.total_number_of_triangles = 1;
  all_tris.all_triangles[0] = yellow_triangle;

  Vector3D light_position;
  light_position.x = 0.0;
  light_position.y = 0.0;
  light_position.z = 0.0;

  /**
  __kernel void trace(
    double camara_plane_x_width, double camera_plane_y_width,
    int number_of_x_pixels, int number_of_y_pixels,
    int random_number_generator_seed, double minimum_distance,
    __global double *all_surfaces_spectrums, __global double
  *all_surfaces_coeffs, int surface_count,
    __global float4 *all_triangles_coords, __global int *all_triangles_surfaces,
  int triangle_count, float4 light_position, double light_radius, int
  max_recursion, __global float4 *out_data)
  */

  int num_surfaces = 4; // I guess hard coded 50?
  int num_triangles = all_tris.total_number_of_triangles;
  printf("NUM TRIANGLES: %d\n", num_triangles);
  float *all_surfaces_spectrums;
  float *all_surfaces_coeffs;
  float4 *all_triangles_coords;
  int *all_triangles_surfaces;

  all_surfaces_spectrums =
      (float *)calloc(num_surfaces, sizeof(float) * SPECTRUM_SIZE);
  all_surfaces_coeffs = (float *)calloc(num_surfaces, sizeof(float));
  all_triangles_coords = (float4 *)calloc(num_triangles, sizeof(float4) * 3);
  all_triangles_surfaces = (int *)calloc(num_triangles, sizeof(int));

  float4 *outc = (float4 *)calloc(OUT_BUFFER_LEN, sizeof(float4));

  // put all the data in

  for (int i = 0; i < all_tris.total_number_of_triangles; i++) {
    auto tri = all_tris.all_triangles + i;
    VEC3TOFLOAT4(tri->coordinates1, all_triangles_coords[(i * 3) + 0]);
    VEC3TOFLOAT4(tri->coordinates2, all_triangles_coords[(i * 3) + 1]);
    VEC3TOFLOAT4(tri->coordinates3, all_triangles_coords[(i * 3) + 2]);
    all_triangles_surfaces[i] = tri->surface_number;
  }

  for (int i = 0; i < num_surfaces; i++) {
    auto surface = all_surfaces.surface_array + i;
    for (int j = 0; j < SPECTRUM_SIZE; j++) {
      all_surfaces_spectrums[(i * SPECTRUM_SIZE) + j] =
          surface->surface_spectrum.spectrum[j];
    }
    all_surfaces_coeffs[i] = surface->diffusion_coefficient;
  }

  double cam_w = 1.0;
  double cam_h = 1.0;

  int num_pixels[] = {NUM_PIXELS_X, NUM_PIXELS_Y};

  int seed = 42;

  double min_distance = 0.0005;

  double light_radius = 2.0;

  int max_recursion = 6;

  float4 light_pos4;

  VEC3TOFLOAT4(light_position, light_pos4);

  // CL_RUN3(cl_mem d_all_surfaces_spectrums =
  //             clCreateBuffer(context, CL_MEM_READ_ONLY |
  //             CL_MEM_COPY_HOST_PTR,
  //                            num_surfaces * SPECTRUM_SIZE * sizeof(float),
  //                            all_surfaces_spectrums, &err));
  // CL_RUN3(cl_mem d_all_surfaces_coeffs = clCreateBuffer(
  //             context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
  //             num_surfaces * sizeof(float), all_surfaces_coeffs, &err);)
  // CL_RUN3(cl_mem d_all_triangles_coords = clCreateBuffer(
  //             context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
  //             num_triangles * 3 * sizeof(float4), all_triangles_coords,
  //             &err);)
  // CL_RUN3(cl_mem d_all_triangles_surfaces = clCreateBuffer(
  //             context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
  //             num_triangles * sizeof(cl_int), all_triangles_surfaces, &err));
  // CL_RUN3(cl_mem d_out_data =
  //             clCreateBuffer(context, CL_MEM_READ_WRITE |
  //             CL_MEM_COPY_HOST_PTR,
  //                            OUT_BUFFER_LEN * sizeof(float4), outc, &err););

  int randsize = RANDOM_POINTS_GIVER_ARRAY_LENGTH * OUT_BUFFER_LEN;
  // CL_RUN3(cl_mem d_g_to_be_returned =
  //             clCreateBuffer(context, CL_MEM_READ_WRITE |
  //             CL_MEM_HOST_NO_ACCESS,
  //                            randsize * sizeof(float2), NULL, &err););
  // CL_RUN3(cl_mem d_g_final_plane =
  //             clCreateBuffer(context, CL_MEM_READ_WRITE |
  //             CL_MEM_HOST_NO_ACCESS,
  //                            randsize * sizeof(float4), NULL, &err););
  // CL_RUN3(cl_mem d_random_buffer =
  //             clCreateBuffer(context, CL_MEM_READ_WRITE |
  //             CL_MEM_HOST_NO_ACCESS,
  //                            randsize * sizeof(float2) * 2, NULL, &err););
  // CL_RUN3(cl_mem d_bg_buffer =
  //             clCreateBuffer(context, CL_MEM_READ_WRITE |
  //             CL_MEM_HOST_NO_ACCESS,
  //                            randsize * sizeof(cl_int), NULL, &err););
  // // SPECTRUM_NUM_ELEMS
  // CL_RUN3(cl_mem g_all_ray_elems = clCreateBuffer(
  //             context, CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
  //             SPECTRUM_SIZE * OUT_BUFFER_LEN * sizeof(float), NULL, &err););

  // err = clSetKernelArg(kernel, 0, sizeof(float), &cam_w);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 1, sizeof(float), &cam_h);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 2, sizeof(cl_int), &num_pixels[0]);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 3, sizeof(cl_int), &num_pixels[1]);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 4, sizeof(cl_int), &seed);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 5, sizeof(float), &min_distance);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 6, sizeof(cl_mem), &d_all_surfaces_spectrums);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 7, sizeof(cl_mem), &d_all_surfaces_coeffs);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 8, sizeof(cl_int), &num_surfaces);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 9, sizeof(cl_mem), &d_all_triangles_coords);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 10, sizeof(cl_mem),
  // &d_all_triangles_surfaces); CHECK_ERROR(err); err = clSetKernelArg(kernel,
  // 11, sizeof(cl_int), &num_triangles); CHECK_ERROR(err); err =
  // clSetKernelArg(kernel, 12, sizeof(float4), &light_pos4); CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 13, sizeof(float), &light_radius);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 14, sizeof(cl_int), &max_recursion);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 15, sizeof(cl_mem), &d_out_data);
  // CHECK_ERROR(err);

  /**
 __global float2 *g_to_be_returned, __global float4 *g_final_plane, __global
 float2 *random_buffer , __global int *bgbuffer
 */
  // err = clSetKernelArg(kernel, 16, sizeof(cl_mem), &d_g_to_be_returned);
  // CHECK_ERROR(err);

  // err = clSetKernelArg(kernel, 17, sizeof(cl_mem), &d_g_final_plane);
  // CHECK_ERROR(err);

  // err = clSetKernelArg(kernel, 18, sizeof(cl_mem), &d_random_buffer);
  // CHECK_ERROR(err);

  // err = clSetKernelArg(kernel, 19, sizeof(cl_mem), &d_bg_buffer);
  // CHECK_ERROR(err);
  // err = clSetKernelArg(kernel, 20, sizeof(cl_mem), &g_all_ray_elems);
  // CHECK_ERROR(err);

  size_t global_work_size[2] = {NUM_PIXELS_X, NUM_PIXELS_Y};
  size_t local_size[2] = {4, 4};

  // CL_RUN2(clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size,
  //                                local_size, 0, NULL, NULL));
  // CL_RUN2(clFlush(queue));
  // CL_RUN2(clFinish(queue));

  // CL_RUN2(clEnqueueReadBuffer(queue, d_out_data, CL_TRUE, 0, 1, outc, 0,
  // NULL,
  //                             NULL));

  ray exp;
  Vector3D e = {0.0, 0.01, 1.0};
  exp.ray_current_coordinates = e;
  exp.ray_direction = e;

  save_png("./test.png", (unsigned char *)outc, NUM_PIXELS_X, NUM_PIXELS_Y);

  return 0;
}
