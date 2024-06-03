#define RANDOM_POINTS_GIVER_ARRAY_ONE_DIM (1)
#define RANDOM_POINTS_GIVER_ARRAY_LENGTH (RANDOM_POINTS_GIVER_ARRAY_ONE_DIM * RANDOM_POINTS_GIVER_ARRAY_ONE_DIM)
#define BGINDEX(a, i, j) (a + (i * RANDOM_POINTS_GIVER_ARRAY_ONE_DIM) + j)

__constant uint a = 1664525;
__constant uint c = 1013904223;
__constant uint m = 0xFFFFFFFF;

float random_float_between_0_1_giver(uint *seed)
{
    return 0.5;
    // Initialize the PRNG with the given seed
    uint state = *seed;

    state = (a * state + c) & m;
    *seed = state;

    return (float)(state) / (float)(m);
}

#define SPECTRUM_NUM_ELEMS ((((700 - 380) / 5 + 1)))
typedef float spectrum_of_light[SPECTRUM_NUM_ELEMS];

typedef struct
{
    float4 ray_current_coordinates;
    float4 ray_direction;
    spectrum_of_light ray_spectrum;
    int recursion_index;

} ray;

/**
  s1 and s2 are spectrum of light
 */
void add_second_spectrum_to_first_one(float *s1, float *s2)
{
    for (int i = 0; i < SPECTRUM_NUM_ELEMS; i++)
    {
        s1[i] += s2[i];
    }
}

float4 give_the_normal_of_a_triangle(float4 *input_coords)
{ // Source: https://stackoverflow.com/questions/19350792/calculate-normal-of-a-single-triangle-in-3d-space

    float4 A = input_coords[1] - input_coords[0];
    float4 B = input_coords[2] - input_coords[0];

    float3 N = cross(A.xyz, B.xyz);
    float4 ret;
    ret.xyz = N;
    ret.w = 0.0;
    return ret;
}

bool does_triangle_and_ray_intersect_correctly(ray *R, float4 *tri_coords)
{ // source is https://stackoverflow.com/questions/42740765/intersection-between-line-and-triangle-in-3d
    float t;
    float u;
    float v;

    float4 E1 = tri_coords[1] - tri_coords[0];
    float4 E2 = tri_coords[2] - tri_coords[0];

    float4 N = give_the_normal_of_a_triangle(tri_coords);
    float det = -dot(R->ray_direction, N);
    if (det < 1e-6)
        return false;
    float invdet = 1.0 / det;
    float4 AO = R->ray_current_coordinates - tri_coords[0];
    float4 DAO = cross(AO, R->ray_direction);
    u = dot(E2, DAO) * invdet;
    v = -dot(E1, DAO) * invdet;
    t = dot(AO, N) * invdet;
    return (t >= 0 && u >= 0 && v >= 0 && (u + v) <= 1.0);
}

float pointToPlaneDistance(float A, float B, float C, float D, float x1, float y1, float z1)
{
    // Calculate the numerator of the distance formula
    float numerator = fabs(((A * x1) + (B * y1) + (C * z1) + D));
    // Calculate the denominator of the distance formula
    float denominator = sqrt((pow(A, 2) + pow(B, 2) + pow(C, 2)));
    // Calculate the distance
    float distance = numerator / denominator;
    return distance;
}

float compute_distance_with_a_triangle_and_ray(ray *R, float4 *tri_coords)
{
    float4 normal = give_the_normal_of_a_triangle(tri_coords);
    float D = -(dot(normal, tri_coords[0]));
    return pointToPlaneDistance(normal.x, normal.y, normal.z, D, R->ray_current_coordinates.x, R->ray_current_coordinates.y, R->ray_current_coordinates.z);
}

float magnitude(float3 v)
{
    return length(v);
}

float compute_distance_with_a_ray_and_a_point(ray *current_ray, float4 light_position)
{
    float4 originToPoint = light_position - current_ray->ray_current_coordinates;
    originToPoint.w = 1;

    float dotProduct = dot(originToPoint, current_ray->ray_direction);

    float distance = magnitude(originToPoint.xyz);

    if (dotProduct < 0)
    {
        return -distance;
    }

    return distance;
}
float4 find_intersection_of_a_triangle_with_a_ray(ray *R, float4 *tri_coords)
{ // written with the help of ChatGPT
    float4 my_zero = R->ray_current_coordinates;
    float4 my_direction = R->ray_direction;

    float4 normal = give_the_normal_of_a_triangle(tri_coords);
    float D1 = -(dot(normal, tri_coords[0]));
    float A = normal.x, B = normal.y, C = normal.z, D = D1;

    float denominator = A * my_direction.x + B * my_direction.y + C * my_direction.z;

    float numerator = -(A * my_zero.x + B * my_zero.y + C * my_zero.z + D);
    float t = numerator / denominator;

    float4 intersection;

    intersection.x = my_zero.x + t * my_direction.x;
    intersection.y = my_zero.y + t * my_direction.y;
    intersection.z = my_zero.z + t * my_direction.z;

    return intersection;
}

float4 give_specular_direction(ray *current_ray, float4 *tri_coords)
{
    float4 planeNormal = give_the_normal_of_a_triangle(tri_coords);
    float4 incidentRay = current_ray->ray_direction;
    float cosTheta_i = dot(incidentRay.xyz, planeNormal.xyz) / (magnitude(incidentRay.xyz) * magnitude(planeNormal.xyz));

    // Calculate the reflected ray direction
    float4 reflectedRay = incidentRay - 2 * cosTheta_i * planeNormal;

    // Normalize the reflected ray direction
    float mag = magnitude(reflectedRay.xyz);

    reflectedRay.x /= mag;
    reflectedRay.y /= mag;
    reflectedRay.z /= mag;

    return reflectedRay;
}

float4 random_direction(float4 normal1, uint *seed)
{
    // Ensure the normal vector is normalized
    float4 normal = normalize(normal1);

    // Generate a random point on the unit hemisphere using spherical coordinates
    float phi = 2.0 * M_PI * random_float_between_0_1_giver(seed);
    float cosTheta = random_float_between_0_1_giver(seed);
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    float4 randomDirection = {sinTheta * cos(phi), sinTheta * sin(phi),
                              cosTheta, 0.0};
    // Create a coordinate system (u, v, w) with w aligned with the normal
    float4 w = normal;
    float4 u = fabs(w.x) > 0.0001 ? (float4){0, 1, 0, 0.0} : (float4){1, 0, 0, 0.0};
    u.xyz = normalize(cross(u.xyz, w.xyz));
    float4 v;
    v.xyz = cross(w.xyz, u.xyz);
    v.w = 0.0;

    // Transform the random direction to align with the normal
    float4 result = {randomDirection.x * u.x + randomDirection.y * v.x +
                         randomDirection.z * w.x,
                     randomDirection.x * u.y + randomDirection.y * v.y +
                         randomDirection.z * w.y,
                     randomDirection.x * u.z + randomDirection.y * v.z +
                         randomDirection.z * w.z,
                     0.0};

    return result;
}

float4 give_diffusive_direction(ray *current_ray, float4 *tri_coords, uint *seed)
{
    float4 planeNormal = give_the_normal_of_a_triangle(tri_coords);
    return random_direction(planeNormal, seed);
}

float2 randomPointInRange(float2 vector, float r, uint *seed)
{
    float angle = 2 * M_PI * random_float_between_0_1_giver(seed);

    float distance = r + (random_float_between_0_1_giver(seed) * r);

    float2 point;
    point.x = vector.x + distance * cos((float)angle);
    point.y = vector.y + distance * sin((float)angle);

    return point;
}

void array_element_remover(float2 *array, int index, int array_total_number_of_elemets)
{
    for (int a = 0; a < array_total_number_of_elemets - index - 1; a++)
    {
        array[index + a] = array[index + a + 1];
    }
}
void populate_the_random_k_points(float2 given, float min, float2 *to_be_populated, uint *seed)
{
    for (int a = 0; a < 30; a++)
    {
        to_be_populated[a] = randomPointInRange(given, min, seed);
    }
}

float distance_between(float2 v1, float2 v2)
{
    return (float)sqrt((float)((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y)));
}

int randomIntInRange(int n, int m, uint *seed);
int randomIntInRange(int n, int m, uint *seed)
{
    return n + (int)(random_float_between_0_1_giver(seed) * (float)(m - n - 1));
}

int fitness_checker(float2 current_point, float2 *location_array, int rows, int cols, int *background_array, float length, float width, int m, int n, float r, float tek_kare_uzunluk)
{
    if (current_point.x > 0 && current_point.x < length && current_point.y > 0 && current_point.y < width)
    {
        int x0 = floor(current_point.x / tek_kare_uzunluk);

        int y0 = floor(current_point.y / tek_kare_uzunluk);

        int i0 = max(y0 - 1, 0);
        int i1 = min(y0 + 1, m - 1);

        int j0 = max(x0 - 1, 0);
        int j1 = min(x0 + 1, n - 1);

        for (int i = i0; i <= i1; i++)
        {
            for (int j = j0; j <= j1; j++)
            {
                if (*BGINDEX(background_array, i, j) > -1 || (i == y0 && j == x0))
                {

                    int our_index = *BGINDEX(background_array, i, j);

                    float dista = (sqrt(pow((location_array[our_index].x - current_point.x), 2) + pow((location_array[our_index].y - current_point.y), 2)));

                    if (x0 == 6 && y0 == 1)
                    {
                    }

                    if (dista < r)
                        return 0;
                }
            }
        }
    }
    else
    {
        return 0;
    }

    return 1;
}

void random_points_giver(float width, float length,
                         float minimum_distance,
                         uint *seed, float2 *input, int *len, float2 *buffer, int *bg_buffer)
{

    float tek_kare_uzunluk = minimum_distance / sqrt((float)2);
    int n = ceil(length / tek_kare_uzunluk);
    int m = ceil(width / tek_kare_uzunluk);
    // int background_array[m][n];
    int *background_array = bg_buffer;

    for (int i = 0; i < m; i++)
    {
        for (int j = 0; j < n; j++)
        {
            *BGINDEX(background_array, i, j) = -1;
        }
    }
    float2 initial_random_vector;
    initial_random_vector.x = random_float_between_0_1_giver(seed) * length;
    initial_random_vector.y = random_float_between_0_1_giver(seed) * width;

    int x0 = floor(initial_random_vector.x / tek_kare_uzunluk);

    int y0 = floor(initial_random_vector.y / tek_kare_uzunluk);

    *BGINDEX(background_array, x0, y0) = 0;

    int active_array_current_index = 0;
    int location_array_index = 0;

    float2 *active_array = buffer;
    float2 *location_array = buffer + RANDOM_POINTS_GIVER_ARRAY_LENGTH;

    active_array[active_array_current_index] = initial_random_vector;
    location_array[location_array_index] = initial_random_vector;
    active_array_current_index++;
    location_array_index += 1;

    float2 random_k_points[30];
    while (active_array_current_index > 0)
    {
        int j = randomIntInRange(0, active_array_current_index, seed);

        populate_the_random_k_points(active_array[j], minimum_distance,
                                     random_k_points, seed);
        for (int i = 0; i < 30; i++)
        {
            float2 current_point = random_k_points[i];
            int j = fitness_checker(current_point, location_array, m, n,
                                    background_array, length, width, m, n,
                                    minimum_distance, tek_kare_uzunluk);

            if (j == 1)
            {
                active_array[active_array_current_index] = current_point;
                active_array_current_index += 1;
                int x1 = floor(current_point.x / tek_kare_uzunluk);

                int y1 = floor(current_point.y / tek_kare_uzunluk);

                *BGINDEX(background_array, x1, y1) = location_array_index;

                input[location_array_index] = current_point;

                location_array_index += 1;

                break;
            }
            if (i == 29)
            {
                array_element_remover(active_array, j, active_array_current_index);
                active_array_current_index -= 1;
            }
        }
    }

    *len = location_array_index;
}

void place_it_to_new_location(float2 *input, int input_len,
                              float new_x_coorinate, float new_y_coordiante, float4 *output, int *out_len)
{
    *out_len = input_len;
    for (int i = 0; i < input_len; i++)
    {
        float4 point;
        point.x = input[i].x + new_x_coorinate;
        point.y = input[i].y + new_y_coordiante;
        point.z = (float)1;
        point.w = 1.0;
        output[i] = point;
    }
}

// below are the functions to convert a spectrum a an raytracer::RGB value

typedef struct
{
    float wavelength;
    float x_bar;
    float y_bar;
    float z_bar;
} CIE1931;
// Placeholder function for CIE 1931 color matching functions
void wavelength_to_xyz(float wavelength, float *x_bar, float *y_bar,
                       float *z_bar);
void wavelength_to_xyz(
    float wavelength, float *x_bar, float *y_bar,
    float *z_bar)
{ // this function and data are obtained from chatgpt
    // Replace this function with actual CIE 1931 color matching functions
    // Example: return x_bar, y_bar, z_bar for the given wavelength
    // This example uses placeholder values and should be replaced with real data.
    CIE1931 cie_data[] = {
        {380, 0.001368, 0.000039, 0.006450},
        {385, 0.002236, 0.000064, 0.010550},
        {390, 0.004243, 0.000120, 0.020050},
        {395, 0.007650, 0.000217, 0.036210},
        {400, 0.014310, 0.000396, 0.067850},
        {405, 0.023190, 0.000640, 0.110200},
        {410, 0.043510, 0.001210, 0.207400},
        {415, 0.077630, 0.002180, 0.371300},
        {420, 0.134380, 0.004000, 0.645600},
        {425, 0.214770, 0.007300, 1.039050},
        {430, 0.283900, 0.011600, 1.385600},
        {435, 0.328500, 0.016840, 1.622960},
        {440, 0.348280, 0.023000, 1.747060},
        {445, 0.348060, 0.029800, 1.782600},
        {450, 0.336200, 0.038000, 1.772110},
        {455, 0.318700, 0.048000, 1.744100},
        {460, 0.290800, 0.060000, 1.669200},
        {465, 0.251100, 0.073900, 1.528100},
        {470, 0.195360, 0.090980, 1.287640},
        {475, 0.142100, 0.112600, 1.041900},
        {480, 0.095640, 0.139020, 0.812950},
        {485, 0.057950, 0.169300, 0.616200},
        {490, 0.032010, 0.208020, 0.465180},
        {495, 0.014700, 0.258600, 0.353300},
        {500, 0.004900, 0.323000, 0.272000},
        {505, 0.002400, 0.407300, 0.212300},
        {510, 0.009300, 0.503000, 0.158200},
        {515, 0.029100, 0.608200, 0.111700},
        {520, 0.063270, 0.710000, 0.078250},
        {525, 0.109600, 0.793200, 0.057250},
        {530, 0.165500, 0.862000, 0.042160},
        {535, 0.225750, 0.914850, 0.029840},
        {540, 0.290400, 0.954000, 0.020300},
        {545, 0.359700, 0.980300, 0.013400},
        {550, 0.433450, 0.994950, 0.008750},
        {555, 0.512050, 1.000000, 0.005750},
        {560, 0.594500, 0.995000, 0.003900},
        {565, 0.678400, 0.978600, 0.002750},
        {570, 0.762100, 0.952000, 0.002100},
        {575, 0.842500, 0.915400, 0.001800},
        {580, 0.916300, 0.870000, 0.001650},
        {585, 0.978600, 0.816300, 0.001400},
        {590, 1.026300, 0.757000, 0.001100},
        {595, 1.056700, 0.694900, 0.001000},
        {600, 1.062200, 0.631000, 0.000800},
        {605, 1.045600, 0.566800, 0.000600},
        {610, 1.002600, 0.503000, 0.000340},
        {615, 0.938400, 0.441200, 0.000240},
        {620, 0.854450, 0.381000, 0.000190},
        {625, 0.751400, 0.321000, 0.000100},
        {630, 0.642400, 0.265000, 0.000050},
        {635, 0.541900, 0.217000, 0.000030},
        {640, 0.447900, 0.175000, 0.000020},
        {645, 0.360800, 0.138200, 0.000010},
        {650, 0.283500, 0.107000, 0.000000},
        {655, 0.218700, 0.081600, 0.000000},
        {660, 0.164900, 0.061000, 0.000000},
        {665, 0.121200, 0.044580, 0.000000},
        {670, 0.087400, 0.032000, 0.000000},
        {675, 0.063600, 0.023200, 0.000000},
        {680, 0.046770, 0.017000, 0.000000},
        {685, 0.032900, 0.011920, 0.000000},
        {690, 0.022700, 0.008210, 0.000000},
        {695, 0.015840, 0.005723, 0.000000},
        {700, 0.011359, 0.004102, 0.000000},
    };
    // Find the closest wavelength in the CIE data
    int i = 0;
    while (cie_data[i].wavelength < wavelength)
    {
        i++;
        if (i >= sizeof(cie_data) / sizeof(cie_data[0]))
        {
            i = sizeof(cie_data) / sizeof(cie_data[0]) - 1;
            break;
        }
    }
    // the following 15 lines are taken from ChatGPT
    // Interpolate to get the corresponding x_bar, y_bar, z_bar
    if (i == 0)
    {
        *x_bar = cie_data[0].x_bar;
        *y_bar = cie_data[0].y_bar;
        *z_bar = cie_data[0].z_bar;
    }
    else if (i >= sizeof(cie_data) / sizeof(cie_data[0]))
    {
        *x_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].x_bar;
        *y_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].y_bar;
        *z_bar = cie_data[sizeof(cie_data) / sizeof(cie_data[0]) - 1].z_bar;
    }
    else
    {
        float ratio = (wavelength - cie_data[i - 1].wavelength) /
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
void spectral_to_xyz(float *spd, float *wavelengths, int length, float *X,
                     float *Y, float *Z);
void spectral_to_xyz(float *spd, float *wavelengths, int length, float *X,
                     float *Y, float *Z)
{
    // Normalize SPD values if not already normalized

    float max_spd = 0.0;
    for (int i = 0; i < length; ++i)
    {
        if (spd[i] > max_spd)
        {

            max_spd = spd[i];
        }
    }
    if (max_spd > 1.0)
    {
        for (int i = 0; i < length; ++i)
        {
            spd[i] /= max_spd;
        }
    }

    // Initialize XYZ values
    *X = 0;
    *Y = 0;
    *Z = 0;
    for (int i = 0; i < length; ++i)
    {
        if (spd[i] != spd[i])
        {
        }
        else
        {

            float lambda = wavelengths[i];
            float x_bar, y_bar, z_bar;
            wavelength_to_xyz(lambda, &x_bar, &y_bar, &z_bar);
            *X += spd[i] * x_bar;
            *Y += spd[i] * y_bar;
            *Z += spd[i] * z_bar;
        }
    }
}
void xyz_to_RGB(float X, float Y, float Z, float *R, float *G, float *B);
void xyz_to_RGB(float X, float Y, float Z, float *R, float *G, float *B)
{
    // Define the transformation matrix from XYZ to linear sraytracer::RGB

    float M[3][3] = {{3.2406, -1.5372, -0.4986},
                     {-0.9689, 1.8758, 0.0415},
                     {0.0557, -0.2040, 1.0570}};
    *R = M[0][0] * X + M[0][1] * Y + M[0][2] * Z;
    *G = M[1][0] * X + M[1][1] * Y + M[1][2] * Z;
    *B = M[2][0] * X + M[2][1] * Y + M[2][2] * Z;
}
float gamma_correct(float value);
float gamma_correct(float value)
{

    if (value <= 0.0031308)
    {
        return 12.92 * value;
    }
    else
    {
        return 1.055 * pow((float)value, (float)(1.0 / 2.4)) - 0.055;
    }
}
void spectral_to_RGB(float *spd, float *wavelengths, int length, int *R,
                     int *G, int *B);
void spectral_to_RGB(float *spd, float *wavelengths, int length, int *R,
                     int *G, int *B)
{
    // Convert the spectral distribution to XYZ
    float X, Y, Z;
    spectral_to_xyz(spd, wavelengths, length, &X, &Y, &Z);

    // Convert XYZ to linear raytracer::RGB
    float RGB_linear[3];
    xyz_to_RGB(X, Y, Z, &RGB_linear[0], &RGB_linear[1], &RGB_linear[2]);

    // Apply gamma correction
    float RGB_corrected[3];
    for (int i = 0; i < 3; ++i)
    {
        RGB_corrected[i] = gamma_correct(RGB_linear[i]);
    }

    // Clamp and scale to [0, 255]

    *R = (int)(fmax((float)0.0, fmin((float)1.0, (float)RGB_corrected[0])) * 255);
    *G = (int)(fmax((float)0.0, fmin((float)1.0, (float)RGB_corrected[1])) * 255);
    *B = (int)(fmax((float)0.0, fmin((float)1.0, (float)RGB_corrected[2])) * 255);
}

// spectrum of light is an array of floats
// surface array is an array of surfaces,
// where a surface has a spectrum and a diffusion coefficient
void compute_the_ray(ray *current_ray, float *all_surfaces_spectrums,
                     float *all_surfaces_coeffs,
                     float4 *all_faces_coords, int *all_faces_surfaces, int triangle_count, float4 light_position,
                     float light_radius, uint *seed, int max_recursion, float **out_spectrum, int gx, int gy)
{
    int triangle_index;
    float minimum_distance_of_ray_to_a_traingle;
    uint s = 1;
    for (int recursion_index = 0; recursion_index < max_recursion; recursion_index++)
    {
        triangle_index = -1;
        minimum_distance_of_ray_to_a_traingle = 999999999;

        for (int a = 0; a < triangle_count; a++)
        {
            if (does_triangle_and_ray_intersect_correctly(
                    current_ray, all_faces_coords + (a * 3)))
            {
                float distance = compute_distance_with_a_triangle_and_ray(
                    current_ray, all_faces_coords + (a * 3));
                // printf(" distance %f  \n", distance);
                if (a == 0 || (distance < minimum_distance_of_ray_to_a_traingle))
                {
                    triangle_index = a;

                    minimum_distance_of_ray_to_a_traingle = distance;
                }
            }
        }

        float distance_to_light =
            compute_distance_with_a_ray_and_a_point(current_ray, light_position);

        // it directly intersects with the light
        if (distance_to_light <= light_radius && distance_to_light >= 0)
        {
            *out_spectrum = current_ray->ray_spectrum;
            return;
        }

        if (triangle_index == -1)
        {
            for (int y = 0; y < SPECTRUM_NUM_ELEMS; y++)
            {
                out_spectrum[0][y] = 0;
            }
            return;
        }

        // instead of creating a new ray, use the old ray instead
        // float rand = random_float_between_0_1_giver(seed);
        int surface_index = all_faces_surfaces[triangle_index];
        // bool is_specular = rand > all_surfaces_coeffs[surface_index];

        for (int o = 0; o < SPECTRUM_NUM_ELEMS; o++)
        {
            float a1 = current_ray->ray_spectrum[o];
            float a3 = all_surfaces_spectrums[(surface_index * SPECTRUM_NUM_ELEMS) + o];

            current_ray->ray_spectrum[o] = a1 * a3;
        }

        float4 new_location = find_intersection_of_a_triangle_with_a_ray(
            current_ray, all_faces_coords + (triangle_index * 3));

        // float4 new_direction;
        float4 new_direction1;
        // float4 new_direction2;

        new_direction1 = give_specular_direction(
            current_ray, all_faces_coords + (triangle_index * 3));

        // new_direction2 = give_diffusive_direction(
        //     current_ray, all_faces_coords + (triangle_index * 3), &s);
        // float spec = all_surfaces_coeffs[surface_index];

        // new_direction.xyz = (normalize(new_direction1.xyz) * (spec)) + (normalize(new_direction2.xyz) * (1 - spec));
        // new_direction.w = 0.0;
        current_ray->ray_current_coordinates = new_location;
        current_ray->ray_direction = new_direction1;
    }
    *out_spectrum = current_ray->ray_spectrum;
    return;
}

/**
ARGUMENTS AND WHAT THEY ARE
camera_plane_x_width:         const shared mem, float
camera_plane_y_width:         const shared mem, float,
number_of_x_pixels:           const shared mem, int
number_of_y_pixels:           const shared mem, int
random_number_generator_seed: const shared mem, int
minimum_distance:             const shared mem, float
all_surfaces_spectrums:       const global float* (think of a contignius array of spectrums)
all_surfaces_coeffs:          const global float* (coeffs for all spectrums)
surface_count:                const shared mem, int
all_triangles_coords:         const global float4* (3* triangle count)
all_triangles_surfaces:       const global int*
triangle_count:               const shared mem, int
light_position:               const shared mem, float4
light_radius:                 const shared mem, float
max_recursion:                const shared mem, int

g_to_be_returned: RANDOM_POINTS_GIVER_ARRAY_LENGTH * num_points
similarly g_final_plane
random_buffer: 2 * RANDOM_POINTS_GIVER_ARRAY_LENGTH * num_points
 */
__kernel void trace(
    float camara_plane_x_width, float camera_plane_y_width,
    int number_of_x_pixels, int number_of_y_pixels,
    int random_number_generator_seed, float minimum_distance,
    __global float *all_surfaces_spectrums, __global float *all_surfaces_coeffs,
    int surface_count,
    __global float4 *all_triangles_coords, __global int *all_triangles_surfaces, int triangle_count,
    float4 light_position, float light_radius,
    int max_recursion, __global float4 *out_data, __global float2 *g_to_be_returned, __global float4 *g_final_plane, __global float2 *random_buffer, __global int *bgbuffer)
{

    // Get the global coordinates (x, y)
    int global_x = get_global_id(0);
    int global_y = get_global_id(1);

    // Get the local coordinates (x, y) within the workgroup
    int local_x = get_local_id(0);
    int local_y = get_local_id(1);

    // Get the workgroup coordinates (x, y)
    int group_x = get_group_id(0);
    int group_y = get_group_id(1);

    // Get the local workgroup size (x, y)
    int group_size_x = get_local_size(0);
    int group_size_y = get_local_size(1);

    if (global_x >= number_of_x_pixels || global_y >= number_of_y_pixels)
        return;

    int one_d_index = (global_y * number_of_x_pixels) + global_x;

    // give unique index so random is different for each one
    uint our_random_number = (uint)(random_number_generator_seed + one_d_index);

    ray my_ray;
    for (int t = 0; t < SPECTRUM_NUM_ELEMS; t++)
    {
        my_ray.ray_spectrum[t] = 0.0;
    }

    // float2 *to_be_returned = to_be_returned + (one_d_index * RANDOM_POINTS_GIVER_ARRAY_LENGTH);
    // int returned_len = RANDOM_POINTS_GIVER_ARRAY_LENGTH;

    // random_points_giver(camera_plane_y_width / ((float)(number_of_y_pixels)),
    //                     camara_plane_x_width / ((float)(number_of_x_pixels)),
    //                     minimum_distance, &our_random_number, to_be_returned, &returned_len, random_buffer + (2 * RANDOM_POINTS_GIVER_ARRAY_LENGTH * one_d_index), bgbuffer + (RANDOM_POINTS_GIVER_ARRAY_LENGTH * one_d_index));

    // float4 *final_plane = g_final_plane + (one_d_index * RANDOM_POINTS_GIVER_ARRAY_LENGTH);
    // int final_plane_len = RANDOM_POINTS_GIVER_ARRAY_LENGTH;
    // place_it_to_new_location(to_be_returned, returned_len,
    //                          global_x * camara_plane_x_width /
    //                                  ((float)(number_of_x_pixels)) -
    //                              (camara_plane_x_width / 2),
    //                          global_y * camera_plane_y_width /
    //                                  ((float)(number_of_y_pixels)) -
    //                              (camera_plane_y_width / 2),
    //                          final_plane, &final_plane_len);

    float resultant_spectrum[SPECTRUM_NUM_ELEMS];
    float *single_spectrum;

    // TODO change final plane iteration to another thread
    float p1x = ((float)global_x) * camara_plane_x_width /
                    ((float)(number_of_x_pixels)) -
                (camara_plane_x_width / 2);
    float p1y = ((float)global_y) * camera_plane_y_width /
                    ((float)(number_of_y_pixels)) -
                (camera_plane_y_width / 2);
    for (int i = 0; i < 1; i++)
    {
        float4 p1;
        p1.x = p1x;
        p1.y = p1y;
        p1.z = 1.0;
        p1.w = 1.0;

        // initialize the white light
        for (int t = 0; t < SPECTRUM_NUM_ELEMS; t++)
        {
            my_ray.ray_spectrum[t] = 1.0;
        }
        my_ray.ray_direction = p1;
        // The COP is assumed to be at the origin
        my_ray.ray_current_coordinates = p1;
        /**
        void compute_the_ray(ray *current_ray, float **all_surfaces_spectrums,
                             float *all_surfaces_coeffs,
                             triangle *all_faces, int triangle_count, float4 light_position,
                             float light_radius, uint *seed, int max_recursion, float **out_spectrum)
         */

        compute_the_ray(&my_ray, all_surfaces_spectrums, all_surfaces_coeffs, all_triangles_coords, all_triangles_surfaces, triangle_count, light_position, light_radius, &our_random_number, 2, &single_spectrum, global_x, global_y);
        // TODO REMOVE THIS
        return;
        add_second_spectrum_to_first_one(resultant_spectrum, my_ray.ray_spectrum);
    }

    for (int j = 0; j < SPECTRUM_NUM_ELEMS; j++)
    {
        resultant_spectrum[j] /= (float)RANDOM_POINTS_GIVER_ARRAY_LENGTH;
    }

    // code to convert the final resultant light spectrum into raytracer::RGB
    // values
    int length = SPECTRUM_NUM_ELEMS;
    float wavelengths[SPECTRUM_NUM_ELEMS];

    for (int i = 0; i < length; ++i)
    {
        wavelengths[i] =
            380 + 5 * i; // Example wavelengths from 380nm to 780nm in 5nm steps
    }

    int R, G, B;

    spectral_to_RGB(resultant_spectrum, wavelengths, length, &R, &G, &B);
    // printf("raytracer::RGB: (%d, %d, %d)\n", R, G, B);

    float4 colour;
    colour.x = R;
    colour.y = G;
    colour.z = B;

    out_data[one_d_index] = colour;
}