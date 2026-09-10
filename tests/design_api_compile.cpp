#include <imkit/imkit.h>
namespace {
imkit::Theme (*volatile theme)(imkit::ColorScheme,imkit::ContrastMode,imkit::Density)=&imkit::MakeTheme;
void (*volatile resolve)(imkit::Theme&)=&imkit::ResolveTheme;
bool (*volatile contrast)(const imkit::Theme&)=&imkit::ValidateContrast;
imkit::StableId (*volatile palette)(const char*,imkit::CommandPaletteState&,std::span<const imkit::Command>,imkit::ComponentOptions)=&imkit::CommandPalette;
void (*volatile table)(const char*,imkit::DataTableState&,const imkit::DataProvider&,std::span<const imkit::DataColumn>,float,imkit::ComponentOptions)=&imkit::DataTable;
decltype(table) tree=&imkit::TreeDataGrid;
bool (*volatile annotate)(imkit::accessibility::AccessibilityFrame&,imkit::accessibility::SemanticNode)=&imkit::accessibility::AnnotateLastItem;
}
int main(){return theme && resolve && contrast && palette && table && tree && annotate?0:1;}
