#pragma once
#include <imkit/preview.h>

namespace imkit::preview {

// OpenGL 3.3 core function pointers, supplied by the host with a current context.
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
    bool Init(const GLFunctions &functions, int width, int height);
    bool Resize(int width, int height);
    bool Render(std::span<const Mesh> meshes, const cg::Camera &camera,
                ImVec4 background = {.06f, .07f, .09f, 1});
    editor::StableId Pick(int x, int y);
    void Shutdown();
    [[nodiscard]] unsigned Texture() const { return color_; }
    [[nodiscard]] bool Initialized() const { return program_ != 0; }
    [[nodiscard]] int Width() const { return width_; }
    [[nodiscard]] int Height() const { return height_; }

  private:
    GLFunctions gl_{};
    unsigned framebuffer_ = 0, color_ = 0, depth_ = 0, picking_ = 0, program_ = 0, vao_ = 0, vbo_ = 0,
             ibo_ = 0;
    int width_ = 0, height_ = 0, matrixLocation_ = -1, idLocation_ = -1;
};

} // namespace imkit::preview
