#include <imkit/accessibility_win32.h>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <new>

namespace imkit::accessibility {
namespace {
std::wstring Wide(std::string_view text) {
    if(text.empty()) return {};
    int size=MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),nullptr,0);
    std::wstring result(size,L'\0');
    MultiByteToWideChar(CP_UTF8,0,text.data(),static_cast<int>(text.size()),result.data(),size); return result;
}
struct OwnedNode { SemanticNode node; std::wstring name,description,value; };
struct Snapshot { HWND window; Win32ActionSink actions; std::vector<OwnedNode> nodes; };
CONTROLTYPEID Type(SemanticRole role) {
    switch(role) {
    case SemanticRole::Button:return UIA_ButtonControlTypeId;
    case SemanticRole::Toggle:return UIA_CheckBoxControlTypeId;
    case SemanticRole::Radio:return UIA_RadioButtonControlTypeId;
    case SemanticRole::ComboBox:return UIA_ComboBoxControlTypeId;
    case SemanticRole::TextField:return UIA_EditControlTypeId;
    case SemanticRole::Toolbar:return UIA_ToolBarControlTypeId;
    case SemanticRole::Tab:return UIA_TabItemControlTypeId;
    case SemanticRole::TabList:return UIA_TabControlTypeId;
    case SemanticRole::Tree:return UIA_TreeControlTypeId;
    case SemanticRole::TreeItem:return UIA_TreeItemControlTypeId;
    case SemanticRole::Grid:return UIA_DataGridControlTypeId;
    case SemanticRole::Row:case SemanticRole::Cell:return UIA_DataItemControlTypeId;
    case SemanticRole::Menu:return UIA_MenuControlTypeId;
    case SemanticRole::MenuItem:return UIA_MenuItemControlTypeId;
    case SemanticRole::Progress:return UIA_ProgressBarControlTypeId;
    case SemanticRole::Dialog:return UIA_WindowControlTypeId;
    default:return UIA_GroupControlTypeId;
    }
}
class Provider final : public IRawElementProviderSimple, public IRawElementProviderFragment,
                       public IRawElementProviderFragmentRoot, public IInvokeProvider, public IToggleProvider {
    std::atomic<ULONG> references_{1};
    std::shared_ptr<Snapshot> snapshot_; int index_; // -1 is the window root.
    const OwnedNode* Node() const { return index_<0?nullptr:&snapshot_->nodes[index_]; }
    HRESULT Action(SemanticAction action) {
        auto n=Node();
        if(!n || n->node.state.disabled) return UIA_E_ELEMENTNOTENABLED;
        if(!Supports(n->node.actions,action)) return UIA_E_NOTSUPPORTED;
        return snapshot_->actions.dispatch?snapshot_->actions.dispatch(snapshot_->actions.user,n->node.id,action):UIA_E_NOTSUPPORTED;
    }
    HRESULT At(int index, IRawElementProviderFragment** result) {
        if(!result) return E_POINTER; *result=nullptr;
        auto p=new(std::nothrow) Provider(snapshot_,index); if(!p) return E_OUTOFMEMORY;
        *result=p; return S_OK;
    }
public:
    Provider(std::shared_ptr<Snapshot> snapshot,int index):snapshot_(std::move(snapshot)),index_(index) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid,void** result) override {
        if(!result) return E_POINTER; *result=nullptr;
        if(iid==__uuidof(IUnknown) || iid==__uuidof(IRawElementProviderSimple))
            *result=static_cast<IRawElementProviderSimple*>(this);
        else if(iid==__uuidof(IRawElementProviderFragment)) *result=static_cast<IRawElementProviderFragment*>(this);
        else if(index_<0 && iid==__uuidof(IRawElementProviderFragmentRoot)) *result=static_cast<IRawElementProviderFragmentRoot*>(this);
        else if(iid==__uuidof(IInvokeProvider)) *result=static_cast<IInvokeProvider*>(this);
        else if(iid==__uuidof(IToggleProvider)) *result=static_cast<IToggleProvider*>(this);
        else return E_NOINTERFACE;
        AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references_; }
    ULONG STDMETHODCALLTYPE Release() override { auto n=--references_; if(!n) delete this; return n; }
    HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* result) override {
        if(!result) return E_POINTER; *result=ProviderOptions_ServerSideProvider; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID id,IUnknown** result) override {
        if(!result) return E_POINTER; *result=nullptr; auto n=Node();
        if(!n) return S_OK;
        if(id==UIA_InvokePatternId && Supports(n->node.actions,SemanticAction::Press))
            return QueryInterface(__uuidof(IInvokeProvider),reinterpret_cast<void**>(result));
        if(id==UIA_TogglePatternId && Supports(n->node.actions,SemanticAction::Toggle))
            return QueryInterface(__uuidof(IToggleProvider),reinterpret_cast<void**>(result));
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID id,VARIANT* result) override {
        if(!result) return E_POINTER; VariantInit(result); auto n=Node();
        if(id==UIA_ControlTypePropertyId) { result->vt=VT_I4; result->lVal=n?Type(n->node.role):UIA_WindowControlTypeId; }
        else if(id==UIA_NamePropertyId || id==UIA_HelpTextPropertyId) {
            result->vt=VT_BSTR; const auto text=n?(id==UIA_NamePropertyId?n->name:n->description):L"ImKit";
            result->bstrVal=SysAllocString(text.c_str()); if(!result->bstrVal) return E_OUTOFMEMORY;
        } else if(id==UIA_IsEnabledPropertyId || id==UIA_HasKeyboardFocusPropertyId ||
                  id==UIA_IsKeyboardFocusablePropertyId || id==UIA_IsControlElementPropertyId || id==UIA_IsContentElementPropertyId) {
            bool value=true;
            if(id==UIA_IsEnabledPropertyId) value=!n || !n->node.state.disabled;
            if(id==UIA_HasKeyboardFocusPropertyId) value=n && n->node.state.focused;
            if(id==UIA_IsKeyboardFocusablePropertyId) value=n && Supports(n->node.actions,SemanticAction::Focus);
            result->vt=VT_BOOL; result->boolVal=value?VARIANT_TRUE:VARIANT_FALSE;
        }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** result) override {
        if(!result) return E_POINTER; *result=nullptr;
        return index_<0?UiaHostProviderFromHwnd(snapshot_->window,result):S_OK;
    }
    HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction,IRawElementProviderFragment** result) override {
        if(!result) return E_POINTER; *result=nullptr;
        const auto* node=Node(); auto parent=node?node->node.parent:0;
        if(direction==NavigateDirection_Parent) {
            if(!node) return S_OK;
            if(!parent) return At(-1,result);
            for(int i=0;i<static_cast<int>(snapshot_->nodes.size());++i) if(snapshot_->nodes[i].node.id==parent) return At(i,result);
        }
        bool children=direction==NavigateDirection_FirstChild || direction==NavigateDirection_LastChild;
        if(!children && !node) return S_OK;
        StableId target=children?(node?node->node.id:0):parent;
        int found=-2;
        for(int i=0;i<static_cast<int>(snapshot_->nodes.size());++i) if(snapshot_->nodes[i].node.parent==target) {
            if(direction==NavigateDirection_FirstChild) return At(i,result);
            if(direction==NavigateDirection_LastChild) found=i;
            if(direction==NavigateDirection_NextSibling && i>index_) return At(i,result);
            if(direction==NavigateDirection_PreviousSibling && i<index_) found=i;
        }
        return found!=-2?At(found,result):S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** result) override {
        if(!result) return E_POINTER; *result=nullptr; if(index_<0) return S_OK;
        auto id=Node()->node.id; int values[]={UiaAppendRuntimeId,static_cast<int>(id>>32),static_cast<int>(id)};
        *result=SafeArrayCreateVector(VT_I4,0,3); if(!*result) return E_OUTOFMEMORY;
        for(LONG i=0;i<3;++i) SafeArrayPutElement(*result,&i,&values[i]); return S_OK;
    }
    HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* result) override {
        if(!result) return E_POINTER;
        if(auto n=Node()) *result={n->node.minimum.x,n->node.minimum.y,n->node.maximum.x-n->node.minimum.x,n->node.maximum.y-n->node.minimum.y};
        else { RECT rect{}; GetWindowRect(snapshot_->window,&rect); *result={double(rect.left),double(rect.top),double(rect.right-rect.left),double(rect.bottom-rect.top)}; }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** result) override { if(!result)return E_POINTER; *result=nullptr; return S_OK; }
    HRESULT STDMETHODCALLTYPE SetFocus() override { return Action(SemanticAction::Focus); }
    HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** result) override {
        if(!result) return E_POINTER; *result=new(std::nothrow) Provider(snapshot_,-1); return *result?S_OK:E_OUTOFMEMORY;
    }
    HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x,double y,IRawElementProviderFragment** result) override {
        if(!result) return E_POINTER; *result=nullptr;
        for(int i=static_cast<int>(snapshot_->nodes.size())-1;i>=0;--i) {
            auto& n=snapshot_->nodes[i].node;
            if(x>=n.minimum.x && y>=n.minimum.y && x<n.maximum.x && y<n.maximum.y) return At(i,result);
        } return At(-1,result);
    }
    HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** result) override {
        if(!result) return E_POINTER; *result=nullptr;
        for(int i=0;i<static_cast<int>(snapshot_->nodes.size());++i) if(snapshot_->nodes[i].node.state.focused) return At(i,result);
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE Invoke() override { return Action(SemanticAction::Press); }
    HRESULT STDMETHODCALLTYPE Toggle() override { return Action(SemanticAction::Toggle); }
    HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* result) override {
        if(!result) return E_POINTER; auto n=Node(); if(!n) return UIA_E_NOTSUPPORTED;
        *result=n->node.state.mixed?ToggleState_Indeterminate:n->node.state.checked?ToggleState_On:ToggleState_Off; return S_OK;
    }
};
}
HRESULT CreateWin32Provider(HWND window,const AccessibilityTree& tree,Win32ActionSink actions,IRawElementProviderFragmentRoot** result) {
    if(!result) return E_POINTER; *result=nullptr;
    if(!IsWindow(window) || !tree.Validate()) return E_INVALIDARG;
    try {
        auto snapshot=std::make_shared<Snapshot>(); snapshot->window=window; snapshot->actions=actions;
        for(auto& n:tree.nodes) {
            OwnedNode copy{n,Wide(n.name),Wide(n.description),Wide(n.value)};
            copy.node.name={}; copy.node.description={}; copy.node.value={}; copy.node.children={};
            snapshot->nodes.push_back(std::move(copy));
        }
        *result=new Provider(std::move(snapshot),-1); return S_OK;
    } catch(const std::bad_alloc&) { return E_OUTOFMEMORY; }
}
}
