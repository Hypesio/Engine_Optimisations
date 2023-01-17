#version 450

#include "utils.glsl"

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

uniform mat4 invert_view_projection;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 0) uniform Data {
    FrameData frame;
};

vec3 ambient = vec3(0.0);

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec3 albedo = texelFetch(in_albedo, coord, 0).xyz;
    vec3 normal = texelFetch(in_normal, coord, 0).xyz;
    float depth = texelFetch(in_depth, coord, 0).x;

    if (depth > 0.0) {
        vec3 acc = frame.sun_color * max(0.0, dot(frame.sun_dir, normal)) + ambient;

        out_color = vec4(albedo * acc, 1.0);
    }
    else
    {
        out_color = vec4(albedo, 1.0);
    }
}