#include <imkit/accessibility_win32.h>
#include <array>
#include <cstdio>
int main() {
    using namespace imkit::accessibility;
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    if(FAILED(initialized)) return 1;
    HWND window=CreateWindowExW(0,L"STATIC",L"UIA fixture",WS_OVERLAPPEDWINDOW,0,0,640,480,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    SemanticNode n; n.id=1; n.name="Save"; n.role=SemanticRole::Button; n.actions=SemanticAction::Press;
    std::array<SemanticNode,1> nodes{n}; int calls=0;
    Win32ActionSink sink{&calls,[](void* p,StableId id,SemanticAction action)->HRESULT { if(id!=1 || action!=SemanticAction::Press) return E_INVALIDARG; ++*static_cast<int*>(p); return S_OK; }};
    IRawElementProviderFragmentRoot* root=nullptr;
    bool ok=SUCCEEDED(CreateWin32Provider(window,{nodes,1,false},sink,&root));
    IRawElementProviderFragment* fragment=nullptr;
    if(ok) ok=SUCCEEDED(root->QueryInterface(IID_PPV_ARGS(&fragment)));
    IRawElementProviderFragment* child=nullptr;
    if(ok) ok=SUCCEEDED(fragment->Navigate(NavigateDirection_FirstChild,&child)) && child;
    IRawElementProviderSimple* simple=nullptr;
    if(ok) ok=SUCCEEDED(child->QueryInterface(IID_PPV_ARGS(&simple)));
    VARIANT value; VariantInit(&value);
    if(ok) ok=SUCCEEDED(simple->GetPropertyValue(UIA_NamePropertyId,&value)) && value.vt==VT_BSTR && wcscmp(value.bstrVal,L"Save")==0;
    VariantClear(&value); IInvokeProvider* invoke=nullptr;
    if(ok) ok=SUCCEEDED(child->QueryInterface(IID_PPV_ARGS(&invoke))) && SUCCEEDED(invoke->Invoke()) && calls==1;
    if(invoke) invoke->Release(); if(simple) simple->Release(); if(child) child->Release(); if(fragment) fragment->Release(); if(root) root->Release();
    DestroyWindow(window); CoUninitialize(); std::printf("UIA representative tree %s\n",ok?"PASS":"FAIL"); return ok?0:1;
}
