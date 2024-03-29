#ifndef SCENEVIEW_H
#define SCENEVIEW_H

#include <Scene.h>
#include <Camera.h>
#include <shader_structs.h>

namespace OM3D {

class SceneView {
    public:
        SceneView(const Scene* scene = nullptr);

        Camera& camera();
        const Camera& camera() const;

        void render() const;
        void render_transparent(Texture &head_list, Texture &ll_buffer, bool transparency_fb) const;
        void deferred_render() const;
        void point_lights_render(std::shared_ptr<StaticMesh> sphere_mesh) const;
        void tiled_render(glm::uvec2 window_size, size_t tile_size) const;

    private:
        const Scene* _scene = nullptr;
        Camera _camera;

};

}

#endif // SCENEVIEW_H
