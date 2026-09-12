#include <imkit/node_editor.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

namespace imkit::node_editor {
namespace {
bool Finite(Point p) {
    return std::isfinite(p.x) && std::isfinite(p.y);
}
Point Add(Point a, Point b) {
    return {a.x + b.x, a.y + b.y};
}
Point Sub(Point a, Point b) {
    return {a.x - b.x, a.y - b.y};
}
bool Equal(Point a, Point b) {
    return a.x == b.x && a.y == b.y;
}
Rect Bounds(const NodeView &n) {
    return {n.position, Add(n.position, n.size)};
}
void Extend(Rect &a, Rect b) {
    a.min = {std::min(a.min.x, b.min.x), std::min(a.min.y, b.min.y)};
    a.max = {std::max(a.max.x, b.max.x), std::max(a.max.y, b.max.y)};
}
const NodeView *Find(GraphView g, NodeId id) {
    auto i = std::find_if(g.nodes.begin(), g.nodes.end(), [&](auto &n) { return n.id == id; });
    return i == g.nodes.end() ? nullptr : &*i;
}
const PinView *Pin(GraphView g, PinId id) {
    auto i = std::find_if(g.pins.begin(), g.pins.end(), [&](auto &p) { return p.id == id; });
    return i == g.pins.end() ? nullptr : &*i;
}
struct Layout {
    GraphView graph;
    std::unordered_map<std::uint64_t, const NodeView *> index;
    std::vector<const NodeView *> selected, roots;
    std::vector<PositionChange> changes;
    LayoutError error = LayoutError::None;
    Layout(GraphView g, std::span<const NodeId> ids) : graph(g) {
        index.reserve(g.nodes.size());
        for (auto &n : g.nodes) {
            if (!n.id || !Finite(n.position) || !Finite(n.size) || n.size.x <= 0 || n.size.y <= 0) {
                error = LayoutError::InvalidInput;
                return;
            }
            if (!index.emplace(n.id.value, &n).second) {
                error = LayoutError::DuplicateId;
                return;
            }
        }
        for (auto &n : g.nodes) {
            auto p = n.parent;
            for (std::size_t depth = 0; p; ++depth) {
                auto i = index.find(p.value);
                if (i == index.end() || depth >= g.nodes.size()) {
                    error = LayoutError::InvalidInput;
                    return;
                }
                p = i->second->parent;
            }
        }
        std::unordered_set<std::uint64_t> seen;
        for (auto id : ids) {
            auto i = index.find(id.value);
            if (i == index.end()) {
                error = LayoutError::MissingNode;
                return;
            }
            if (!seen.insert(id.value).second) {
                error = LayoutError::DuplicateId;
                return;
            }
            selected.push_back(i->second);
        }
        for (auto *n : selected) {
            bool ancestor = false;
            for (auto p = n->parent; p; p = index.at(p.value)->parent)
                if (seen.contains(p.value))
                    ancestor = true;
            if (!ancestor)
                roots.push_back(n);
        }
    }
    bool Locked(const NodeView &n) const {
        if (n.locked)
            return true;
        for (auto p = n.parent; p; p = index.at(p.value)->parent)
            if (index.at(p.value)->locked)
                return true;
        return false;
    }
    void Move(const NodeView &n, Point position) {
        if (Locked(n) || Equal(n.position, position))
            return;
        changes.push_back({n.id, n.position, position});
        auto delta = Sub(position, n.position);
        for (auto &c : graph.nodes) {
            if (c.id == n.id || Locked(c))
                continue;
            for (auto p = c.parent; p; p = index.at(p.value)->parent)
                if (p == n.id) {
                    changes.push_back({c.id, c.position, Add(c.position, delta)});
                    break;
                }
        }
    }
    LayoutResult Finish(std::span<PositionChange> out) {
        if (error != LayoutError::None)
            return {error, 0};
        for (auto &c : changes)
            if (!Finite(c.after))
                return {LayoutError::InvalidInput, 0};
        if (out.size() < changes.size())
            return {LayoutError::Capacity, 0};
        std::copy(changes.begin(), changes.end(), out.begin());
        return {LayoutError::None, changes.size()};
    }
};
} // namespace
bool RequestBuffer::PushBatch(std::span<const EditRequest> batch) {
    if (count > storage.size() || batch.size() > storage.size() - count) {
        overflow = true;
        return false;
    }
    if (!batch.empty())
        for (auto &r : batch)
            if (r.revision != batch.front().revision || r.graph != batch.front().graph ||
                r.phase != batch.front().phase || r.operation != batch.front().operation ||
                r.operationSize != batch.size())
                return false;
    std::copy(batch.begin(), batch.end(), storage.begin() + count);
    count += batch.size();
    return true;
}
bool RequestBuffer::Push(EditRequest r) {
    r.operationSize = 1;
    return PushBatch({&r, 1});
}
void RequestBuffer::Clear() {
    count = 0;
    overflow = false;
}
std::span<const EditRequest> RequestBuffer::Requests() const {
    return storage.first(std::min(count, storage.size()));
}
LayoutResult AlignNodes(GraphView g, std::span<const NodeId> selection, std::span<PositionChange> out,
                        AlignOptions opt) {
    Layout l(g, selection);
    if (l.error != LayoutError::None || l.roots.empty())
        return l.Finish(out);
    Rect b = Bounds(*l.roots.front());
    for (auto *n : l.roots)
        Extend(b, Bounds(*n));
    if (opt.reference == AlignReference::Node) {
        auto i = l.index.find(opt.anchor.value);
        if (i == l.index.end())
            return {LayoutError::MissingNode, 0};
        b = Bounds(*i->second);
    } else if (opt.reference == AlignReference::Rectangle)
        b = opt.bounds;
    if (!Finite(b.min) || !Finite(b.max) || b.min.x > b.max.x || b.min.y > b.max.y)
        return {LayoutError::InvalidInput, 0};
    for (auto *n : l.roots) {
        if (opt.reference == AlignReference::Node && n->id == opt.anchor)
            continue;
        auto p = n->position;
        switch (opt.alignment) {
        case Alignment::Left:
            p.x = b.min.x;
            break;
        case Alignment::CenterX:
            p.x = (b.min.x + b.max.x - n->size.x) / 2;
            break;
        case Alignment::Right:
            p.x = b.max.x - n->size.x;
            break;
        case Alignment::Top:
            p.y = b.min.y;
            break;
        case Alignment::CenterY:
            p.y = (b.min.y + b.max.y - n->size.y) / 2;
            break;
        case Alignment::Bottom:
            p.y = b.max.y - n->size.y;
            break;
        }
        l.Move(*n, p);
    }
    return l.Finish(out);
}
LayoutResult DistributeNodes(GraphView g, std::span<const NodeId> selection, std::span<PositionChange> out,
                             Axis axis, Distribution mode) {
    Layout l(g, selection);
    if (l.error != LayoutError::None || l.roots.size() < 3)
        return l.Finish(out);
    auto pos = [&](auto *n) { return axis == Axis::Horizontal ? n->position.x : n->position.y; };
    auto size = [&](auto *n) { return axis == Axis::Horizontal ? n->size.x : n->size.y; };
    auto key = [&](auto *n) { return pos(n) + (mode == Distribution::Centers ? size(n) / 2 : 0); };
    std::stable_sort(l.roots.begin(), l.roots.end(), [&](auto *a, auto *b) { return key(a) < key(b); });
    // Locked nodes form stationary interval boundaries, rather than breaking uniformity silently.
    std::size_t first = 0;
    for (std::size_t last = 1; last < l.roots.size(); ++last) {
        if (last + 1 < l.roots.size() && !l.Locked(*l.roots[last]))
            continue;
        if (last - first > 1) {
            double total = 0;
            for (auto i = first; i <= last; ++i)
                total += size(l.roots[i]);
            const double step =
                mode == Distribution::Centers
                    ? (key(l.roots[last]) - key(l.roots[first])) / (last - first)
                    : (pos(l.roots[last]) + size(l.roots[last]) - pos(l.roots[first]) - total) /
                          (last - first);
            double cursor = pos(l.roots[first]);
            for (auto i = first + 1; i < last; ++i) {
                cursor += size(l.roots[i - 1]) + step;
                auto p = l.roots[i]->position;
                double value = mode == Distribution::Centers
                                   ? key(l.roots[first]) + (i - first) * step - size(l.roots[i]) / 2
                                   : cursor;
                (axis == Axis::Horizontal ? p.x : p.y) = value;
                l.Move(*l.roots[i], p);
            }
        }
        first = last;
    }
    return l.Finish(out);
}
LayoutResult SnapNodesToGrid(GraphView g, std::span<const NodeId> selection, std::span<PositionChange> out,
                             double spacing) {
    if (!std::isfinite(spacing) || spacing <= 0)
        return {LayoutError::InvalidInput, 0};
    Layout l(g, selection);
    if (l.error == LayoutError::None)
        for (auto *n : l.roots)
            l.Move(*n, {std::round(n->position.x / spacing) * spacing,
                        std::round(n->position.y / spacing) * spacing});
    return l.Finish(out);
}
LayoutResult ArrangeNodes(GraphView g, std::span<const NodeId> selection, std::span<PositionChange> out,
                          ArrangeOptions opt) {
    if (!Finite(opt.gap) || opt.gap.x < 0 || opt.gap.y < 0)
        return {LayoutError::InvalidInput, 0};
    Layout l(g, selection);
    if (l.error != LayoutError::None || l.roots.empty())
        return l.Finish(out);
    // Process each visual container independently. Kahn layers + deterministic cycle remainder.
    std::vector<NodeId> parents;
    for (auto *n : l.roots)
        if (std::find(parents.begin(), parents.end(), n->parent) == parents.end())
            parents.push_back(n->parent);
    for (auto parent : parents) {
        std::vector<const NodeView *> nodes;
        for (auto *n : l.roots)
            if (n->parent == parent && !l.Locked(*n))
                nodes.push_back(n);
        if (nodes.empty())
            continue;
        std::sort(nodes.begin(), nodes.end(), [](auto *a, auto *b) { return a->id < b->id; });
        std::unordered_map<std::uint64_t, std::size_t> ix;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            ix[nodes[i]->id.value] = i;
        std::vector<std::vector<std::size_t>> edges(nodes.size());
        std::vector<int> degree(nodes.size()), layer(nodes.size(), 0);
        for (auto &link : g.links) {
            auto *a = Pin(g, link.from);
            auto *b = Pin(g, link.to);
            if (!a || !b || !ix.contains(a->node.value) || !ix.contains(b->node.value) || link.reference)
                continue;
            auto x = ix.at(a->node.value), y = ix.at(b->node.value);
            if (x == y)
                continue;
            edges[x].push_back(y);
            ++degree[y];
        }
        std::vector<std::size_t> queue;
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (!degree[i])
                queue.push_back(i);
        for (std::size_t k = 0; k < queue.size(); ++k)
            for (auto j : edges[queue[k]]) {
                layer[j] = std::max(layer[j], layer[queue[k]] + 1);
                if (!--degree[j])
                    queue.push_back(j);
            }
        int tail = 0;
        for (auto i : queue)
            tail = std::max(tail, layer[i] + 1);
        for (std::size_t i = 0; i < nodes.size(); ++i)
            if (degree[i])
                layer[i] = tail;
        Point base = nodes.front()->position;
        for (auto *n : nodes) {
            base.x = std::min(base.x, n->position.x);
            base.y = std::min(base.y, n->position.y);
        }
        double along = opt.direction == Axis::Horizontal ? base.x : base.y;
        int maxLayer = *std::max_element(layer.begin(), layer.end());
        for (int level = 0; level <= maxLayer; ++level) {
            double cross = opt.direction == Axis::Horizontal ? base.y : base.x, maxSize = 0;
            for (std::size_t i = 0; i < nodes.size(); ++i)
                if (layer[i] == level) {
                    const auto &n = *nodes[i];
                    Point p = opt.direction == Axis::Horizontal ? Point{along, cross} : Point{cross, along};
                    // Avoid stationary siblings (locked or outside the requested selection).
                    bool moved = true;
                    std::size_t passes = 0;
                    while (moved && passes++ <= g.nodes.size()) {
                        moved = false;
                        for (auto &fixed : g.nodes)
                            if (fixed.parent == parent && (!ix.contains(fixed.id.value) || l.Locked(fixed))) {
                                auto b = Bounds(fixed);
                                if (p.x < b.max.x && p.x + n.size.x > b.min.x && p.y < b.max.y &&
                                    p.y + n.size.y > b.min.y) {
                                    if (opt.direction == Axis::Horizontal)
                                        p.y = b.max.y + opt.gap.y;
                                    else
                                        p.x = b.max.x + opt.gap.x;
                                    moved = true;
                                }
                            }
                    }
                    l.Move(n, p);
                    cross = (opt.direction == Axis::Horizontal ? p.y + n.size.y + opt.gap.y
                                                               : p.x + n.size.x + opt.gap.x);
                    maxSize = std::max(maxSize, opt.direction == Axis::Horizontal ? n.size.x : n.size.y);
                }
            along += maxSize + (opt.direction == Axis::Horizontal ? opt.gap.x : opt.gap.y);
        }
    }
    return l.Finish(out);
}
bool FitGroupToContents(GraphView g, NodeId group, Rect &bounds, double padding) {
    Layout l(g, {});
    if (l.error != LayoutError::None || !l.index.contains(group.value) || !std::isfinite(padding) ||
        padding < 0)
        return false;
    bool found = false;
    Rect b{};
    for (auto &n : g.nodes)
        if (n.parent == group) {
            if (!found)
                b = Bounds(n);
            else
                Extend(b, Bounds(n));
            found = true;
        }
    if (found) {
        bounds = {{b.min.x - padding, b.min.y - padding}, {b.max.x + padding, b.max.y + padding}};
    }
    return found;
}
bool QueueLayout(GraphView g, std::span<const PositionChange> changes, RequestBuffer &out,
                 std::uint64_t operation) {
    if (!operation || out.count > out.storage.size() || changes.size() > out.storage.size() - out.count) {
        out.overflow = true;
        return false;
    }
    std::vector<EditRequest> requests;
    requests.reserve(changes.size());
    std::unordered_set<std::uint64_t> seen;
    for (auto &c : changes) {
        auto *n = Find(g, c.node);
        if (!n || n->locked || !Equal(n->position, c.before) || !Finite(c.after) ||
            !seen.insert(c.node.value).second)
            return false;
        EditRequest r;
        r.graph = g.id;
        r.revision = g.revision;
        r.operation = operation;
        r.operationSize = changes.size();
        r.node = c.node;
        r.before = c.before;
        r.after = c.after;
        requests.push_back(r);
    }
    return out.PushBatch(requests);
}
ConnectionVerdict CanConnect(GraphView g, PinId a, PinId b, LinkId replacing) {
    auto *x = Pin(g, a);
    auto *y = Pin(g, b);
    if (!x || !y || !a || !b)
        return {false, "Missing socket"};
    if (x->kind == y->kind)
        return {false, "Connect an output to an input"};
    if (x->kind == PinKind::Input)
        std::swap(x, y);
    auto *nx = Find(g, x->node);
    auto *ny = Find(g, y->node);
    if (!nx || !ny || nx->locked || ny->locked || x->hidden || y->hidden)
        return {false, "Socket is unavailable"};
    for (auto *node : {nx, ny}) {
        auto parent = node->parent;
        for (std::size_t depth = 0; parent; ++depth) {
            auto *ancestor = Find(g, parent);
            if (!ancestor || ancestor->locked || depth >= g.nodes.size())
                return {false, "Parent is unavailable"};
            parent = ancestor->parent;
        }
    }
    for (auto &link : g.links)
        if (link.id != replacing) {
            if (link.from == x->id && link.to == y->id)
                return {false, "Already connected"};
            if ((!x->multiple && link.from == x->id) || (!y->multiple && link.to == y->id))
                return {false, "Socket already occupied"};
        }
    if (g.canConnect)
        return g.canConnect(g.user, x->id, y->id);
    if (g.typeCompatibility)
        return g.typeCompatibility(g.user, x->type, y->type);
    if (x->type && y->type && x->type != y->type)
        return {false, "Different socket types"};
    return {}; // Cycles and same-node connections are host policy, not a UI restriction.
}
LayoutResult TraceNodes(GraphView g, NodeId start, bool upstream, std::span<NodeId> out) {
    if (!Find(g, start))
        return {LayoutError::MissingNode, 0};
    std::vector<NodeId> ids{start};
    for (std::size_t i = 0; i < ids.size(); ++i)
        for (auto &link : g.links) {
            auto *a = Pin(g, upstream ? link.to : link.from);
            auto *b = Pin(g, upstream ? link.from : link.to);
            if (a && b && a->node == ids[i] && Find(g, b->node) &&
                std::find(ids.begin(), ids.end(), b->node) == ids.end())
                ids.push_back(b->node);
        }
    if (out.size() < ids.size())
        return {LayoutError::Capacity, 0};
    std::copy(ids.begin(), ids.end(), out.begin());
    return {LayoutError::None, ids.size()};
}
bool IsPinConnected(GraphView g, PinId pin) {
    return std::any_of(g.links.begin(), g.links.end(), [&](auto &l) { return l.from == pin || l.to == pin; });
}
ConnectionVerdict ValidatePinEdit(GraphView g, const EditRequest &r) {
    auto *n = Find(g, r.node);
    if (!n || n->locked)
        return {false, "Node is unavailable"};
    auto parent = n->parent;
    for (std::size_t depth = 0; parent; ++depth) {
        if (depth >= g.nodes.size())
            return {false, "Invalid containment"};
        auto *ancestor = Find(g, parent);
        if (!ancestor || ancestor->locked)
            return {false, "Parent is unavailable"};
        parent = ancestor->parent;
        if (parent == n->id)
            return {false, "Invalid containment"};
    }
    auto *p = Pin(g, r.from);
    auto kind = p ? p->kind : r.pinKind;
    auto caps = p ? p->capabilities : (kind == PinKind::Input ? n->inputs : n->outputs);
    int count = 0;
    for (auto &pin : g.pins)
        if (pin.node == r.node && pin.kind == kind && pin.capabilities.group == (p ? caps.group : r.group))
            ++count;
    if (r.kind != EditKind::CreatePin && (!p || p->node != r.node))
        return {false, "Missing socket"};
    bool allowed = false;
    switch (r.kind) {
    case EditKind::CreatePin:
        allowed = caps.add && r.index >= 0 && r.index <= count && count < caps.maximum;
        break;
    case EditKind::DeletePin:
        allowed = caps.remove && !caps.required && count > caps.minimum;
        break;
    case EditKind::RenamePin:
        allowed = caps.rename && r.text[0];
        break;
    case EditKind::ChangePinType:
        allowed = caps.changeType;
        break;
    case EditKind::ReorderPin:
        allowed = caps.reorder && r.index >= 0 && r.index < count;
        break;
    case EditKind::SetPinMultiplicity:
        allowed = caps.changeMultiplicity;
        break;
    case EditKind::SetPinLimits:
        allowed = caps.group && caps.add && r.minimum >= 0 && r.maximum >= r.minimum && count >= r.minimum &&
                  count <= r.maximum;
        break;
    case EditKind::HidePin:
    case EditKind::ExposePin:
        allowed = true;
        break;
    case EditKind::SetPinValue:
        allowed = p->kind == PinKind::Input && !IsPinConnected(g, p->id);
        break;
    default:
        return {false, "Not a socket edit"};
    }
    if (!allowed)
        return {false, caps.reason.empty() ? "Socket capability or count limit" : caps.reason};
    if (r.kind == EditKind::CreatePin || r.kind == EditKind::ChangePinType) {
        if (!g.socketTypes.empty() &&
            std::none_of(g.socketTypes.begin(), g.socketTypes.end(), [&](auto &t) { return t.id == r.type; }))
            return {false, "Unknown socket type"};
    }
    if (g.canEditPin)
        return g.canEditPin(g.user, r);
    return {};
}
bool QueuePinEdits(GraphView g, std::span<const EditRequest> edits, RequestBuffer &out,
                   std::uint64_t operation) {
    if (!operation || edits.empty())
        return false;
    std::vector<EditRequest> batch;
    batch.reserve(edits.size());
    for (auto r : edits) {
        if (!ValidatePinEdit(g, r).allowed)
            return false;
        r.graph = g.id;
        r.revision = g.revision;
        r.operation = operation;
        r.operationSize = edits.size();
        r.affectedLinks = std::count_if(g.links.begin(), g.links.end(),
                                        [&](auto &l) { return l.from == r.from || l.to == r.from; });
        batch.push_back(r);
    }
    // The host validates the resulting model as one unit, including group counts,
    // dependent links and its own constraints. No model mutation takes place here.
    return out.PushBatch(batch);
}
} // namespace imkit::node_editor
