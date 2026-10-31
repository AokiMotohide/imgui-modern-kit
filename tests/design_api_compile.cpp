#include <imkit/imkit.h>
namespace {
imkit::Theme (*volatile theme)(imkit::ColorScheme,imkit::ContrastMode,imkit::Density)=&imkit::MakeTheme;
void (*volatile resolve)(imkit::Theme&)=&imkit::ResolveTheme;
bool (*volatile contrast)(const imkit::Theme&)=&imkit::ValidateContrast;
imkit::StableId (*volatile palette)(const char*,imkit::CommandPaletteState&,std::span<const imkit::Command>,imkit::ComponentOptions)=&imkit::CommandPalette;
void (*volatile table)(const char*,imkit::DataTableState&,const imkit::DataProvider&,std::span<const imkit::DataColumn>,float,imkit::ComponentOptions)=&imkit::DataTable;
decltype(table) tree=&imkit::TreeDataGrid;
bool (*volatile annotate)(imkit::accessibility::AccessibilityFrame&,imkit::accessibility::SemanticNode)=&imkit::accessibility::AnnotateLastItem;
imkit::CompactActionRowRequest (*volatile compactActionRow)(const char*,const imkit::CompactActionRowOptions&)=&imkit::CompactActionRow;
imkit::Theme (*volatile brand)(const imkit::BrandPalette&)=&imkit::MakeBrandTheme;
void (*volatile corners)(imkit::Theme&,imkit::CornerStyle)=&imkit::SetCornerStyle;
std::string (*volatile serialize)(const imkit::Theme&)=&imkit::SerializeTheme;
imkit::ThemeParseResult (*volatile parse)(std::string_view,imkit::Theme&)=&imkit::ParseTheme;
void (*volatile pushRole)(const imkit::Theme&,imkit::TextRole)=&imkit::PushTextRole;
void (*volatile popRole)()=&imkit::PopTextRole;
void (*volatile shadow)(const imkit::Theme&,ImDrawList*,ImVec2,ImVec2,float,imkit::ElevationLevel,float)=&imkit::DrawShadow;
int (*volatile slices)(ImVec2,ImVec2,ImVec2,imkit::NineSliceBorder,float,ImVec2,ImVec2,imkit::NineSliceQuad(&)[9])=&imkit::NineSliceQuads;
void (*volatile nineSlice)(ImDrawList*,ImTextureRef,ImVec2,ImVec2,ImVec2,imkit::NineSliceBorder,float,ImU32,ImVec2,ImVec2)=&imkit::DrawNineSlice;
void (*volatile backdrop)(ImDrawList*,ImVec2,ImVec2,const imkit::BackdropStyle&)=&imkit::DrawBackdropPattern;
bool (*volatile skinFrame)(const imkit::SkinFrame&,void*)=imkit::Skin{}.drawFrame;
int (*volatile spriteAt)(const imkit::SpriteSheet&,double,bool)=&imkit::SpriteFrameAt;
bool (*volatile spriteUV)(const imkit::SpriteSheet&,int,ImVec2&,ImVec2&)=&imkit::SpriteFrameUV;
void (*volatile drawSprite)(ImDrawList*,const imkit::SpriteSheet&,int,ImVec2,ImVec2,ImU32)=&imkit::DrawSprite;
int (*volatile sprite)(const char*,const imkit::SpriteSheet&,imkit::SpriteState&,double,ImVec2,bool)=&imkit::Sprite;
imkit::ProgressThumbFn volatile thumb=nullptr;
}
int main(){return theme && resolve && contrast && palette && table && tree && annotate && compactActionRow && brand && corners && serialize && parse && pushRole && popRole && shadow && slices && nineSlice && backdrop && spriteAt && spriteUV && drawSprite && sprite && !thumb && !skinFrame?0:1;}
