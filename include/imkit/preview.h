#pragma once
#include <imkit/cg.h>
namespace imkit::preview {
struct Vertex {
    float position[3]{}, normal[3]{0, 1, 0}, color[4]{1, 1, 1, 1};
};
struct Mesh {
    editor::StableId id = 0;
    std::span<const Vertex> vertices;
    std::span<const std::uint32_t> indices;
    cg::Transform transform{};
    bool wire = false;
};
struct Triangle {
    ImVec2 points[3];
    double depth = 0;
    ImU32 color = 0;
    bool wire = false;
};
// Returns required triangle count; writes only within host scratch capacity.
std::size_t DrawListPreview(ImDrawList &draw, std::span<const Mesh> meshes, const cg::Camera &camera,
                            ImVec2 origin, ImVec2 size, std::span<Triangle> scratch);
struct NormalOverlayOptions {
    bool vertices=true,faces=false;
    double length=.2; // World-space length after normalization.
    ImU32 color=IM_COL32(80,180,240,255);
};
// DrawList overlay, without depth occlusion. Returns drawn normal segments; inputs remain borrowed.
std::size_t DrawMeshNormals(ImDrawList &draw,std::span<const Mesh> meshes,const cg::Camera &camera,
                           ImVec2 origin,ImVec2 size,NormalOverlayOptions options={});
bool Cube(std::span<Vertex> vertices, std::span<std::uint32_t> indices); // 24 vertices, 36 indices.
bool Sphere(std::span<Vertex> vertices, std::span<std::uint32_t> indices, int slices = 24, int rings = 12);
// OpenGL 3.3 core function pointers, supplied by the host with a current context.
// On Windows x64 OpenGL APIENTRY uses the platform's single calling convention.
struct GLFunctions {
    void (*GenFramebuffers)(int, unsigned *) = nullptr;
    void (*DeleteFramebuffers)(int, const unsigned *) = nullptr;
    void (*BindFramebuffer)(unsigned, unsigned) = nullptr;
    unsigned (*CheckFramebufferStatus)(unsigned) = nullptr;
    void (*FramebufferTexture2D)(unsigned, unsigned, unsigned, unsigned, int) = nullptr;
    void (*DrawBuffers)(int, const unsigned *) = nullptr;
    void (*ReadBuffer)(unsigned) = nullptr;
    void (*ReadPixels)(int, int, int, int, unsigned, unsigned, void *) = nullptr;
    void (*GenTextures)(int, unsigned *) = nullptr;
    void (*DeleteTextures)(int, const unsigned *) = nullptr;
    void (*BindTexture)(unsigned, unsigned) = nullptr;
    void (*TexImage2D)(unsigned, int, int, int, int, int, unsigned, unsigned, const void *) = nullptr;
    void (*TexParameteri)(unsigned, unsigned, int) = nullptr;
    void (*GenVertexArrays)(int, unsigned *) = nullptr;
    void (*DeleteVertexArrays)(int, const unsigned *) = nullptr;
    void (*BindVertexArray)(unsigned) = nullptr;
    void (*GenBuffers)(int, unsigned *) = nullptr;
    void (*DeleteBuffers)(int, const unsigned *) = nullptr;
    void (*BindBuffer)(unsigned, unsigned) = nullptr;
    void (*BufferData)(unsigned, std::ptrdiff_t, const void *, unsigned) = nullptr;
    unsigned (*CreateShader)(unsigned) = nullptr;
    void (*ShaderSource)(unsigned, int, const char *const *, const int *) = nullptr;
    void (*CompileShader)(unsigned) = nullptr;
    void (*GetShaderiv)(unsigned, unsigned, int *) = nullptr;
    void (*DeleteShader)(unsigned) = nullptr;
    unsigned (*CreateProgram)() = nullptr;
    void (*AttachShader)(unsigned, unsigned) = nullptr;
    void (*LinkProgram)(unsigned) = nullptr;
    void (*GetProgramiv)(unsigned, unsigned, int *) = nullptr;
    void (*DeleteProgram)(unsigned) = nullptr;
    void (*UseProgram)(unsigned) = nullptr;
    int (*GetUniformLocation)(unsigned, const char *) = nullptr;
    void (*UniformMatrix4fv)(int, int, unsigned char, const float *) = nullptr;
    void (*Uniform2ui)(int, unsigned, unsigned) = nullptr;
    void (*EnableVertexAttribArray)(unsigned) = nullptr;
    void (*VertexAttribPointer)(unsigned, int, unsigned, unsigned char, int, const void *) = nullptr;
    void (*DrawElements)(unsigned, int, unsigned, const void *) = nullptr;
    void (*Viewport)(int, int, int, int) = nullptr;
    void (*ClearBufferfv)(unsigned, int, const float *) = nullptr;
    void (*ClearBufferuiv)(unsigned, int, const unsigned *) = nullptr;
    void (*Enable)(unsigned) = nullptr;
    void (*Disable)(unsigned) = nullptr;
    void (*DepthFunc)(unsigned) = nullptr;
    void (*DepthMask)(unsigned char) = nullptr;
    void (*PolygonMode)(unsigned, unsigned) = nullptr;
};
class OpenGL3Renderer {
  public:
    OpenGL3Renderer() = default;
    OpenGL3Renderer(const OpenGL3Renderer &) = delete;
    OpenGL3Renderer &operator=(const OpenGL3Renderer &) = delete;
    // Explicit Shutdown is mandatory while the original GL context is current.
    // Destructor intentionally makes no GL calls.
    bool Init(const GLFunctions &functions, int width, int height);
    bool Resize(int width, int height);
    bool Render(std::span<const Mesh> meshes, const cg::Camera &camera,
                ImVec4 background = {.06f, .07f, .09f, 1});
    editor::StableId Pick(int x, int y); // Top-left pixel coordinates.
    void Shutdown();
    unsigned Texture() const {
        return color_;
    }
    bool Initialized() const {
        return program_ != 0;
    }
    int Width() const {
        return width_;
    }
    int Height() const {
        return height_;
    }

  private:
    GLFunctions gl_{};
    unsigned framebuffer_ = 0, color_ = 0, depth_ = 0, picking_ = 0, program_ = 0, vao_ = 0, vbo_ = 0,
             ibo_ = 0;
    int width_ = 0, height_ = 0, matrixLocation_ = -1, idLocation_ = -1;
};
} // namespace imkit::preview
