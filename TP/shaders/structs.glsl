struct CameraData {
    mat4 view_proj;
    mat4 inv_view_proj;
};

struct FrameData {
    CameraData camera;

    vec3 sun_dir;
    uint point_light_count;

    vec3 sun_color;
    float padding_1;
};

struct PointLight {
    vec3 position;
    float radius;
    vec3 color;
    float padding_1;
};

struct AdvancedCameraData {
    vec3 position; 
    vec3 forward;
    vec3 up; 
    vec3 right;
    float half_fov; 
    vec2 window_size; 
    float tile_size;
};