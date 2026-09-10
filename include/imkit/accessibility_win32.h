#pragma once
#include <imkit/accessibility.h>
#ifdef _WIN32
#include <windows.h>
#include <objbase.h>
#include <UIAutomation.h>
namespace imkit::accessibility {
// Host calls CoInitializeEx/CoUninitialize and handles WM_GETOBJECT. The returned
// COM provider holds a COPY of this frame. Release the initial reference after
// UiaReturnRawElementProvider. Native clients may retain their own references.
// dispatch must be thread-safe and outlive all providers (or be invalidated by host).
struct Win32ActionSink {
    void* user=nullptr;
    HRESULT (*dispatch)(void*, StableId, SemanticAction)=nullptr;
};
HRESULT CreateWin32Provider(HWND window, const AccessibilityTree& tree,
                           Win32ActionSink actions, IRawElementProviderFragmentRoot** result);
}
#endif
