#version 330 core

uniform float time;

out vec4 fragment_color;

void main()
{
    const float two_pi_over_three = 2.0943951;
    const vec3 color = 0.5 + 0.5 * vec3(
        sin(time),
        sin(time + two_pi_over_three),
        sin(time + 2.0 * two_pi_over_three));

    fragment_color = vec4(color, 1.0);
}
