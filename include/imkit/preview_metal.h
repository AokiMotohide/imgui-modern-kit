#pragma once
#include <imkit/preview.h>

namespace imkit::preview {

// Borrows an MTLDevice and one command buffer per render call. The host owns
// command submission and synchronization. Objective-C types stay out of this header.
class MetalRenderer {
  public:
    MetalRenderer() = default;
    ~MetalRenderer() = default;
    MetalRenderer(const MetalRenderer &) = delete;
    MetalRenderer &operator=(const MetalRenderer &) = delete;

    bool Init(void *mtlDevice, int width, int height);
    bool Resize(int width, int height);
    bool Render(std::span<const Mesh> meshes, const cg::Camera &camera, void *mtlCommandBuffer,
                ImVec4 background = {.06f, .07f, .09f, 1});
    // Reads the most recently completed frame. The host must synchronize its command buffer first.
    editor::StableId Pick(int x, int y) const;
    void Shutdown();

    [[nodiscard]] void *Texture() const noexcept { return color_; }
    [[nodiscard]] bool Initialized() const noexcept { return pipeline_ != nullptr; }
    [[nodiscard]] int Width() const noexcept { return width_; }
    [[nodiscard]] int Height() const noexcept { return height_; }

  private:
    void *device_ = nullptr;
    void *pipeline_ = nullptr;
    void *depthState_ = nullptr;
    void *color_ = nullptr;
    void *depth_ = nullptr;
    void *picking_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};

} // namespace imkit::preview
