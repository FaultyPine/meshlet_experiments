@vs vs


uniform vs_params
{
    mat4 mvp;
    float time;
};

struct vertex_data
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
// meshlet flags
const uint MESHLET_FLAG_VISIBLE = (1 << 0);
// ------
struct meshlet_data
{
    uint flags;
};
readonly buffer ssbo_meshlet_data
{
    meshlet_data meshlets[];
};

out vec3 position_inter;
out vec3 normal_inter;
out vec2 texcoord_inter;
out uint meshlet_idx_inter;
out float time_inter;
out uint is_visible_inter;

void main() 
{
    vec3 pos = verts[gl_VertexIndex].pos.xyz;
    normal_inter = verts[gl_VertexIndex].normal.xyz;
    texcoord_inter = verts[gl_VertexIndex].texcoord.xy;
    meshlet_idx_inter = verts[gl_VertexIndex].meshlet_idx;
    position_inter = pos;
    time_inter = time;
    is_visible_inter = meshlets[meshlet_idx_inter].flags & MESHLET_FLAG_VISIBLE;

    gl_Position = mvp * vec4(pos.xyz, 1.0);
}
@end

@fs fs
in vec3 position_inter;
in vec3 normal_inter;
in vec2 texcoord_inter;
flat in uint meshlet_idx_inter;
flat in float time_inter;
flat in uint is_visible_inter;

out vec4 frag_color;

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
    vec3 light_dir = normalize(vec3(0, 0.5, sin(time_inter)));
    float ndotl = dot(normal, light_dir);
    vec3 col = MeshletIdxToColor(meshlet_idx_inter);
    float alpha = 1.0;
    if (is_visible_inter == 0)
    {
        discard;
    }
    frag_color = vec4(col, alpha);
}
@end

@program lit_model vs fs