#include <imkit/imkit.h>
#include <imkit/editor_canvas.h>
namespace {
auto volatile step=&imkit::StepNavigator;
auto volatile rail=&imkit::NavigationRail;
auto volatile filter=&imkit::FilterChip;
auto volatile choose=&imkit::SelectNotifications;
bool (*volatile notification)(const imkit::FeedbackView&,double,imkit::ComponentOptions)=&imkit::NotificationCard;
void (*volatile toast)(const char*,std::span<const imkit::FeedbackView>,double,std::span<std::size_t>,imkit::RequestBuffer&,imkit::ToastOptions,imkit::ComponentOptions)=&imkit::ToastRegion;
auto volatile alert=&imkit::InlineAlert;
auto volatile banner=&imkit::PersistentBanner;
bool (*volatile empty)(const char*,const imkit::StateView&,imkit::ComponentOptions)=&imkit::EmptyState;
auto volatile unavailable=&imkit::UnavailableState;
auto volatile retry=&imkit::RetryState;
bool (*volatile progress)(const char*,const imkit::ProgressView&,imkit::ProgressPresentation,imkit::DialogState&,ImVec2,imkit::ComponentOptions)=&imkit::Progress;
auto volatile card=&imkit::BeginCard;auto volatile endCard=&imkit::EndCard;
auto volatile section=&imkit::SectionHeader;auto volatile multi=&imkit::MultiSelectionBar;
auto volatile help=&imkit::HelpCallout;auto volatile validation=&imkit::ValidationSummary;
imkit::StableId (*volatile toolbar)(const char*,imkit::ToolbarState&,std::span<const imkit::Command>,imkit::ToolbarOptions,imkit::ComponentOptions)=&imkit::ResponsiveToolbar;
auto volatile valid=&imkit::editor::ImageGeometryValid;
auto volatile fit=&imkit::editor::FitImage;auto volatile clamp=&imkit::editor::ClampImage;
auto volatile normalized=&imkit::editor::PixelToNormalized;auto volatile pixel=&imkit::editor::NormalizedToPixel;
auto volatile viewport=&imkit::editor::BeginImageViewport;auto volatile endViewport=&imkit::editor::EndImageViewport;
auto volatile zoom=&imkit::editor::ZoomToolbar;auto volatile overlay=&imkit::editor::DrawOverlay;
auto volatile tile=&imkit::editor::PreviewTile;auto volatile strip=&imkit::editor::ResizableTileStrip;
}
int main() {
    imkit::StableId ids[1];imkit::RequestBuffer requests{ids};
    imkit::editor::TileEvent events[1];imkit::editor::TileEventBuffer out{events};
    return !(step&&rail&&filter&&choose&&notification&&toast&&alert&&banner&&empty&&unavailable&&retry&&progress&&card&&endCard&&section&&multi&&help&&validation&&toolbar&&valid&&fit&&clamp&&normalized&&pixel&&viewport&&endViewport&&zoom&&overlay&&tile&&strip&&requests.Push(1)&&out.Push({}));
}
