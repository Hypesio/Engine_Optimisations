#version 450

#include "utils.glsl"

// fragment shader of the main lighting pass

// #define DEBUG_NORMAL

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_normal;
layout(location = 1) in vec2 in_uv;
layout(location = 2) in vec3 in_color;
layout(location = 3) in vec3 in_position;
layout(location = 4) in vec3 in_tangent;
layout(location = 5) in vec3 in_bitangent;
layout(location = 6) in float depth; 

layout(offset = 0, binding = 0) uniform atomic_uint counter;

layout(binding = 0) uniform sampler2D in_texture;
layout(binding = 1) uniform sampler2D in_normal_texture;
layout(binding = 2) uniform sampler2D in_depth;

layout(binding = 2) uniform StorageSize {
    int maxSize; 
};

layout(r32ui, binding = 1) uniform uimage2D head_texture;
layout(rgba32ui, binding = 0) uniform uimageBuffer data_list;

layout(binding = 0) uniform Data {
    FrameData frame;
};

layout(binding = 1) buffer PointLights {
    PointLight point_lights[];
};

const vec3 ambient = vec3(0.0);

void main() {
    ivec2 coord = ivec2(gl_FragCoord.xy);
    float depth_z = texelFetch(in_depth, coord, 0).x;
    if (gl_FragCoord.z <= depth_z)
        discard;

#ifdef NORMAL_MAPPED
    const vec3 normal_map = unpack_normal_map(texture(in_normal_texture, in_uv).xy);
    const vec3 normal = normal_map.x * in_tangent +
                        normal_map.y * in_bitangent +
                        normal_map.z * in_normal;
#else
    const vec3 normal = in_normal;
#endif

    vec3 acc = frame.sun_color * max(0.0, dot(frame.sun_dir, normal)) + ambient;
    
    for(uint i = 0; i != frame.point_light_count; ++i) {
        PointLight light = point_lights[i];
        const vec3 to_light = (light.position - in_position);
        const float dist = length(to_light);
        const vec3 light_vec = to_light / dist;

        const float NoL = dot(light_vec, normal);
        const float att = attenuation(dist, light.radius) * light.intensity;
        if(NoL <= 0.0 || att <= 0.0f) {
            continue;
        }

        acc += light.color * (NoL * att);
    }

    out_color = vec4(in_color * acc, 1.0);

#ifdef TEXTURED
    out_color *= texture(in_texture, in_uv);
#endif

#ifdef DEBUG_NORMAL
    out_color = vec4(normal * 0.5 + 0.5, 1.0);
#endif

    // Here for test - Force transparency of object
    out_color[3] = 0.4f;

    int idx = int(atomicCounterIncrement(counter) + 1u);

    if (idx < maxSize)
    {
        uint prev = imageAtomicExchange(head_texture, ivec2(gl_FragCoord.xy), idx);
        uint color_rg = packHalf2x16(out_color.rg);
        uint color_ba = packHalf2x16(out_color.ba);
        imageStore(data_list, idx, uvec4(color_rg, color_ba, uint(depth), prev));
    }

    out_color = vec4(0.0);
}

