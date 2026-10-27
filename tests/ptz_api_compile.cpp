#include <imkit/imkit.h>
imkit::PtzRequest (*volatile ptz)(const char*, const imkit::PtzView&, imkit::PtzState&, imkit::PtzOptions, imkit::ComponentOptions) = &imkit::PtzControl;
ImVec2 (*volatile padVector)(ImVec2, ImVec2, float, float) = &imkit::PtzPadVector;
bool (*volatile range)(const char*, float*, float*, float, float, const char*, imkit::RangeSliderOptions, imkit::ComponentOptions) = &imkit::RangeSlider;
bool (*volatile rangeInt)(const char*, int*, int*, int, int, const char*, imkit::RangeSliderOptions, imkit::ComponentOptions) = &imkit::RangeSliderInt;
int main() {
    imkit::PtzPreset preset;
    imkit::PtzRequest request;
    imkit::RangeSliderOptions options;
    return !preset.stored || request.recallPreset != -1 || options.step != 0;
}
