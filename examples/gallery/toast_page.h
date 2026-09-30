#pragma once
#include <imkit/toast.h>
#include <vector>
namespace imkit::gallery {
struct GalleryState;
struct ToastPage {
    ToastViewportState state;
    ToastViewportOptions layout;
    std::vector<ToastView> items;
    StableId nextId=0;
    int position=2, maximum=4, actionCount=0;
    bool japanese=false, initialized=false, indeterminate=false;
    float progress=.67f;
    double duration=-1;
    void Show(GalleryState& host);
};
}
