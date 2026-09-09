#pragma once
#include <imkit/editor_core.h>

namespace imkit::detail {
// Retry terminal events on subsequent frames, including when the edited item is clipped.
inline void ResumeTerminal(editor::Transaction &transaction, std::uint64_t revision,
                           editor::EventBuffer &events) {
    if (!transaction.active)
        return;
    if (transaction.draft.phase == editor::Phase::Cancel || transaction.draft.revision != revision)
        transaction.Cancel(events);
    else if (transaction.draft.phase == editor::Phase::Commit)
        transaction.Commit(revision, events);
}
}
