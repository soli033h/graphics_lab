#version 330 core

uniform float time;

out vec4 fragment_color;

void main()
{
    fragment_color = vec4(0.5 + 0.5 * sin(time), 0.7, 1.0, 1.0);
}
