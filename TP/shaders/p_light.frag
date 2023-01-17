#version 450

#include "utils.glsl"

layout(location = 0) in vec2 in_uv;

layout(location = 0) out vec4 out_color;

uniform mat4 invert_view_projection;

layout(binding = 0) uniform sampler2D in_albedo;
layout(binding = 1) uniform sampler2D in_normal;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) uniform PLightData {
    PointLight point_light;
};

vec3 unproject(vec2 uv, float depth, mat4 inv_view_proj) {
    const vec3 ndc = vec3(uv * 2.0 - vec2(1.0), depth);
    const vec4 p = inv_view_proj * vec4(ndc, 1.0);
    return p.xyz / p.w;
}

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    vec3 albedo = texelFetch(in_albedo, coord, 0).xyz;
    vec3 normal = texelFetch(in_normal, coord, 0).xyz;
    float depth = texelFetch(in_depth, coord, 0).x;

    if (depth > 0.0) {
        vec3 position = unproject(coord, depth, frame.camera.view_proj);

        const vec3 to_light = (point_light.position - position);
        const float dist = length(to_light);

        const vec3 light_vec = to_light / dist;

        const float NoL = dot(light_vec, normal);
        const float att = attenuation(dist, point_light.radius);

        vec3 lit = point_light.color;// * (NoL * att);

        out_color = vec4(albedo * lit, 1.0);
    }
    else
        out_color = vec4(vec3(0.0), 1.0);
}