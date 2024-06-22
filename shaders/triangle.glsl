@vs vs
in vec2 position;
in vec2 texcoord;

out vec2 texcoord_inter;

void main() 
{
    gl_Position = vec4(position, 0, 1);
    texcoord_inter = texcoord;
}
@end

@fs fs
in vec2 texcoord_inter;
out vec4 frag_color;

uniform texture2D tex;
uniform sampler smp;


void main() 
{
    frag_color = texture(sampler2D(tex,smp), texcoord_inter);
}
@end

@program triangle vs fs