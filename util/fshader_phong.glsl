// Per-fragment interpolated values from the vertex shader

#version 410

in  vec3 fN;
in  vec3 fL;
in  vec3 fV;

in vec2 texc;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform float Shininess;
uniform sampler2D texture1;
uniform int HasTexture;

out vec4 color;

void main() 
{ 
        // Normalize the input lighting vectors
        vec3 N = normalize(fN);
        vec3 V = normalize(fV);
        vec3 L = normalize(fL);

        vec3 H = normalize( L + V );
        
        vec4 dfs = DiffuseProduct;
        if(HasTexture >= 1){
            dfs = (.1 * DiffuseProduct) + (.9 * texture(texture1, texc));
        }
        
        vec4 ambient = AmbientProduct;

        float Kd = max(dot(L, N), 0.0);
        vec4 diffuse = Kd*dfs;
        
        float Ks = pow(max(dot(N, H), 0.0), Shininess);
        vec4 specular = Ks*SpecularProduct;

        // discard the specular highlight if the light's behind the vertex
        if( dot(L, N) < 0.0 ) {
            specular = vec4(0.0, 0.0, 0.0, 1.0);
        }

    color = ambient + diffuse + specular;
        color.a = 1.0;

} 

