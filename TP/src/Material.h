#ifndef MATERIAL_H
#define MATERIAL_H

#include <glad/glad.h>

#include <Program.h>
#include <Texture.h>

#include <memory>
#include <vector>

namespace OM3D {

enum class BlendMode {
    None,
    Alpha,
    Additive,
};

enum class CullMode {
    None, 
    Backface,
    Frontface,
};

enum class DepthTestMode {
    Standard,
    Reversed,
    Equal,
    None
};

class Material {

    public:
        Material();

        void set_program(std::shared_ptr<Program> prog);
        void set_blend_mode(BlendMode blend);
        void set_cull_mode(CullMode cull);
        void set_depth_test_mode(DepthTestMode depth);
        void set_depth_mask(GLboolean mask);
        void set_texture(u32 slot, std::shared_ptr<Texture> tex);
        bool is_transparent();

        template<typename... Args>
        void set_uniform(Args&&... args) {
            _program->set_uniform(FWD(args)...);
        }


        void bind(CullMode force_cullmode = CullMode::None) const;

        static std::shared_ptr<Material> empty_material();
        static Material textured_material();
        static Material textured_normal_mapped_material();


    private:
        std::shared_ptr<Program> _program;
        std::vector<std::pair<u32, std::shared_ptr<Texture>>> _textures;

        BlendMode _blend_mode = BlendMode::None;
        DepthTestMode _depth_test_mode = DepthTestMode::Standard;
        CullMode _culling_mode = CullMode::Backface; 
        GLboolean _depth_mask = GL_TRUE;

};

}

#endif // MATERIAL_H
