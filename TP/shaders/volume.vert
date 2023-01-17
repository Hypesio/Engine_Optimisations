#version 450

#include "utils.glsl"

layout(location = 0) in vec3 in_pos;
layout(location = 2) in vec2 in_uv;

layout(location = 0) out vec2 out_uv;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) uniform PLightData {
    PointLight point_light;
};

layout(binding = 2) uniform Model {
    mat4 model;
};

void main() {
    vec4 position = model * vec4(in_pos, 1.0);
    gl_Position = frame.camera.view_proj * position;

    out_uv = in_uv;
}