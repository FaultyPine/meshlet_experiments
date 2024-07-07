@vs vs
in vec3 position;
in vec3 color;

out vec3 color_inter;

uniform vs_params
{
    mat4 mvp;
};

void main() 
{
    gl_Position = mvp * vec4(position, 1);
    color_inter = color;
}
@end

@fs fs

in vec3 color_inter;

out vec4 frag_color;

void main() 
{
    frag_color = vec4(color_inter, 1.0);
}
@end

@program lines vs fs