#version 330 core
in vec2 texture_coordinate;

uniform sampler2D image;

out vec4 fragment_color;

void main()
{
    fragment_color = texture(image, texture_coordinate);
}
