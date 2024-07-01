@vs vs

in vec3 position;
in vec3 normal;
in vec2 texcoord;

uniform vs_params
{
    mat4 mvp;
};

// struct vertex_data 
// { 
//     vec4 position; 
//     vec4 normal; 
//     vec2 texcoord; 
// };

// readonly buffer ssbo 
// {
//     vertex_data verts[];
// };


out vec3 position_inter;
out vec3 normal_inter;
out vec2 texcoord_inter;


void main() 
{
    // vec4 position = verts[gl_VertexIndex].position;
    // gl_Position = mvp * position;
    // position_inter = position.xyz;
    // normal_inter = verts[gl_VertexIndex].normal.xyz;
    // texcoord_inter = verts[gl_VertexIndex].texcoord.xy;
    gl_Position = mvp * vec4(position.xyz, 1.0);
    position_inter = position.xyz;
    normal_inter = normal.xyz;
    texcoord_inter = texcoord.xy;
}
@end

@fs fs
in vec3 position_inter;
in vec3 normal_inter;
in vec2 texcoord_inter;
out vec4 frag_color;

const vec3 sun_pos = vec3(5, 20, 0);
const vec3 light_pos = vec3(0, 8, 2);

void main() 
{
    vec3 normal = normalize(normal_inter);
    vec3 light_dir = normalize(position_inter - light_pos);
    float ndotl = dot(normal, light_dir);
    frag_color = vec4(normal, 1.0);
}
@end

@program lit_model vs fs