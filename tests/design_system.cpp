#include <imkit/imkit.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <array>
#include <limits>
#ifdef _MSC_VER
#include <crtdbg.h>
#endif
namespace {
int failures=0;
void Check(bool ok,const char* text) { if(!ok) { std::fprintf(stderr,"FAIL %s\n",text); ++failures; } }
int queries=0,rows=0,allocations=0; bool measure=false;
void* Allocate(std::size_t size,void*) { if(measure) ++allocations; return std::malloc(size); }
void Free(void* data,void*) { std::free(data); }
}
int main() {
#ifdef _MSC_VER
    _CrtSetReportMode(_CRT_ASSERT,_CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT,_CRTDBG_FILE_STDERR);
#endif
    using namespace imkit; using namespace imkit::accessibility;
    for(auto scheme:{ColorScheme::Light,ColorScheme::Dark}) for(auto contrast:{ContrastMode::Standard,ContrastMode::HighContrast})
        for(auto density:{Density::Compact,Density::Comfortable,Density::Touch}) {
            auto theme=MakeTheme(scheme,contrast,density);
            if(!ValidateContrast(theme)) std::fprintf(stderr,"contrast scheme=%d mode=%d density=%d\n",int(scheme),int(contrast),int(density));
            Check(ValidateContrast(theme),"theme contrast");
            Check(theme.metrics.controlHeight==(density==Density::Compact?24:density==Density::Touch?44:28),"density target");
            Check(theme.metrics.bodySize==14,"density preserves font size");
        }
    Check(ContrastRatio({0,0,0,1},{1,1,1,1})>20.99f,"WCAG black white");
    std::array<SemanticNode,4> nodes{}; std::array<ActionRequest,4> requests{};
    ActionQueue queue(requests); AccessibilityFrame semantic(nodes,&queue); semantic.Begin(7);
    std::array<StableId,1> children{2}; SemanticNode root; root.id=1; root.children=children;
    SemanticNode button; button.id=2; button.parent=1; button.role=SemanticRole::Button; button.name="Save"; button.actions=SemanticAction::Press;
    semantic.Add(root); semantic.Add(button);
    Check(semantic.Tree().Validate() && semantic.Tree().generation==7,"tree snapshot relationships");
    Check(semantic.Tree().Find(2)->name=="Save" && semantic.Tree().Find(2)->role==SemanticRole::Button,"tree name and role");
    Check(queue.Push({2,SemanticAction::Press}) && semantic.Take(2,SemanticAction::Press) && queue.Size()==0,"action consumed once");
    semantic.Add(button); Check(!semantic.Tree().Validate(),"duplicate rejected by validation");
    auto range=QueryVisibleRange(100000,500000,320,28); Check(range.count<=15 && range.first>17000,"visible range bounded");
    Check(QueryVisibleRange(100000,0,100,std::numeric_limits<float>::quiet_NaN()).count==0,"invalid geometry");
    LocaleContext rtl; rtl.direction=TextDirection::RTL; Check(rtl.VisualIndex(0,4)==3,"RTL order");
    ImGui::SetAllocatorFunctions(Allocate,Free); auto* context=ImGui::CreateContext(); auto& io=ImGui::GetIO();
    io.DisplaySize={1280,720}; io.DeltaTime=1.f/60; io.IniFilename=nullptr; io.ConfigFlags|=ImGuiConfigFlags_NavEnableKeyboard;
    unsigned char* pixels; int width,height; io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
    DataProvider provider; provider.count=100000;
    provider.query=[](void*,VisibleRange r) { ++queries; rows+=r.count; };
    provider.id=[](void*,int r)->StableId{return r+1;}; provider.cell=[](void*,int,int){return "Cell";};
    std::array<DataColumn,2> columns{{{1,"Name"},{2,"Value"}}}; DataTableState state;
    for(int frame=0;frame<8;++frame) {
        measure=frame>=4; queries=rows=0;
        ImGui::NewFrame(); ImGui::SetNextWindowSize({1000,600}); ImGui::Begin("test");
        semantic.Begin(frame); ComponentOptions options; options.accessibility=&semantic;
        ActionButton("Save",ActionVariant::Primary,{},options);
        Check(semantic.Tree().nodes.size()==1 && semantic.Tree().nodes[0].name=="Save","automatic semantic item");
        DataTable("data",state,provider,columns,320); ImGui::End(); ImGui::Render();
        Check(rows<100 && queries<6,"100000 rows query only viewport");
    }
    measure=false; Check(allocations==0,"steady ImGui allocations zero");
    ImGui::DestroyContext(context);
    std::printf("design system failures=%d allocations=%d\n",failures,allocations); return failures?1:0;
}
