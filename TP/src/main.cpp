#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>

#include <graphics.h>
#include <SceneView.h>
#include <Texture.h>
#include <Framebuffer.h>
#include <ImGuiRenderer.h>
#include <shader_structs.h>

#include <imgui/imgui.h>


using namespace OM3D;

static float delta_time = 0.0f;
const glm::uvec2 window_size(1600, 900);


void glfw_check(bool cond) {
    if(!cond) {
        const char* err = nullptr;
        glfwGetError(&err);
        std::cerr << "GLFW error: " << err << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

void update_delta_time() {
    static double time = 0.0;
    const double new_time = program_time();
    delta_time = float(new_time - time);
    time = new_time;
}

void process_inputs(GLFWwindow* window, Camera& camera) {
    static glm::dvec2 mouse_pos;

    glm::dvec2 new_mouse_pos;
    glfwGetCursorPos(window, &new_mouse_pos.x, &new_mouse_pos.y);

    {
        glm::vec3 movement = {};
        if(glfwGetKey(window, 'W') == GLFW_PRESS) {
            movement += camera.forward();
        }
        if(glfwGetKey(window, 'S') == GLFW_PRESS) {
            movement -= camera.forward();
        }
        if(glfwGetKey(window, 'D') == GLFW_PRESS) {
            movement += camera.right();
        }
        if(glfwGetKey(window, 'A') == GLFW_PRESS) {
            movement -= camera.right();
        }

        float speed = 10.0f;
        if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            speed *= 10.0f;
        }

        if(movement.length() > 0.0f) {
            const glm::vec3 new_pos = camera.position() + movement * delta_time * speed;
            camera.set_view(glm::lookAt(new_pos, new_pos + camera.forward(), camera.up()));
        }
    }

    if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        const glm::vec2 delta = glm::vec2(mouse_pos - new_mouse_pos) * 0.01f;
        if(delta.length() > 0.0f) {
            glm::mat4 rot = glm::rotate(glm::mat4(1.0f), delta.x, glm::vec3(0.0f, 1.0f, 0.0f));
            rot = glm::rotate(rot, delta.y, camera.right());
            camera.set_view(glm::lookAt(camera.position(), camera.position() + (glm::mat3(rot) * camera.forward()), (glm::mat3(rot) * camera.up())));
        }

    }

    mouse_pos = new_mouse_pos;
}

void add_lights(std::unique_ptr<Scene>& scene)
{
        // Add lights
    {
        PointLight light;
        light.set_position(glm::vec3(0.0f, 50.0f, 0.0f));
        light.set_color(glm::vec3(255.0f, 255.0f, 255.0f));
        light.set_radius(40.0f);
        light.set_intensity(30.0f);
        scene->add_object(std::move(light));
    }
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 50.0f, -4.0f));
        light.set_color(glm::vec3(255.0f, 255.0f, 255.0f));
        light.set_radius(100.0f);
        light.set_intensity(50.0f);
        scene->add_object(std::move(light));
    }

    {
        PointLight light;
        light.set_position(glm::vec3(201.0f, 70.0f, 40.0f));
        light.set_color(glm::vec3(255.0f, 125.0f, 125.0f));
        light.set_radius(100.0f);
        light.set_intensity(50.0f);
        scene->add_object(std::move(light));
    }

    {
        PointLight light;
        light.set_position(glm::vec3(-201.0f, 70.0f, -340.0f));
        light.set_color(glm::vec3(255.0f, 125.0f, 125.0f));
        light.set_radius(250.0f);
        light.set_intensity(30.0f);
        scene->add_object(std::move(light));
    }

    {
        PointLight light;
        light.set_position(glm::vec3(201.0f, 70.0f, 240.0f));
        light.set_color(glm::vec3(255.0f, 125.0f, 125.0f));
        light.set_radius(50.0f);
        light.set_intensity(30.0f);
        scene->add_object(std::move(light));
    }

    {
        PointLight light;
        light.set_position(glm::vec3(-151.0f, 120.0f, -100.0f));
        light.set_color(glm::vec3(255.0f, 125.0f, 125.0f));
        light.set_radius(80.0f);
        light.set_intensity(30.0f);
        scene->add_object(std::move(light));
    }
}

std::unique_ptr<Scene> create_default_scene() {
    auto scene = std::make_unique<Scene>();

    // Load default cube model
    auto result = Scene::from_gltf(std::string(data_path) + "forest.glb");
    ALWAYS_ASSERT(result.is_ok, "Unable to load default scene");
    scene = std::move(result.value);
    
    add_lights(scene);

    // Order objects for instancing
    scene->order_objects_in_lists();

    return scene;
}


int main(int, char**) {
    DEBUG_ASSERT([] { std::cout << "Debug asserts enabled" << std::endl; return true; }());

    glfw_check(glfwInit());
    DEFER(glfwTerminate());

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(window_size.x, window_size.y, "TP window", nullptr, nullptr);
    glfw_check(window);
    DEFER(glfwDestroyWindow(window));

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    init_graphics();

    ImGuiRenderer imgui(window);

    std::unique_ptr<Scene> scene = create_default_scene();
    SceneView scene_view(scene.get());

    auto sphereSceneResult = Scene::from_gltf(std::string(data_path) + "sphere.glb");
    ALWAYS_ASSERT(sphereSceneResult.is_ok, "Unable to load default scene");
    std::unique_ptr<Scene> sphere_scene = std::move(sphereSceneResult.value);
    SceneView sphere_scene_view(sphere_scene.get());


    auto tonemap_program = Program::from_file("tonemap.comp");
    auto deferred_program = Program::from_files("deferred.frag", "screen.vert");
    auto plight_program = Program::from_files("p_light.frag", "volume.vert");
    auto transparent_program = Program::from_files("transparency.frag", "transparency.vert", std::array<std::string, 2>{"TEXTURED", "NORMAL_MAPPED"});
    auto oit_compute_program = Program::from_file("transparency.comp");
    auto tiled_program = Program::from_file("tiled.comp");

    auto deferred_mat = Material();
    deferred_mat.set_program(deferred_program);
    deferred_mat.set_blend_mode(BlendMode::Alpha);
    deferred_mat.set_depth_test_mode(DepthTestMode::None);
    deferred_mat.set_depth_mask(GL_FALSE);

    auto plight_mat = Material();
    plight_mat.set_program(plight_program);
    plight_mat.set_cull_mode(CullMode::Frontface);
    plight_mat.set_blend_mode(BlendMode::Additive);
    plight_mat.set_depth_test_mode(DepthTestMode::Reversed);
    plight_mat.set_depth_mask(GL_FALSE);

    Texture depth(window_size, ImageFormat::Depth32_FLOAT);
    Texture lit(window_size, ImageFormat::RGBA16_FLOAT);
    Texture transparent(window_size, ImageFormat::RGBA16_FLOAT);
    Texture color(window_size, ImageFormat::RGBA8_UNORM);
    
    Framebuffer tonemap_framebuffer(nullptr, std::array{&color});

    Texture g_depth(window_size, ImageFormat::Depth32_FLOAT);
    Texture albedo(window_size, ImageFormat::RGBA8_UNORM);
    Texture normals(window_size, ImageFormat::RGBA8_UNORM);
    Framebuffer g_buffer(&g_depth, std::array{&albedo, &normals});
    Framebuffer main_framebuffer(&g_depth, std::array{&lit});

    Texture ll_buffer(window_size.x * window_size.y * 8, ImageFormat::RGBA_32UI);
    
    int nb_buffers = 3;
    Texture *buffers[] = { &albedo, &normals, &transparent };
    int buffer_index = 0;
    int force_transparency_group = -1;
    std::shared_ptr<Material> last_material = nullptr;
    bool transparency_fb = false;
    for(;;) {
        glfwPollEvents();
        if(glfwWindowShouldClose(window) || glfwGetKey(window, GLFW_KEY_ESCAPE)) {
            break;
        }

        update_delta_time();

        if(const auto& io = ImGui::GetIO(); !io.WantCaptureMouse && !io.WantCaptureKeyboard) {
            process_inputs(window, scene_view.camera());
        }

        // Render the scene
        {
            g_buffer.bind();
            scene_view.render();
        }

        // Deferred operations
        {
            deferred_mat.bind();
            main_framebuffer.bind();

            albedo.bind(0);
            normals.bind(1);
            g_depth.bind(2);

            scene_view.deferred_render();

            // Compute deferred contribution of each visible point lights
            tiled_program->bind();

            lit.bind_as_image(0, AccessType::ReadWrite);

            albedo.bind(0);
            normals.bind(1);
            g_depth.bind(2);

            uint tile_size = 10;
            tiled_program->set_uniform("tile_size", tile_size);
            tiled_program->set_uniform("window_size", window_size);
            scene_view.tiled_render(window_size, tile_size);
        }
        
        // Render transparency
        {
            // Forward rendering of transparent objects
            Texture oit_head_list(window_size, ImageFormat::R32_UINT, 0);
            g_depth.bind(2);
            scene_view.render_transparent(oit_head_list, ll_buffer, transparency_fb);

            // Compute to sort pixels values
            oit_compute_program->bind(); 
            lit.bind(0); // Bind actual result 
            oit_head_list.bind_as_image(0, AccessType::ReadOnly); 
            transparent.bind_as_image(2, AccessType::WriteOnly); // Will write result on color image
            ll_buffer.bind_as_buffer(0);
            glDispatchCompute(align_up_to(window_size.x, 8) / 8, align_up_to(window_size.y, 8) / 8, 1);
        }

        // Apply a tonemap in compute shader
        {
            tonemap_program->bind();
            
            // Display debug buffer
            if (buffer_index > 0)
                buffers[buffer_index - 1]->bind(0);
            else 
                transparent.bind(0);

            color.bind_as_image(1, AccessType::WriteOnly);
            glDispatchCompute(align_up_to(window_size.x, 8) / 8, align_up_to(window_size.y, 8) / 8, 1);
        }
        // Blit tonemap result to screen
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        tonemap_framebuffer.blit();

        // GUI
        imgui.start();
        {
            char buffer[1024] = {};
            if(ImGui::InputText("Load scene", buffer, sizeof(buffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
                auto result = Scene::from_gltf(std::string(data_path) + buffer);
                if(!result.is_ok) {
                    std::cerr << "Unable to load scene (" << buffer << ")" << std::endl;
                } else {
                    scene = std::move(result.value);
                    add_lights(scene);
                    scene->order_objects_in_lists();
                    scene_view = SceneView(scene.get());
                }
            }
            
            // Choose debug buffer to display
            ImGui::InputInt("Display debug", &buffer_index);
            if (buffer_index >= nb_buffers + 1)  
                buffer_index = 0;
            if (buffer_index < 0)
                buffer_index = nb_buffers;
            
            int new_group_force_transparency = force_transparency_group;
            ImGui::InputInt("Force transparency group", &new_group_force_transparency);
            if (new_group_force_transparency != force_transparency_group)
            {
                scene->undo_transparency(last_material);
                last_material = scene->force_transparency(transparent_program, new_group_force_transparency);
                force_transparency_group = new_group_force_transparency;
            }

            ImGui::Checkbox("Transparency front and back", &transparency_fb);
        }
        imgui.finish();

        glfwSwapBuffers(window);
    }

    scene = nullptr; // destroy scene and child OpenGL objects
}
