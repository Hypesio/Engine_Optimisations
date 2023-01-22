#include "SceneView.h"

namespace OM3D {

SceneView::SceneView(const Scene* scene) : _scene(scene) {
}

Camera& SceneView::camera() {
    return _camera;
}

const Camera& SceneView::camera() const {
    return _camera;
}

void SceneView::render() const {
    if(_scene) {
        _scene->render(_camera);
    }
}

void SceneView::render_transparent(Texture &head_list, Texture &ll_buffer) const {
    if(_scene) {
        _scene->render_transparent(_camera, head_list, ll_buffer);
    }
}

void SceneView::deferred_render() const {
    if(_scene) {
        _scene->deferred_render(_camera);
    }
}

void SceneView::point_lights_render(std::shared_ptr<StaticMesh> sphere_mesh) const {
    if (_scene) {
        _scene->point_lights_render(_camera, sphere_mesh);
    }
}

void SceneView::tiled_render(glm::uvec2 window_size, size_t tile_size) const {
    if (_scene) {
        _scene->tiled_render(_camera, window_size, tile_size);
    }    
}

}
