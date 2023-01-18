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


std::unique_ptr<Scene> create_default_scene() {
    auto scene = std::make_unique<Scene>();

    // Load default cube model
    auto result = Scene::from_gltf(std::string(data_path) + "forest.glb");
    ALWAYS_ASSERT(result.is_ok, "Unable to load default scene");
    scene = std::move(result.value);

    // Add lights
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 2.0f, 100.0f));
        light.set_color(glm::vec3(255.0f, 255.0f, 255.0f));
        light.set_radius(100.0f);
        scene->add_object(std::move(light));
    }
    {
        PointLight light;
        light.set_position(glm::vec3(1.0f, 50.0f, -4.0f));
        light.set_color(glm::vec3(255.0f, 255.0f, 255.0f));
        light.set_radius(100.0f);
        scene->add_object(std::move(light));
    }

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
    // TODO - remove - find better examples of transparency
    

    auto sphereSceneResult = Scene::from_gltf(std::string(data_path) + "sphere.glb");
    ALWAYS_ASSERT(sphereSceneResult.is_ok, "Unable to load default scene");
    std::unique_ptr<Scene> sphere_scene = std::move(sphereSceneResult.value);
    SceneView sphere_scene_view(sphere_scene.get());


    auto tonemap_program = Program::from_file("tonemap.comp");
    auto deferred_program = Program::from_files("deferred.frag", "screen.vert");
    auto plight_program = Program::from_files("p_light.frag", "volume.vert");
    auto transparent_program = Program::from_files("transparency.frag", "basic.vert");
    scene->force_transparency(transparent_program);

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

    auto transparent_mat = Material();

    Texture depth(window_size, ImageFormat::Depth32_FLOAT);
    Texture lit(window_size, ImageFormat::RGBA16_FLOAT);
    Texture color(window_size, ImageFormat::RGBA8_UNORM);
    
    Framebuffer tonemap_framebuffer(nullptr, std::array{&color});

    Texture g_depth(window_size, ImageFormat::Depth32_FLOAT);
    Texture albedo(window_size, ImageFormat::RGBA8_sRGB);
    Texture normals(window_size, ImageFormat::RGBA8_UNORM);
    Framebuffer g_buffer(&g_depth, std::array{&albedo, &normals});
    Framebuffer main_framebuffer(&g_depth, std::array{&lit});

    
    int nb_buffers = 2;
    Texture *buffers[] = { &albedo, &normals };
    int buffer_index = 0;
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
            scene_view.render(false);
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
            plight_mat.bind();

            albedo.bind(0);
            normals.bind(1);
            g_depth.bind(2);

            plight_mat.set_uniform("window_size", window_size);

            std::shared_ptr<StaticMesh> sphere_mesh = sphere_scene.get()->get_mesh(0);
            scene_view.point_lights_render(sphere_mesh);

            // Forward rendering of transparent objects
            // TODO bind right program
            scene_view.render(true);
        }

        // Apply a tonemap in compute shader
        {
            tonemap_program->bind();
            
            // Display debug buffer
            if (buffer_index > 0)
                buffers[buffer_index - 1]->bind(0);
            else 
                lit.bind(0);

            color.bind_as_image(1, AccessType::WriteOnly);
            glDispatchCompute(align_up_to(window_size.x, 8), align_up_to(window_size.y, 8), 1);
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
        }
        imgui.finish();

        glfwSwapBuffers(window);
    }

    scene = nullptr; // destroy scene and child OpenGL objects
}
