#pragma once
#include <imkit/imkit.h>
#include <vector>
#include <array>
#include <string>
namespace imkit::gallery {
struct DesignPages {
    struct Row { std::array<char,64> name{}, value{}; bool selected=false; };
    std::vector<Row> rows;
    std::vector<int> order;
    std::array<bool,10000> collapsed{};
    DataTableState table;
    CommandPaletteState palette;
    ToolbarState toolbar;
    DialogState dialog;
    LocaleContext locale;
    std::array<accessibility::SemanticNode,512> nodes;
    std::array<accessibility::ActionRequest,64> requests;
    accessibility::ActionQueue actions{requests};
    accessibility::AccessibilityFrame semantics{nodes,&actions};
    int density=1, contrast=0, language=0, clicks=0, queried=0, page=0;
    bool reducedMotion=false, tree=false, loading=false, error=false, toast=false;
    bool descending=false;
    std::string filter;
    void Prepare();
    void Reindex();
    void Show(int section,Theme& theme);
};
}
