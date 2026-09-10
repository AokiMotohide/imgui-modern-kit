#include <imkit/preview.h>
#include <cmath>
#include <algorithm>
namespace imkit::preview {
namespace {
constexpr unsigned Framebuffer = 0x8D40, Texture2D = 0x0DE1, Color0 = 0x8CE0, Color1 = 0x8CE1,
                   DepthAttachment = 0x8D00;
bool Complete(const GLFunctions &g) {
    return g.GenFramebuffers && g.DeleteFramebuffers && g.BindFramebuffer && g.CheckFramebufferStatus &&
           g.FramebufferTexture2D && g.DrawBuffers && g.ReadBuffer && g.ReadPixels && g.GenTextures &&
           g.DeleteTextures && g.BindTexture && g.TexImage2D && g.TexParameteri && g.GenVertexArrays &&
           g.DeleteVertexArrays && g.BindVertexArray && g.GenBuffers && g.DeleteBuffers && g.BindBuffer &&
           g.BufferData && g.CreateShader && g.ShaderSource && g.CompileShader && g.GetShaderiv &&
           g.DeleteShader && g.CreateProgram && g.AttachShader && g.LinkProgram && g.GetProgramiv &&
           g.DeleteProgram && g.UseProgram && g.GetUniformLocation && g.UniformMatrix4fv && g.Uniform2ui &&
           g.EnableVertexAttribArray && g.VertexAttribPointer && g.DrawElements && g.Viewport &&
           g.ClearBufferfv && g.ClearBufferuiv && g.Enable && g.Disable && g.DepthFunc && g.DepthMask &&
           g.PolygonMode;
}
void Multiply(const float *a, const float *b, float *out) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) {
            out[c * 4 + r] = 0;
            for (int k = 0; k < 4; ++k)
                out[c * 4 + r] += a[k * 4 + r] * b[c * 4 + k];
        }
}
void Matrix(const cg::Camera &c, const cg::Transform &t, float aspect, float *result, float *normal) {
    const auto basis=cg::OrientationBasis(cg::Orientation::View,{},c);
    const auto x=basis.x,y=basis.y,z=basis.z;
    float view[16]={static_cast<float>(x.x),static_cast<float>(y.x),static_cast<float>(z.x),0,
                    static_cast<float>(x.y),static_cast<float>(y.y),static_cast<float>(z.y),0,
                    static_cast<float>(x.z),static_cast<float>(y.z),static_cast<float>(z.z),0,0,0,0,1};
    view[12]=-static_cast<float>(c.target.x*x.x+c.target.y*x.y+c.target.z*x.z);
    view[13]=-static_cast<float>(c.target.x*y.x+c.target.y*y.y+c.target.z*y.z);
    view[14]=-static_cast<float>(c.distance+c.target.x*z.x+c.target.y*z.y+c.target.z*z.z);
    float projection[16]{};
    float near = .01f, far = 10000;
    if (c.projection == cg::Projection::Orthographic) {
        projection[0] = 2 / static_cast<float>(c.orthographicHeight) / aspect;
        projection[5] = 2 / static_cast<float>(c.orthographicHeight);
        projection[10] = -2 / (far - near);
        projection[14] = -(far + near) / (far - near);
        projection[15] = 1;
    } else {
        projection[5] = 1 / static_cast<float>(std::tan(c.verticalFov * .5));
        projection[0] = projection[5] / aspect;
        projection[10] = -(far + near) / (far - near);
        projection[11] = -1;
        projection[14] = -2 * far * near / (far - near);
    }
    const auto linear=cg::LinearBasis(t),normals=cg::NormalBasis(t);
    float model[16]{};std::fill(normal,normal+16,0.f);model[15]=normal[15]=1;
    const cg::Vec3 columns[]{linear.x,linear.y,linear.z},normalColumns[]{normals.x,normals.y,normals.z};
    for (int column=0;column<3;++column) {
        const auto &v=columns[column],&n=normalColumns[column];
        model[column*4]=static_cast<float>(v.x);model[column*4+1]=static_cast<float>(v.y);model[column*4+2]=static_cast<float>(v.z);
        normal[column*4]=static_cast<float>(n.x);normal[column*4+1]=static_cast<float>(n.y);normal[column*4+2]=static_cast<float>(n.z);
    }
    model[12]=static_cast<float>(t.translation.x);model[13]=static_cast<float>(t.translation.y);model[14]=static_cast<float>(t.translation.z);
    float vp[16];
    Multiply(projection, view, vp);
    Multiply(vp, model, result);
}
} // namespace
bool OpenGL3Renderer::Init(const GLFunctions &functions, int width, int height) {
    if (Initialized() || !Complete(functions) || width <= 0 || height <= 0)
        return false;
    gl_ = functions;
    const char *vs = "#version 330 core\nlayout(location=0) in vec3 p;layout(location=1) in vec3 "
                     "n;layout(location=2) in vec4 c;uniform mat4 mvp;uniform mat4 normalMatrix;out vec4 color;void "
                     "main(){gl_Position=mvp*vec4(p,1);vec3 wn=(normalMatrix*vec4(n,0)).xyz;"
                     "wn=length(wn)>0.?normalize(wn):vec3(0);color=vec4(c.rgb*(.25+.75*max(0.,dot(wn,"
                     "normalize(vec3(.3,.8,.5))))),c.a);}";
    const char *fs = "#version 330 core\nin vec4 color;uniform uvec2 objectId;layout(location=0) out vec4 "
                     "frag;layout(location=1) out uvec2 pick;void main(){frag=color;pick=objectId;}";
    unsigned shaders[2] = {gl_.CreateShader(0x8B31), gl_.CreateShader(0x8B30)};
    const char *sources[] = {vs, fs};
    bool ok = true;
    for (int i = 0; i < 2; ++i) {
        gl_.ShaderSource(shaders[i], 1, &sources[i], nullptr);
        gl_.CompileShader(shaders[i]);
        int compiled = 0;
        gl_.GetShaderiv(shaders[i], 0x8B81, &compiled);
        ok &= compiled != 0;
    }
    if (ok) {
        program_ = gl_.CreateProgram();
        for (auto shader : shaders)
            gl_.AttachShader(program_, shader);
        gl_.LinkProgram(program_);
        int linked = 0;
        gl_.GetProgramiv(program_, 0x8B82, &linked);
        ok = linked != 0;
    }
    for (auto shader : shaders)
        gl_.DeleteShader(shader);
    if (!ok) {
        Shutdown();
        return false;
    }
    matrixLocation_ = gl_.GetUniformLocation(program_, "mvp");
    idLocation_ = gl_.GetUniformLocation(program_, "objectId");
    gl_.GenVertexArrays(1, &vao_);
    gl_.GenBuffers(1, &vbo_);
    gl_.GenBuffers(1, &ibo_);
    gl_.GenFramebuffers(1, &framebuffer_);
    gl_.BindVertexArray(vao_);
    gl_.BindBuffer(0x8892, vbo_);
    gl_.BindBuffer(0x8893, ibo_);
    for (unsigned i = 0; i < 3; ++i) {
        gl_.EnableVertexAttribArray(i);
        gl_.VertexAttribPointer(i, i == 2 ? 4 : 3, 0x1406, 0, sizeof(Vertex),
                                reinterpret_cast<const void *>(static_cast<std::uintptr_t>(i == 0   ? 0
                                                                                           : i == 1 ? 12
                                                                                                    : 24)));
    }
    gl_.BindVertexArray(0);
    if (!Resize(width, height)) {
        Shutdown();
        return false;
    }
    return true;
}
bool OpenGL3Renderer::Resize(int width, int height) {
    if (!Initialized() || width <= 0 || height <= 0)
        return false;
    if (width == width_ && height == height_)
        return true;
    // ImGui draw commands may already refer to the color texture this frame.
    // Reallocate storage without invalidating the borrowed texture name.
    unsigned textures[3]{color_, depth_, picking_};
    if (!color_) gl_.GenTextures(3, textures);
    color_ = textures[0];
    depth_ = textures[1];
    picking_ = textures[2];
    gl_.BindFramebuffer(Framebuffer, framebuffer_);
    for (int i = 0; i < 3; ++i) {
        gl_.BindTexture(Texture2D, textures[i]);
        gl_.TexParameteri(Texture2D, 0x2801, 0x2600);
        gl_.TexParameteri(Texture2D, 0x2800, 0x2600);
        gl_.TexParameteri(Texture2D, 0x2802, 0x812F);
        gl_.TexParameteri(Texture2D, 0x2803, 0x812F);
        gl_.TexImage2D(Texture2D, 0,
                       i == 0   ? 0x8058
                       : i == 1 ? 0x81A6
                                : 0x823C,
                       width, height, 0,
                       i == 0   ? 0x1908
                       : i == 1 ? 0x1902
                                : 0x8228,
                       i == 0 ? 0x1401 : 0x1405, nullptr);
        gl_.FramebufferTexture2D(Framebuffer,
                                 i == 0   ? Color0
                                 : i == 1 ? DepthAttachment
                                          : Color1,
                                 Texture2D, textures[i], 0);
    }
    const unsigned attachments[] = {Color0, Color1};
    gl_.DrawBuffers(2, attachments);
    bool ok = gl_.CheckFramebufferStatus(Framebuffer) == 0x8CD5;
    gl_.BindFramebuffer(Framebuffer, 0);
    width_ = ok ? width : 0;
    height_ = ok ? height : 0;
    return ok;
}
bool OpenGL3Renderer::Render(std::span<const Mesh> meshes, const cg::Camera &camera, ImVec4 background) {
    if (!Initialized() || width_ <= 0 || height_ <= 0)
        return false;
    gl_.BindFramebuffer(Framebuffer, framebuffer_);
    gl_.Viewport(0, 0, width_, height_);
    gl_.Disable(0x0C11);
    gl_.Disable(0x0BE2);
    gl_.Disable(0x0B44);
    gl_.Enable(0x0B71);
    gl_.DepthFunc(0x0201);
    gl_.DepthMask(1);
    float depth = 1;
    unsigned clear[4]{};
    gl_.ClearBufferfv(0x1800, 0, &background.x);
    gl_.ClearBufferuiv(0x1800, 1, clear);
    gl_.ClearBufferfv(0x1801, 0, &depth);
    gl_.UseProgram(program_);
    gl_.BindVertexArray(vao_);
    const int normalLocation = gl_.GetUniformLocation(program_, "normalMatrix");
    for (const auto &mesh : meshes) {
        if (mesh.vertices.empty() || mesh.indices.empty())
            continue;
        if (std::any_of(mesh.indices.begin(), mesh.indices.end(),
                        [&](auto index) { return index >= mesh.vertices.size(); }))
            continue;
        float matrix[16], normal[16];
        Matrix(camera, mesh.transform, static_cast<float>(width_) / height_, matrix, normal);
        gl_.UniformMatrix4fv(matrixLocation_, 1, 0, matrix);
        gl_.UniformMatrix4fv(normalLocation, 1, 0, normal);
        gl_.Uniform2ui(idLocation_, static_cast<unsigned>(mesh.id), static_cast<unsigned>(mesh.id >> 32));
        gl_.BindBuffer(0x8892, vbo_);
        gl_.BufferData(0x8892, mesh.vertices.size_bytes(), mesh.vertices.data(), 0x88E0);
        gl_.BindBuffer(0x8893, ibo_);
        gl_.BufferData(0x8893, mesh.indices.size_bytes(), mesh.indices.data(), 0x88E0);
        gl_.PolygonMode(0x0408, mesh.wire ? 0x1B01 : 0x1B02);
        gl_.DrawElements(0x0004, static_cast<int>(mesh.indices.size()), 0x1405, nullptr);
    }
    gl_.PolygonMode(0x0408, 0x1B02);
    gl_.BindVertexArray(0);
    gl_.UseProgram(0);
    gl_.BindFramebuffer(Framebuffer, 0);
    return true;
}
editor::StableId OpenGL3Renderer::Pick(int x, int y) {
    if (!Initialized() || x < 0 || y < 0 || x >= width_ || y >= height_)
        return 0;
    unsigned id[2]{};
    gl_.BindFramebuffer(Framebuffer, framebuffer_);
    gl_.ReadBuffer(Color1);
    gl_.ReadPixels(x, height_ - 1 - y, 1, 1, 0x8228, 0x1405, id);
    gl_.ReadBuffer(Color0);
    gl_.BindFramebuffer(Framebuffer, 0);
    return static_cast<editor::StableId>(id[0]) | (static_cast<editor::StableId>(id[1]) << 32);
}
void OpenGL3Renderer::Shutdown() {
    if (gl_.DeleteTextures) {
        unsigned textures[] = {color_, depth_, picking_};
        gl_.DeleteTextures(3, textures);
    }
    if (framebuffer_)
        gl_.DeleteFramebuffers(1, &framebuffer_);
    if (vao_)
        gl_.DeleteVertexArrays(1, &vao_);
    if (vbo_)
        gl_.DeleteBuffers(1, &vbo_);
    if (ibo_)
        gl_.DeleteBuffers(1, &ibo_);
    if (program_)
        gl_.DeleteProgram(program_);
    framebuffer_ = color_ = depth_ = picking_ = vao_ = vbo_ = ibo_ = program_ = 0;
    width_ = height_ = 0;
    gl_ = {};
}
} // namespace imkit::preview
