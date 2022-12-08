#version 450

#include "utils.glsl"

// fragment shader of the main lighting pass

// #define DEBUG_NORMAL

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;

layout(binding = 0) uniform sampler2D in_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

const vec3 ambient = vec3(0.0);

void main() {
    out_albedo = vec4(in_color, 1.0);

#ifdef TEXTURED
    out_albedo *= texture(in_texture, in_uv);
#endif
    
    out_normal = vec4(normalize(in_normal), 1.0);
}

