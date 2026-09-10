#pragma once
#include <imkit/components.h>
#include <array>

namespace imkit {
using StableId=accessibility::StableId;
struct Command { StableId id; const char* label; const char* shortcut=""; bool disabled=false; const char* disabledReason=""; };
struct CommandPaletteState { bool open=false; char search[256]{}; int focused=0; };
// Returns the stable command ID to execute, or zero. Host applies the command.
StableId CommandPalette(const char* id, CommandPaletteState& state, std::span<const Command> commands, ComponentOptions options={});
bool SearchField(const char* id, char* text, std::size_t capacity, ComponentOptions options={});
int Breadcrumbs(const char* id, std::span<const char* const> labels, ComponentOptions options={});
enum class SplitButtonResult { None, Primary, Menu };
SplitButtonResult SplitButton(const char* id, const char* label, ComponentOptions options={});
struct ToolbarState { int focused=0; bool focusPending=false; };
StableId ResponsiveToolbar(const char* id, ToolbarState& state, std::span<const Command> commands, ComponentOptions options={});
StableId Toolbar(const char* id, ToolbarState& state, std::span<const Command> commands, ComponentOptions options={});
struct DialogState { bool open=false; bool initialFocus=false; bool restoreFocus=false; };
bool DialogLauncher(const char* label, DialogState& state, ComponentOptions options={});
bool BeginDialog(const char* id, DialogState& state, ComponentOptions options={});
void EndDialog();
enum class DialogResult { None, Accept, Cancel };
DialogResult AlertDialog(const char* id, const char* message, DialogState& state, ComponentOptions options={});
bool BeginPopover(const char* id); // Open with public ImGui::OpenPopup. End only if true.
void EndPopover();
StableId Menu(const char* id, std::span<const Command> commands, ComponentOptions options={});
struct FormFieldInfo { const char* label; const char* hint=""; const char* validation=""; bool required=false; };
bool BeginFormField(const char* id, const FormFieldInfo& field, ComponentOptions options={});
void EndFormField();
void Progress(const char* id, float fraction, const char* label, ComponentOptions options={}); // negative = indeterminate
void Spinner(const char* id, ComponentOptions options={});
void Skeleton(const char* id, ImVec2 size, ComponentOptions options={});
void EmptyState(const char* message);
void LoadingState(const char* message, ComponentOptions options={});
bool ErrorState(const char* message, ComponentOptions options={}); // retry request
const char* ToastRegion(std::span<const Notification> notifications, double now, ComponentOptions options={});
bool Pagination(const char* id, int& page, int pageCount, ComponentOptions options={});
// Host draws the first pane, calls Next, draws the second pane, then End.
struct AdaptiveSplitState { bool stacked=false; bool firstVisible=false; };
bool BeginAdaptiveSplitLayout(const char* id, AdaptiveSplitState& state, float minimumPaneWidth=280, float firstFraction=.35f);
bool NextAdaptiveSplitPane(AdaptiveSplitState& state);
void EndAdaptiveSplitLayout();

struct VisibleRange { int first=0, count=0; };
VisibleRange QueryVisibleRange(int count, float scroll, float height, float rowHeight, int overscan=1);
struct ListProvider {
    void* user=nullptr; int count=0;
    void (*query)(void*, VisibleRange)=nullptr;
    StableId (*id)(void*, int row)=nullptr;
    const char* (*label)(void*, int row)=nullptr;
};
StableId VirtualList(const char* id, const ListProvider& provider, StableId selected, float height=240, ComponentOptions options={});
struct DataColumn { StableId id; const char* label; float width=160; bool editable=false; };
enum class DataAction { Select, Sort, Filter, Edit, Context, Expand };
struct DataEvent {
    DataAction action; StableId row=0, column=0; int first=0, last=0;
    bool control=false, shift=false, descending=false; std::string_view value{};
};
// query prepares ONLY the requested visible range. Provider owns the flattened
// visible index (including tree expansion/filter/sort), selection and all data.
struct DataProvider {
    void* user=nullptr; int count=0;
    void (*query)(void*, VisibleRange)=nullptr;
    StableId (*id)(void*, int row)=nullptr;
    const char* (*cell)(void*, int row, int column)=nullptr;
    bool (*selected)(void*, StableId)=nullptr;
    int (*depth)(void*, int row)=nullptr;
    bool (*expandable)(void*, int row)=nullptr;
    bool (*expanded)(void*, int row)=nullptr;
    void (*apply)(void*, const DataEvent&)=nullptr;
};
struct DataTableState {
    int focusedRow=0, focusedColumn=0, anchor=0;
    StableId editingRow=0, editingColumn=0;
    char edit[512]{}, filter[256]{};
    bool focusEditor=false;
    bool focusPending=false;
};
void DataTable(const char* id, DataTableState& state, const DataProvider& provider,
               std::span<const DataColumn> columns, float height=320, ComponentOptions options={});
void TreeDataGrid(const char* id, DataTableState& state, const DataProvider& provider,
                  std::span<const DataColumn> columns, float height=320, ComponentOptions options={});
} // namespace imkit
