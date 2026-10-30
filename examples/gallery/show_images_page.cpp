#include "show_pages.h"
#include "gallery.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace imkit::gallery {
using namespace show;

void ShowPages::Images(GalleryState& host) {
    ComponentOptions o{&host.theme, &host.animation};
    auto& s = images;
    const float u = ImGui::GetFrameHeight();
    const float time = static_cast<float>(ImGui::GetTime());
    if (!s.initialized) {
        s.initialized = true;
        for (std::size_t i = 0; i < s.views.size(); ++i) {
            s.views[i].image = host.sampleImages[i];
            s.views[i].imagePixels = {96, 96};
            s.views[i].label = s.names[i];
        }
        s.views[3].shape = ImageShape::Circle;
        s.views[4].badge = 3;
        s.views[5].fit = ImageFit::Contain;
    }
    auto assign = [&](std::size_t index, std::size_t picture) {
        s.views[index].image = host.sampleImages[picture % host.sampleImages.size()];
        s.views[index].imagePixels = {96, 96};
        s.views[index].crop = {};
        s.assigned[index] = true;
    };

    if (BeginGrid(host, "show-images-grid", 2)) {
        const bool section1 = BeginSection(host, "Picture buttons", "Momentary, toggle, circular, badge and progress, label below. Right-click for the edit menu.");
        if (section1) {
            const ConsoleMode modes[6] = {ConsoleMode::Momentary, ConsoleMode::Toggle, ConsoleMode::Flash, ConsoleMode::Momentary, ConsoleMode::Momentary, ConsoleMode::Toggle};
            for (std::size_t i = 0; i < s.views.size(); ++i) {
                if (i > 0 && i % 3 != 0)
                    ImGui::SameLine();
                ImageButtonView view = s.views[i];
                if (!s.assigned[i])
                    view.image = {};
                view.on = s.on[i];
                if (i == 4)
                    view.progress = std::fmod(time * .2f, 1.f);
                ImageButtonOptions io;
                io.size = {u * 6.f, u * 4.2f};
                io.mode = modes[i];
                io.editable = true;
                io.dropPayload = "IMKIT_IMAGE";
                io.labelStyle = i == 5 ? ImageLabel::Below : ImageLabel::Overlay;
                const ImageButtonRequest r = ImageButton(s.names[i], view, io, o);
                Record(host, (std::string("image-button-") + std::to_string(i)).c_str());
                if (r.changed)
                    s.on[i] = !s.on[i];
                if (r.pressed) {
                    ++s.presses[i];
                    s.last = std::string(s.names[i]) + " pressed";
                }
                if (r.chooseImage) {
                    assign(i, i + 1 + static_cast<std::size_t>(s.presses[i]));
                    s.last = std::string(s.names[i]) + ": picture chosen";
                }
                if (r.clearImage) {
                    s.assigned[i] = false;
                    s.last = std::string(s.names[i]) + ": picture removed";
                }
                if (r.adjustImage) {
                    s.adjusting = static_cast<int>(i);
                    s.crop = s.views[i].crop;
                }
                if (r.payload && r.payload->DataSize == sizeof(int)) {
                    assign(i, static_cast<std::size_t>(*static_cast<const int*>(r.payload->Data)));
                    s.last = std::string(s.names[i]) + ": picture dropped";
                }
            }
            ImGui::TextDisabled("Presses: %d %d %d %d %d %d   last: %s", s.presses[0], s.presses[1], s.presses[2], s.presses[3], s.presses[4], s.presses[5], s.last.c_str());
        }
        EndSection(section1);

        const bool section2 = BeginSection(host, "Assign a picture", "Drag a picture onto a button. Choosing a file is the host's job; here the menu cycles the sample pictures.");
        if (section2) {
            for (int k = 0; k < static_cast<int>(host.sampleImages.size()); ++k) {
                if (k > 0)
                    ImGui::SameLine();
                ImGui::PushID(k);
                ImGui::Image(host.sampleImages[static_cast<std::size_t>(k)], {u * 2.2f, u * 2.2f});
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("IMKIT_IMAGE", &k, sizeof(k));
                    ImGui::Image(host.sampleImages[static_cast<std::size_t>(k)], {u * 2.2f, u * 2.2f});
                    ImGui::EndDragDropSource();
                }
                ImGui::PopID();
            }
            ImGui::Spacing();
            ImGui::TextDisabled("Empty buttons show a call to action and highlight while a picture is dragged over them.");
        }
        EndSection(section2);

        const bool section3 = BeginSection(host, "Reposition", "Drag to pan, scroll or use the slider to zoom. Pick \"Adjust position...\" in a button's menu.");
        if (section3) {
            if (s.adjusting >= 0) {
                ImageButtonView view = s.views[static_cast<std::size_t>(s.adjusting)];
                view.crop = s.crop;
                view.fit = ImageFit::Cover;
                const ValueEdit e = ImageCropEditor("crop", view, &s.crop, {u * 9.f, u * 9.f}, o);
                Record(host, "image-crop");
                if (e.changed || e.finished)
                    s.views[static_cast<std::size_t>(s.adjusting)].crop = s.crop;
                if (ImGui::SmallButton("Done"))
                    s.adjusting = -1;
            } else {
                ImGui::TextDisabled("No button selected for adjustment.");
                if (ImGui::SmallButton("Adjust the first button")) {
                    s.adjusting = 0;
                    s.crop = s.views[0].crop;
                }
            }
        }
        EndSection(section3);

        const bool section4 = BeginSection(host, "Fit and cover", "Cover fills and crops, Contain letterboxes, Stretch ignores the aspect ratio.");
        if (section4) {
            const ImageFit fits[3] = {ImageFit::Cover, ImageFit::Contain, ImageFit::Stretch};
            const char* names[3] = {"Cover", "Contain", "Stretch"};
            for (int i = 0; i < 3; ++i) {
                if (i > 0)
                    ImGui::SameLine();
                ImageButtonView view;
                view.image = host.sampleImages[static_cast<std::size_t>(i)];
                view.imagePixels = {96, 48}; // Pretend the picture is wide, so the fit modes differ.
                view.fit = fits[i];
                view.label = names[i];
                ImageButtonOptions io;
                io.size = {u * 6.f, u * 4.f};
                ImageButton(names[i], view, io, o);
            }
        }
        EndSection(section4);
        EndGrid();
    }
}
} // namespace imkit::gallery
