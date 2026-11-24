#pragma once
#include <windows.h>
#include <GLFW/glfw3.h>
#include "gallery.h"
#include <imkit/window_frame_win32.h>
#include "../design_gallery/motion_capture.h"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace imkit::gallery::host {
struct IconArtworkTile {
    std::string name;
    GLuint texture = 0;
};
struct IconArtworkSlide {
    std::string category;
    std::string description;
    int presetCount = 0;
    int sourceImageCount = 0;
    std::vector<IconArtworkTile> tiles;
};
struct Host {
    GLFWwindow *window = nullptr;
    imkit::gallery::GalleryState s;
    imkit::WindowFrameWin32Adapter windowFrame;
    std::string windowTitle = "ImKit Precision Layers";
    bool automated = false;
    ImGuiMemAllocFunc originalAlloc = nullptr;
    ImGuiMemFreeFunc originalFree = nullptr;
    void *originalAllocatorUser = nullptr;
    bool countImGuiAllocations = false;
    std::size_t imguiAllocations = 0;
    ImVec2 mouse{-100, -100};
    bool iconArtworkShowcase = false;
    int iconArtworkSlide = 0;
    int iconArtworkSourceCount = 0;
    int iconArtworkSourceCategoryCount = 0;
    int iconArtworkCatalogCategoryCount = 0;
    std::vector<IconArtworkSlide> iconArtworkSlides;
    imkit::design::MotionCapture *motion = nullptr;

    void Frame(const std::function<void(ImGuiIO &)> &input = {}, const std::filesystem::path &shot = {});
    void Settle(int n = 5);
    void Page(int page);
    void ClickAt(ImVec2 pos);
    ImVec2 Reveal(const char *name);
    void Click(const char *name);
    void Key(ImGuiKey key);
    void Replace(const char *probe, const char *text);
    void ReplaceActive(const char *text);
};

void DrawIconArtworkShowcase(Host &h);
void LoadIconArtworkShowcase(Host &h);
void DestroyIconArtworkShowcase(Host &h);

void Verify(Host &h, const std::filesystem::path &out);
void VerifyIcons(Host &h, const std::filesystem::path &out);
void VerifyEditors(Host &h, const std::filesystem::path &out, const imkit::preview::GLFunctions &functions);
void BenchmarkEditors(Host &h, const std::filesystem::path &out);
void VerifyLinkedClips(Host &h, const std::filesystem::path &out);
void VerifyTimelineUI(Host &h, const std::filesystem::path &out);
void VerifyComparison(Host &h, const std::filesystem::path &out);
void VerifyTrackControls(Host &h, const std::filesystem::path &out);
void VerifyMonitors(Host &h, const std::filesystem::path &out);
void VerifyColor(Host &h, const std::filesystem::path &out);
void VerifyWindowFrame(Host &h, const std::filesystem::path &out);

bool HasMotionScene(const std::string &name);
void CaptureFrames(Host &h, const std::filesystem::path &dir, int &index, int count);
void CaptureMotion(Host &h, const std::filesystem::path &out, const std::string &demo);
void CaptureDemo(Host &h, const std::filesystem::path &out, const std::string &demo, bool legacyReadme);

int VerifyTimelineModel();
int VerifyInspectorModel();
} // namespace imkit::gallery::host
