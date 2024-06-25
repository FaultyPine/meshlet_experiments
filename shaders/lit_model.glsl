@vs vs
in vec3 position;
in vec4 color;
//in vec3 normal;
//in vec2 texcoord;

//out vec3 position_inter;
//out vec3 normal_inter;
//out vec2 texcoord_inter;
out vec4 color0;

uniform vs_params
{
    mat4 mvp;
};

void main() 
{
    gl_Position = mvp * vec4(position, 1);
    color0 = color;
    // position_inter = position;
    // normal_inter = normal;
    // texcoord_inter = texcoord;
}
@end

@fs fs
// in vec3 position_inter;
// in vec3 normal_inter;
// in vec2 texcoord_inter;
in vec4 color0;
out vec4 frag_color;

void main() 
{
    // frag_color = vec4(position_inter, 1.0);
    frag_color = color0;
}
@end

@program lit_model vs fs