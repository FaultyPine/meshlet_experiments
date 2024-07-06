@vs vs


uniform vs_params
{
    mat4 mvp;
};

struct vertex_data // TODO: use interleaved vert data
{
    vec4 pos;
    vec4 normal;
    vec2 texcoord;
    uint meshlet_idx;
    uint pad;
};

readonly buffer ssbo_vert_data
{
    vertex_data verts[];
};

out vec3 position_inter;
out vec3 normal_inter;
out vec2 texcoord_inter;
out uint meshlet_idx_inter;

void main() 
{
    vec3 pos = verts[gl_VertexIndex].pos.xyz;
    normal_inter = verts[gl_VertexIndex].normal.xyz;
    texcoord_inter = verts[gl_VertexIndex].texcoord.xy;
    meshlet_idx_inter = verts[gl_VertexIndex].meshlet_idx;
    position_inter = pos;

    gl_Position = mvp * vec4(pos.xyz, 1.0);
}
@end

@fs fs
in vec3 position_inter;
in vec3 normal_inter;
in vec2 texcoord_inter;
flat in uint meshlet_idx_inter;
out vec4 frag_color;

const vec3 sun_pos = vec3(5, 20, 0);
const vec3 light_pos = vec3(0, 8, 2);

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}
vec3 MeshletIdxToColor(uint meshlet_idx)
{
    float id = float(meshlet_idx);
    const float PHI = (1.0 + sqrt(5.0))/2.0;
    float hue = id * PHI - floor(id * PHI); 
    vec3 col = hsv2rgb(vec3(hue, 0.99, 0.99));
    return col;
}

void main() 
{
    vec3 normal = normalize(normal_inter);
    vec3 light_dir = normalize(position_inter - light_pos);
    float ndotl = dot(normal, light_dir);
    frag_color = vec4(MeshletIdxToColor(meshlet_idx_inter), 1.0);
}
@end

@program lit_model vs fs