#include <imkit/node_editor.h>
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#ifdef IMKIT_NODE_EDITOR_METAL
#include <imgui_impl_metal.h>
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3native.h>
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#else
#include <windows.h>
#include <objbase.h>
#include <imgui_impl_opengl3.h>
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <unordered_map>
#include <memory>
#include "../design_gallery/capture.h"
#include "material_mock.h"
#include "toolbar.h"
#include "appearance.h"
#ifdef _MSC_VER
#include <crtdbg.h>
#include <cstdlib>
#endif

namespace ne = imkit::node_editor;
namespace {
#ifdef IMKIT_NODE_EDITOR_METAL
id<MTLDevice> gMetalDevice=nil;
id<MTLTexture> MakeMetalTexture(int width,int height,const void *rgba) {
    auto *d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:width height:height mipmapped:NO];
    d.usage=MTLTextureUsageShaderRead; d.storageMode=MTLStorageModeShared;
    id<MTLTexture> texture=[gMetalDevice newTextureWithDescriptor:d];
    [texture replaceRegion:MTLRegionMake2D(0,0,width,height) mipmapLevel:0 withBytes:rgba bytesPerRow:width*4]; return texture;
}
#endif
struct Node {
    ImVec4 color{};
    ne::NodeStyle customStyle{};
    ne::NodeView view;
    ne::GraphId graph{1};
    std::string title;
    float value = .5f;
    bool valueExposed = false;
};
struct Snapshot {
    std::vector<Node> nodes;
    std::vector<ne::PinView> pins;
    std::vector<ne::LinkView> links;
};
// Application model and undo deliberately live in this example, not in the library.
struct Model {
    Snapshot data, clipboard;
    std::vector<Snapshot> undo, redo;
    std::vector<Snapshot> templates;
    std::vector<ne::NodeView> views;
    std::vector<ne::PinView> pins;
    std::vector<ne::LinkView> links;
    ne::GraphId graph{1};
    std::uint64_t next = 100, revision = 1;
    std::vector<ne::PathEntry> path{{{1}, {}, "Root"}};
    bool changed = false;
    Node *Find(ne::NodeId id) {
        auto i = std::find_if(data.nodes.begin(), data.nodes.end(), [&](auto &n) { return n.view.id == id; });
        return i == data.nodes.end() ? nullptr : &*i;
    }
    ne::PinView *Pin(ne::PinId id) {
        auto i = std::find_if(data.pins.begin(), data.pins.end(), [&](auto &p) { return p.id == id; });
        return i == data.pins.end() ? nullptr : &*i;
    }
    ne::NodeId Add(ne::Point p, std::uint64_t type = 1) {
        Node n;
        n.view.id = {next++};
        n.view.position = p;
        n.view.size = {260, 390};
        n.graph = graph;
        n.title = type == 2   ? "Image preview"
                  : type == 3 ? "Monitor"
                  : type == 4 ? "Note"
                  : type == 5 ? "Frame"
                  : type == 6 ? "Reroute"
                              : "Processing node";
        if (type == 4) {
            n.view.kind = ne::NodeKind::Note;
            n.view.description = "Add a note or annotation to your graph.";
            n.view.size = {260, 140};
        }
        if (type == 5) {
            n.view.kind = ne::NodeKind::Frame;
            n.view.size = {360, 240};
        }
        if (type == 6) {
            n.view.kind = ne::NodeKind::Reroute;
            n.view.size = {160, 120};
        }
        auto id = n.view.id;
        data.nodes.push_back(n);
        if (type != 4 && type != 5) {
            ne::PinView in;
            in.id = {next++};
            in.node = id;
            in.label = "Input";
            in.manualPosition = false;
            data.pins.push_back(in);
            ne::PinView out;
            out.id = {next++};
            out.node = id;
            out.label = "Output";
            out.manualPosition = false;
            out.kind = ne::PinKind::Output;
            out.side = ne::Side::Right;
            out.multiple = true;
            data.pins.push_back(out);
        }
        return id;
    }
    void Init() {
        auto a = Add({40, 60}), b = Add({380, 100}, 2), c = Add({720, 60}, 3);
        data.nodes[0].title = "Source";
        data.nodes[1].title = "Color adjustment";
        data.nodes[2].title = "Output monitor";
        data.nodes[1].view.status = ne::Status::Cached;
        data.nodes[1].view.milliseconds = .32;
        data.links.push_back({{next++}, Socket(a, false), Socket(b, true)});
        data.links.push_back({{next++}, Socket(b, false), Socket(c, true)});
    }
    ne::PinId Socket(ne::NodeId node, bool input) {
        for (auto &p : data.pins)
            if (p.node == node && p.kind == (input ? ne::PinKind::Input : ne::PinKind::Output))
                return p.id;
        return {};
    }
    ne::GraphView View() {
        views.clear();
        pins.clear();
        links.clear();
        for (auto &n : data.nodes)
            if (n.graph == graph) {
                auto v = n.view;
                v.title = n.title;
                views.push_back(v);
            }
        for (auto &p : data.pins)
            if (auto *n = Find(p.node); n && n->graph == graph)
                pins.push_back(p);
        for (auto &link : data.links) {
            auto a = std::find_if(pins.begin(), pins.end(), [&](auto &p) { return p.id == link.from; });
            auto b = std::find_if(pins.begin(), pins.end(), [&](auto &p) { return p.id == link.to; });
            if (a != pins.end() && b != pins.end())
                links.push_back(link);
        }
        return {graph, revision, views, pins, links};
    }
    void Save() {
        undo.push_back(data);
        if (undo.size() > 64)
            undo.erase(undo.begin());
        redo.clear();
    }
    std::vector<ne::NodeId> Closure(std::span<const ne::EditRequest> requests) {
        std::vector<ne::NodeId> ids;
        for (auto &r : requests)
            if (r.node && Find(r.node))
                ids.push_back(r.node);
        for (std::size_t i = 0; i < ids.size(); ++i)
            for (auto &n : data.nodes)
                if ((n.view.parent == ids[i] ||
                     (Find(ids[i])->view.childGraph && n.graph == Find(ids[i])->view.childGraph)) &&
                    std::find(ids.begin(), ids.end(), n.view.id) == ids.end())
                    ids.push_back(n.view.id);
        return ids;
    }
    void Copy(std::span<const ne::EditRequest> requests) {
        clipboard = {};
        auto ids = Closure(requests);
        for (auto &n : data.nodes)
            if (std::find(ids.begin(), ids.end(), n.view.id) != ids.end())
                clipboard.nodes.push_back(n);
        for (auto &p : data.pins)
            if (std::find(ids.begin(), ids.end(), p.node) != ids.end())
                clipboard.pins.push_back(p);
        for (auto &l : data.links) {
            auto *a = Pin(l.from);
            auto *b = Pin(l.to);
            if (a && b && std::find(ids.begin(), ids.end(), a->node) != ids.end() &&
                std::find(ids.begin(), ids.end(), b->node) != ids.end())
                clipboard.links.push_back(l);
        }
    }
    void Paste() {
        std::unordered_map<std::uint64_t, std::uint64_t> nodeMap, pinMap, graphMap;
        for (auto &n : clipboard.nodes) {
            nodeMap[n.view.id.value] = next++;
            if (n.view.childGraph)
                graphMap[n.view.childGraph.value] = next++;
        }
        for (auto n : clipboard.nodes) {
            n.view.id = {nodeMap.at(n.view.id.value)};
            n.view.parent = {nodeMap.contains(n.view.parent.value) ? nodeMap.at(n.view.parent.value) : 0};
            n.graph = {graphMap.contains(n.graph.value) ? graphMap.at(n.graph.value) : graph.value};
            if (n.view.childGraph)
                n.view.childGraph = {graphMap.at(n.view.childGraph.value)};
            n.view.position.x += 36;
            n.view.position.y += 36;
            data.nodes.push_back(n);
        }
        for (auto &p : clipboard.pins)
            pinMap[p.id.value] = next++;
        for (auto p : clipboard.pins) {
            p.node = {nodeMap.at(p.node.value)};
            p.id = {pinMap.at(p.id.value)};
            p.internal = {pinMap.contains(p.internal.value) ? pinMap.at(p.internal.value) : 0};
            data.pins.push_back(p);
        }
        for (auto l : clipboard.links) {
            l.id = {next++};
            l.from = {pinMap.at(l.from.value)};
            l.to = {pinMap.at(l.to.value)};
            data.links.push_back(l);
        }
    }
    void Delete(std::span<const ne::EditRequest> requests) {
        auto ids = Closure(requests);
        std::vector<ne::PinId> removed;
        for (auto &p : data.pins)
            if (std::find(ids.begin(), ids.end(), p.node) != ids.end())
                removed.push_back(p.id);
        std::erase_if(data.links, [&](auto &l) {
            return std::find(removed.begin(), removed.end(), l.from) != removed.end() ||
                   std::find(removed.begin(), removed.end(), l.to) != removed.end();
        });
        std::erase_if(data.pins, [&](auto &p) {
            return std::find(removed.begin(), removed.end(), p.id) != removed.end();
        });
        std::erase_if(data.nodes,
                      [&](auto &n) { return std::find(ids.begin(), ids.end(), n.view.id) != ids.end(); });
    }
    void Group(std::span<const ne::EditRequest> requests, bool subgraph) {
        auto ids = Closure(requests);
        // A selected subgraph moves as one node; its inner graph retains its identity.
        std::erase_if(ids, [&](auto id) { return Find(id)->graph != graph; });
        if (ids.empty())
            return;
        auto parent = Find(ids.front())->view.parent;
        ne::Point min = Find(ids.front())->view.position, max = min;
        for (auto id : ids) {
            auto *n = Find(id);
            min.x = std::min(min.x, n->view.position.x);
            min.y = std::min(min.y, n->view.position.y);
            max.x = std::max(max.x, n->view.position.x + n->view.size.x);
            max.y = std::max(max.y, n->view.position.y + n->view.size.y);
        }
        Node groupNode;
        groupNode.graph = graph;
        groupNode.view.id = {next++};
        groupNode.title = subgraph ? "Subgraph" : "Group";
        groupNode.view.kind = subgraph ? ne::NodeKind::Subgraph : ne::NodeKind::Group;
        groupNode.view.parent = parent;
        groupNode.view.position = {min.x - 24, min.y - 48};
        groupNode.view.size =
            subgraph ? ne::Point{260, 160} : ne::Point{max.x - min.x + 48, max.y - min.y + 72};
        if (subgraph)
            groupNode.view.childGraph = {next++};
        auto groupId = groupNode.view.id;
        auto child = groupNode.view.childGraph;
        data.nodes.insert(data.nodes.begin(), groupNode);
        for (auto id : ids) {
            auto *n = Find(id);
            if (std::find(ids.begin(), ids.end(), n->view.parent) == ids.end())
                n->view.parent = subgraph ? ne::NodeId{} : groupId;
            if (subgraph)
                n->graph = child;
        }
        if (!subgraph)
            return;
        // Split boundary wires through exposed sockets. Original socket correspondence
        // allows the host to unpack the subgraph without losing the external links.
        std::vector<ne::PinView> exposed;
        std::unordered_map<std::uint64_t, ne::PinId> mapping;
        for (auto &l : data.links) {
            auto *a = Pin(l.from);
            auto *b = Pin(l.to);
            if (!a || !b)
                continue;
            bool ai = std::find(ids.begin(), ids.end(), a->node) != ids.end(),
                 bi = std::find(ids.begin(), ids.end(), b->node) != ids.end();
            if (ai == bi)
                continue;
            auto original = ai ? *a : *b;
            if (!mapping.contains(original.id.value)) {
                auto p = original;
                p.id = {next++};
                p.node = groupId;
                p.internal = original.id;
                p.offset = 60 + 20.f * exposed.size();
                exposed.push_back(p);
                mapping[original.id.value] = p.id;
            }
            if (ai)
                l.from = mapping.at(original.id.value);
            else
                l.to = mapping.at(original.id.value);
        }
        data.pins.insert(data.pins.end(), exposed.begin(), exposed.end());
    }
    void Ungroup(ne::NodeId id) {
        auto *ptr = Find(id);
        if (!ptr)
            return;
        auto groupNode = *ptr;
        if (groupNode.view.kind != ne::NodeKind::Group && groupNode.view.kind != ne::NodeKind::Frame &&
            groupNode.view.kind != ne::NodeKind::Subgraph)
            return;
        for (auto &n : data.nodes) {
            if (n.view.parent == id)
                n.view.parent = groupNode.view.parent;
            if (groupNode.view.childGraph && n.graph == groupNode.view.childGraph) {
                n.graph = groupNode.graph;
                if (!n.view.parent)
                    n.view.parent = groupNode.view.parent;
            }
        }
        for (auto &link : data.links) {
            auto *a = Pin(link.from);
            auto *b = Pin(link.to);
            if (a && a->node == id && a->internal)
                link.from = a->internal;
            if (b && b->node == id && b->internal)
                link.to = b->internal;
        }
        std::erase_if(data.pins, [&](auto &p) { return p.node == id; });
        std::erase_if(data.nodes, [&](auto &n) { return n.view.id == id; });
    }
    void Apply(std::span<const ne::EditRequest> all) {
        // Every batch is complete and belongs to the frame's immutable revision.
        const auto frameRevision = revision;
        bool modified = false;
        for (std::size_t i = 0; i < all.size();) {
            auto count = all[i].operationSize;
            if (!count || i + count > all.size())
                break;
            auto batch = all.subspan(i, count);
            i += count;
            auto r = batch.front();
            if (r.phase != ne::Phase::Commit || r.graph != graph || r.revision != frameRevision)
                continue;
            if (r.kind == ne::EditKind::EnterGraph) {
                if (r.type) {
                    graph = {r.type};
                    auto p =
                        std::find_if(path.begin(), path.end(), [&](auto &e) { return e.graph == graph; });
                    if (p != path.end())
                        path.erase(p + 1, path.end());
                } else if (auto *n = Find(r.node); n && n->view.childGraph) {
                    graph = n->view.childGraph;
                    path.push_back({graph, r.node, "Subgraph"});
                }
                continue;
            }
            if (r.kind == ne::EditKind::Undo) {
                if (!undo.empty()) {
                    redo.push_back(data);
                    data = undo.back();
                    undo.pop_back();
                    modified = true;
                }
                continue;
            }
            if (r.kind == ne::EditKind::Redo) {
                if (!redo.empty()) {
                    undo.push_back(data);
                    data = redo.back();
                    redo.pop_back();
                    modified = true;
                }
                continue;
            }
            if (r.kind == ne::EditKind::Copy) {
                Copy(batch);
                continue;
            }
            if (r.kind == ne::EditKind::SaveTemplate) {
                auto saved = clipboard;
                Copy(batch);
                templates.push_back(clipboard);
                clipboard = saved;
                continue;
            }
            if (r.kind == ne::EditKind::Run || r.kind == ne::EditKind::Step || r.kind == ne::EditKind::Stop)
                continue;
            Save();
            modified = true;
            if (r.kind == ne::EditKind::Group || r.kind == ne::EditKind::MakeSubgraph) {
                Group(batch, r.kind == ne::EditKind::MakeSubgraph);
                continue;
            }
            if (r.kind == ne::EditKind::Cut) {
                Copy(batch);
                Delete(batch);
                continue;
            }
            if (r.kind == ne::EditKind::Duplicate) {
                auto saved = clipboard;
                Copy(batch);
                Paste();
                clipboard = saved;
                continue;
            }
            if (r.kind == ne::EditKind::Paste) {
                Paste();
                continue;
            }
            if (r.kind == ne::EditKind::DeleteNode) {
                Delete(batch);
                continue;
            }
            for (auto &event : batch) {
                auto *n = Find(event.node);
                switch (event.kind) {
                case ne::EditKind::InsertNode: {
                    auto it = std::find_if(data.links.begin(), data.links.end(),
                                           [&](auto &link) { return link.id == event.link; });
                    if (it != data.links.end()) {
                        auto previous = *it;
                        data.links.erase(it);
                        data.links.push_back({{next++}, previous.from, event.from});
                        data.links.push_back({{next++}, event.to, previous.to});
                    }
                    break;
                }
                case ne::EditKind::Rename:
                    if (n)
                        n->title = event.text.data();
                    break;
                case ne::EditKind::HidePin:
                    if (auto *p = Pin(event.from))
                        p->hidden = event.index != 0;
                    break;
                case ne::EditKind::ReorderPin: {
                    std::vector<ne::PinView> sockets;
                    for (auto &p : data.pins)
                        if (p.node == event.node)
                            sockets.push_back(p);
                    auto it = std::find_if(sockets.begin(), sockets.end(),
                                           [&](auto &p) { return p.id == event.from; });
                    if (it != sockets.end() && event.index >= 0 && event.index < int(sockets.size())) {
                        auto pin = *it;
                        sockets.erase(it);
                        sockets.insert(sockets.begin() + event.index, pin);
                        for (std::size_t j = 0; j < sockets.size(); ++j)
                            sockets[j].offset = 65 + 28.f * j;
                        std::size_t j = 0;
                        for (auto &p : data.pins)
                            if (p.node == event.node)
                                p = sockets[j++];
                    }
                    break;
                }
                case ne::EditKind::ExposePin: {
                    auto *pin = Pin(event.from);
                    if (!pin)
                        break;
                    auto original = *pin;
                    auto owner = std::find_if(data.nodes.begin(), data.nodes.end(),
                                              [&](auto &item) { return item.view.childGraph == graph; });
                    if (owner != data.nodes.end()) {
                        bool exists = std::any_of(data.pins.begin(), data.pins.end(), [&](auto &p) {
                            return p.node == owner->view.id && p.internal == original.id;
                        });
                        if (!exists) {
                            original.internal = original.id;
                            original.id = {next++};
                            original.node = owner->view.id;
                            data.pins.push_back(original);
                        }
                    }
                    break;
                }
                case ne::EditKind::ExposeProperty:
                    if (n)
                        n->valueExposed = event.index != 0;
                    break;
                case ne::EditKind::Move:
                    if (n)
                        n->view.position = event.after;
                    break;
                case ne::EditKind::Resize:
                    if (n)
                        n->view.size = event.after;
                    break;
                case ne::EditKind::Collapse:
                    if (n)
                        n->view.collapsed = !n->view.collapsed;
                    break;
                case ne::EditKind::Lock:
                    if (n)
                        n->view.locked = !n->view.locked;
                    break;
                case ne::EditKind::Breakpoint:
                    if (n)
                        n->view.breakpoint = !n->view.breakpoint;
                    break;
                case ne::EditKind::Bypass:
                    if (n)
                        n->view.status =
                            n->view.status == ne::Status::Bypassed ? ne::Status::Idle : ne::Status::Bypassed;
                    break;
                case ne::EditKind::Ungroup:
                    Ungroup(event.node);
                    break;
                case ne::EditKind::CreateNode: {
                    if (event.type >= 1000 && event.type - 1000 < templates.size()) {
                        auto saved = clipboard;
                        clipboard = templates[event.type - 1000];
                        Paste();
                        clipboard = saved;
                        break;
                    }
                    auto id = Add(event.after, event.type);
                    if (event.from) {
                        auto *from = Pin(event.from);
                        if (from) {
                            bool input = from->kind == ne::PinKind::Input;
                            auto socket = Socket(id, !input);
                            ne::LinkView link;
                            link.id = {next++};
                            link.from = input ? socket : event.from;
                            link.to = input ? event.from : socket;
                            data.links.push_back(link);
                        }
                    }
                    break;
                }
                case ne::EditKind::Reconnect:
                    std::erase_if(data.links, [&](auto &link) { return link.id == event.link; });
                    [[fallthrough]];
                case ne::EditKind::CreateLink:
                    data.links.push_back({{next++}, event.from, event.to});
                    break;
                case ne::EditKind::DeleteLink:
                    std::erase_if(data.links, [&](auto &link) { return link.id == event.link; });
                    break;
                default:
                    break;
                }
            }
        }
        if (modified)
            ++revision;
        changed = modified;
    }
};
void PreviewWave(void *user, ImVec2 size, bool) {
    auto value = *static_cast<float *>(user);
    float points[80];
    for (int i = 0; i < 80; ++i)
        points[i] = std::sin(i * .15f) * value;
    ImGui::PlotLines("##wave", points, 80, 0, nullptr, -1, 1, size);
}
struct Demo {
    node_gallery::AppearanceEditor appearance;
    ImVec2 monitorHeader{};
    material_mock::Page material;
    bool materialPage = true;
    Model model;
    ne::EditorState state;
    std::array<ne::EditRequest, 4096> requests{};
    bool dark = true, snap = false, lasso = false;
    int wire = 0, density = 1;
    bool highContrast = false;
    #ifdef IMKIT_NODE_EDITOR_METAL
    id<MTLTexture> texture=nil;
    #else
    GLuint texture = 0;
    #endif
    imkit::IconAtlas icons;
    #ifdef IMKIT_NODE_EDITOR_METAL
    std::array<id<MTLTexture>,7> iconTextures{};
    #else
    std::array<GLuint, 7> iconTextures{};
    #endif
    bool reducedMotion = false;
    void Init() {
#ifndef IMKIT_NODE_EDITOR_METAL
        glGenTextures(int(iconTextures.size()), iconTextures.data());
#endif
        for (std::size_t i = 0; i < iconTextures.size(); ++i) {
            auto pixels = imkit::GetIconAtlasPixels(imkit::IconPixelSizes[i]);
#ifdef IMKIT_NODE_EDITOR_METAL
            iconTextures[i]=MakeMetalTexture(pixels.width,pixels.height,pixels.rgba.data());
            icons.SetTexture(imkit::IconPixelSizes[i],ImTextureRef((__bridge void*)iconTextures[i]));
#else
            glBindTexture(GL_TEXTURE_2D, iconTextures[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, 0x812F /* GL_CLAMP_TO_EDGE */);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, 0x812F /* GL_CLAMP_TO_EDGE */);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pixels.width, pixels.height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                         pixels.rgba.data());
            icons.SetTexture(imkit::IconPixelSizes[i], ImTextureRef(ImTextureID(iconTextures[i])));
#endif
        }
        model.Init();
        state.Reserve(1024);
        state.origin = {-25, -20};
        std::array<unsigned char, 64 * 64 * 4> pixels{};
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                int i = (y * 64 + x) * 4;
                pixels[i] = static_cast<unsigned char>(x * 4);
                pixels[i + 1] = static_cast<unsigned char>(y * 4);
                pixels[i + 2] = 180;
                pixels[i + 3] = 255;
            }
#ifdef IMKIT_NODE_EDITOR_METAL
        texture=MakeMetalTexture(64,64,pixels.data());
#else
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
#endif
    }
    std::array<ne::PreviewOutput, 2> Outputs(Node &n) {
        ne::PreviewOutput wave;
        wave.title = "Waveform";
        wave.status = ne::PreviewStatus::Ready;
        wave.user = &n.value;
        wave.draw = PreviewWave;
        ne::PreviewOutput image;
        image.title = "Texture";
        image.status = ne::PreviewStatus::Ready;
#ifdef IMKIT_NODE_EDITOR_METAL
        image.texture=ImTextureRef((__bridge void*)texture);
#else
        image.texture = ImTextureRef(ImTextureID(texture));
#endif
        image.aspect = 1;
        return {wave, image};
    }
    void Draw() {
        auto theme =
            imkit::MakeTheme(dark ? imkit::ColorScheme::Dark : imkit::ColorScheme::Light,
                             highContrast ? imkit::ContrastMode::HighContrast : imkit::ContrastMode::Standard,
                             static_cast<imkit::Density>(density));
        theme.motion.reducedMotion = reducedMotion;
        imkit::ThemeScope scope(theme);
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("ModernKIT Node Studio", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted("ModernKIT / Node Studio");
        if (ImGui::GetContentRegionAvail().x > 520)
            ImGui::SameLine();
        if (ImGui::Button("Node Studio")) materialPage = false;
        ImGui::SameLine();
        if (ImGui::Button("Material Graph")) materialPage = true;
        ImGui::SameLine();
        if (node_gallery::Action("settings", icons, imkit::IconId::Settings,
                                 "Appearance and canvas settings"))
            ImGui::OpenPopup("Settings");
        if (ImGui::BeginPopup("Settings")) {
            ImGui::Checkbox("Dark", &dark);
            ImGui::SetNextItemWidth(160);
            ImGui::Combo("Density", &density, "Compact\0Comfortable\0Touch\0");
            ImGui::Checkbox("High contrast", &highContrast);
            ImGui::Checkbox("Reduced motion", &reducedMotion);
            ImGui::Separator();
            ImGui::Checkbox("Grid snap", &snap);
            ImGui::Checkbox("Lasso", &lasso);
            ImGui::SetNextItemWidth(160);
            ImGui::Combo("Wires", &wire, "Bezier\0Straight\0Orthogonal\0");
            ImGui::EndPopup();
        }
        if (materialPage) {
            material.Draw(theme, icons, snap, lasso, static_cast<ne::LinkStyle>(wire));
            ImGui::End();
            return;
        }
        auto graph = model.View();
        for (auto &view : model.views)
            if (view.kind == ne::NodeKind::Node)
                view.size.y = std::max(view.size.y, double(6 * ImGui::GetFrameHeightWithSpacing() + 240));
        ne::RequestBuffer out{requests};
        auto style = ne::MakeNodeStyle(theme);
        style.linkStyle = static_cast<ne::LinkStyle>(wire);
        node_gallery::StyleNodes(model.views, model, style);
        ne::EditorFrame controls;
        controls.graph = graph;
        controls.state = &state;
        controls.requests = &out;
        controls.style = style;
        controls.min = {0, 0};
        controls.max = ImGui::GetIO().DisplaySize;
        if (node_gallery::Action("add", icons, imkit::IconId::Add, "Add node (Tab)")) {
            state.press = {state.origin.x + 100, state.origin.y + 100};
            state.palette = true;
        }
        ImGui::SameLine();
        ImGui::BeginDisabled(model.undo.empty());
        if (node_gallery::Action("undo", icons, imkit::IconId::Undo, "Undo"))
            ne::QueueCommand(controls, ne::EditKind::Undo);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(model.redo.empty());
        if (node_gallery::Action("redo", icons, imkit::IconId::Redo, "Redo"))
            ne::QueueCommand(controls, ne::EditKind::Redo);
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (node_gallery::Action("frame", icons, imkit::IconId::FitView, "Frame selection / all (F)"))
            ne::FrameNodes(state, graph, state.selection,
                           {ImGui::GetIO().DisplaySize.x - 310, ImGui::GetIO().DisplaySize.y - 140});
        ImGui::SameLine();
        ne::LayoutToolbar(controls);
        ImGui::SameLine();
        if (node_gallery::Action("more", icons, imkit::IconId::More, "Graph actions and help"))
            ImGui::OpenPopup("Graph actions");
        if (ImGui::BeginPopup("Graph actions")) {
            ImGui::BeginDisabled(state.selection.empty());
            if (ImGui::MenuItem("Group"))
                ne::QueueCommand(controls, ne::EditKind::Group);
            if (ImGui::MenuItem("Make subgraph"))
                ne::QueueCommand(controls, ne::EditKind::MakeSubgraph);
            if (ImGui::MenuItem("Unpack"))
                ne::QueueCommand(controls, ne::EditKind::Ungroup);
            ImGui::EndDisabled();
            ImGui::Separator();
            ImGui::TextUnformatted(
                "Drag headers to move. Middle drag pans.\nWheel zooms. Drag an occupied input to "
                "reconnect.\nAlt-drag cuts links. Tab adds nodes. F frames selection.");
            ImGui::EndPopup();
        }
        ne::Breadcrumbs(controls, model.path);
        auto canvasSize = ImGui::GetContentRegionAvail();
        canvasSize.x = std::max(160.f, canvasSize.x - std::min(310.f, canvasSize.x * .36f));
        ne::EditorOptions options;
        options.snap = snap;
        options.lasso = lasso;
        options.size = canvasSize;
        auto f = ne::BeginEditor("demo", graph, state, out, style, options);
        if (model.data.nodes.size() > 2) {
            const auto& monitor = model.data.nodes[2].view;
            monitorHeader = {f.min.x + float((monitor.position.x - state.origin.x + 60) * state.zoom),
                             f.min.y + float((monitor.position.y - state.origin.y) * state.zoom) + style.headerHeight * .5f};
        }
        ne::DrawLinks(f);
        ne::DrawNodes(
            f,
            [](void *user, ne::EditorFrame &frame, const ne::NodeView &node) {
                auto &self = *static_cast<Demo *>(user);
                auto *n = self.model.Find(node.id);
                if (!n)
                    return;
                if (node.kind == ne::NodeKind::Group || node.kind == ne::NodeKind::Frame)
                    return;
                node_gallery::BodyTextScope bodyText(frame.style.text);
                if (node.kind == ne::NodeKind::Note) {
                    ImGui::TextWrapped("This note belongs to the host. Group nodes to move them together.");
                    return;
                }
                ImGui::SliderFloat("##value", &n->value, 0, 1);
                if (ImGui::IsItemHovered())
                    ImGui::SetTooltip("Value");
                for (auto &pin : frame.graph.pins)
                    if (pin.node == node.id)
                        ne::PinRow(frame, pin.id);
                auto outputs = self.Outputs(*n);
                ne::Preview(frame, outputs);
            },
            this);
        std::vector<ne::PaletteEntry> palette = {
            {1, "Processing", "Nodes", "Custom native ImGui contents"},
            {2, "Image preview", "Nodes", "Borrowed host texture"},
            {3, "Monitor", "Nodes", "Waveform and output comparison"},
            {4, "Note", "Organize", "Host-owned annotation"},
            {5, "Frame", "Organize", "Visual grouping frame"},
            {6, "Reroute", "Organize", "Route connections through a small node"}};
        std::vector<std::string> labels;
        labels.reserve(model.templates.size());
        for (std::size_t i = 0; i < model.templates.size(); ++i)
            labels.push_back("Template " + std::to_string(i + 1));
        for (std::size_t i = 0; i < labels.size(); ++i)
            palette.push_back({1000 + i, labels[i], "Templates", "Host-owned reusable graph"});
        for (auto &entry : palette)
            if (entry.type == 1 || entry.type == 2 || entry.type == 3 || entry.type == 6)
                entry.compatible = [](void *, ne::PinId) { return true; };
        ne::NodePalette(f, palette);
        ne::EndEditor(f);
        ImGui::SameLine();
        ImGui::BeginChild("Inspector", {0, canvasSize.y}, ImGuiChildFlags_Borders);
        ne::NodeSearch(f);
        ImGui::Separator();
        ImGui::SeparatorText("Node properties");
        auto* selected = model.Find(state.active);
        const bool applyColor = appearance.Draw(f, selected ? &selected->view : nullptr,
                                                selected ? selected->color : ImVec4{}, style.accent);
        ne::NodeInspector(f, state.active);
        if (auto *n = model.Find(state.active)) {
            ne::PropertyView value{1, "Expose value", n->valueExposed};
            ne::ExposedProperties(f, state.active, {&value, 1});
        }
        ImGui::Separator();
        ne::Diagnostics(f);
        ImGui::EndChild();
        for (auto &n : model.data.nodes)
            if (n.graph == model.graph) {
                auto outputs = Outputs(n);
                ne::DrawDetachedPreviews(f, n.view.id, outputs);
            }
        ImGui::End();
        model.Apply(out.Requests());
        if (applyColor) node_gallery::ApplyColor(model, appearance.node, appearance.draft);
    }
};
bool ModelSmoke() {
    Model m;
    m.Init();
    auto g = m.View();
    auto id = g.nodes[0].id;
    ne::EditRequest r;
    r.graph = g.id;
    r.revision = g.revision;
    r.operation = 1;
    r.node = id;
    r.kind = ne::EditKind::Move;
    r.before = g.nodes[0].position;
    r.after = {90, 100};
    m.Apply({&r, 1});
    if (m.Find(id)->view.position.x != 90)
        return false;
    r.kind = ne::EditKind::Undo;
    r.revision = m.revision;
    m.Apply({&r, 1});
    if (m.Find(id)->view.position.x != 40)
        return false;
    r.kind = ne::EditKind::Redo;
    r.revision = m.revision;
    m.Apply({&r, 1});
    if (m.Find(id)->view.position.x != 90)
        return false;
    r.kind = ne::EditKind::Duplicate;
    r.revision = m.revision;
    m.Apply({&r, 1});
    if (m.data.nodes.size() != 4)
        return false;
    auto groupRequests = std::array<ne::EditRequest, 2>{r, r};
    for (auto &e : groupRequests) {
        e.kind = ne::EditKind::MakeSubgraph;
        e.revision = m.revision;
        e.operationSize = 2;
    }
    groupRequests[0].node = m.data.nodes[0].view.id;
    groupRequests[1].node = m.data.nodes[1].view.id;
    const auto linkCount = m.data.links.size();
    m.Apply(groupRequests);
    auto group = m.data.nodes.front().view.id;
    if (!m.Find(group)->view.childGraph)
        return false;
    r.node = group;
    r.kind = ne::EditKind::Ungroup;
    r.revision = m.revision;
    m.Apply({&r, 1});
    if (m.data.nodes.size() != 4 || m.data.links.size() != linkCount)
        return false;
    r.node = id;
    r.kind = ne::EditKind::DeleteNode;
    r.revision = m.revision;
    m.Apply({&r, 1});
    return !m.Find(id);
}
} // namespace
int main(int argc, char **argv) {
    bool smoke = argc > 1 && std::string_view(argv[1]) == "--smoke";
    const bool verify = argc > 1 && std::string_view(argv[1]) == "--verify-node-actions";
    std::filesystem::path captureDirectory;
    int captureWidth = 1440, captureHeight = 810;
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--capture-gif" && i + 1 < argc)
            captureDirectory = argv[++i];
        else if (std::string_view(argv[i]) == "--width" && i + 1 < argc)
            captureWidth = std::clamp(std::stoi(argv[++i]), 640, 7680);
        else if (std::string_view(argv[i]) == "--height" && i + 1 < argc)
            captureHeight = std::clamp(std::stoi(argv[++i]), 480, 4320);
    }
#ifdef IMKIT_NODE_EDITOR_METAL
    if (!captureDirectory.empty()) {
        std::fprintf(stderr, "--capture-gif requires the OpenGL companion host\n");
        return 2;
    }
#endif
    const bool capture = !captureDirectory.empty();
#ifdef _MSC_VER
    if (verify) {
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    }
#endif
    if (!ModelSmoke()) {
        std::fprintf(stderr, "host model smoke failed\n");
        return 1;
    }
#ifndef IMKIT_NODE_EDITOR_METAL
    const auto com = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
#endif
    if (!glfwInit())
        return 2;
    glfwWindowHint(GLFW_VISIBLE, smoke || verify || capture ? GLFW_FALSE : GLFW_TRUE);
#ifdef IMKIT_NODE_EDITOR_METAL
    glfwWindowHint(GLFW_CLIENT_API,GLFW_NO_API);
#endif
    auto has = [&](std::string_view option) {
        for (int i = 1; i < argc; ++i)
            if (argv[i] == option)
                return true;
        return false;
    };
    auto *window = glfwCreateWindow(capture ? captureWidth : has("--narrow") ? 780 : 1400,
                                    capture ? captureHeight : 900, "ModernKIT Node Studio", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 3;
    }
#ifndef IMKIT_NODE_EDITOR_METAL
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
#endif
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr;
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard|ImGuiConfigFlags_DockingEnable;
    if(!smoke && !verify && !capture) ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigDpiScaleFonts=true;
    ImGui::GetIO().ConfigDpiScaleViewports=true;
    // Host-owned system font: used locally, never bundled with the library.
#ifdef IMKIT_NODE_EDITOR_METAL
    const std::array<const char*,2> systemFonts{"/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc","/System/Library/Fonts/Helvetica.ttc"};
#else
    const std::array<const char*,3> systemFonts{"C:/Windows/Fonts/YuGothR.ttc","C:/Windows/Fonts/meiryo.ttc","C:/Windows/Fonts/segoeui.ttf"};
#endif
    for (const char *font : systemFonts)
        if (std::filesystem::exists(font)) {
            ImGui::GetIO().FontDefault = ImGui::GetIO().Fonts->AddFontFromFileTTF(font, 18);
            break;
        }
#ifdef IMKIT_NODE_EDITOR_METAL
    gMetalDevice=MTLCreateSystemDefaultDevice(); id<MTLCommandQueue> commandQueue=[gMetalDevice newCommandQueue];
    NSWindow *nativeWindow=glfwGetCocoaWindow(window); CAMetalLayer *layer=[CAMetalLayer layer]; layer.device=gMetalDevice; layer.pixelFormat=MTLPixelFormatBGRA8Unorm; nativeWindow.contentView.layer=layer; nativeWindow.contentView.wantsLayer=YES;
    ImGui::GetIO().ConfigFlags|=ImGuiConfigFlags_DockingEnable|ImGuiConfigFlags_ViewportsEnable;
    ImGui::GetIO().ConfigDpiScaleFonts=true; ImGui::GetIO().ConfigDpiScaleViewports=true;
    ImGui_ImplGlfw_InitForOther(window,true); ImGui_ImplMetal_Init(gMetalDevice);
#else
    ImGui_ImplGlfw_InitForOpenGL(window, true); ImGui_ImplOpenGL3_Init("#version 130");
#endif
    auto demo = std::make_unique<Demo>();
    demo->Init();
    demo->materialPage = !has("--studio");
    if (verify) demo->materialPage = false;
    demo->density = has("--touch") ? 2 : has("--compact") ? 0 : 1;
    demo->highContrast = has("--contrast");
    if (has("--narrow"))
        demo->material.state.zoom = .7;
    if (has("--japanese")) {
        demo->material.model.data.nodes[1].name = "長い名前のマテリアル・プレビュー設定";
        demo->material.model.data.pins[2].name = "未接続入力の長い日本語ラベル";
    }
    if (capture) {
        std::filesystem::create_directories(captureDirectory);
        demo->materialPage = true;
        demo->material.state.zoom = .72;
        demo->material.state.origin = {-8, -18};
    }
    int frames = 0;
    int collapseChanges = 0;
    bool collapsed = false;
    while (!glfwWindowShouldClose(window) && (!smoke || frames < 5) && (!verify || frames < 724) &&
           (!capture || frames < 56)) {
        glfwPollEvents();
#ifdef IMKIT_NODE_EDITOR_METAL
        @autoreleasepool {
        int metalWidth=0,metalHeight=0;glfwGetFramebufferSize(window,&metalWidth,&metalHeight);layer.drawableSize=CGSizeMake(metalWidth,metalHeight);
        id<CAMetalDrawable> drawable=[layer nextDrawable]; if(!drawable) continue;
        id<MTLCommandBuffer> commandBuffer=[commandQueue commandBuffer]; MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=drawable.texture;pass.colorAttachments[0].loadAction=MTLLoadActionClear;pass.colorAttachments[0].storeAction=MTLStoreActionStore;pass.colorAttachments[0].clearColor=MTLClearColorMake(.06,.07,.09,1);
        ImGui_ImplMetal_NewFrame(pass);
#else
        ImGui_ImplOpenGL3_NewFrame();
#endif
        ImGui_ImplGlfw_NewFrame();
        if (capture) {
            const auto transition = [](int value, int first, int last) {
                return std::clamp(static_cast<float>(value - first) / static_cast<float>(last - first), 0.f, 1.f);
            };
            const float zoomIn = transition(frames, 8, 18);
            const float panOut = transition(frames, 38, 48);
            demo->material.state.zoom = .72 + .12 * zoomIn - .08 * panOut;
            demo->material.state.origin = {-8.0 - 52.0 * zoomIn - 48.0 * panOut,
                                           -18.0 - 18.0 * zoomIn + 12.0 * panOut};
            if (frames == 18) {
                auto node = std::find_if(demo->material.model.data.nodes.begin(),
                                         demo->material.model.data.nodes.end(),
                                         [](const auto &entry) { return entry.type == 7; });
                if (node != demo->material.model.data.nodes.end()) {
                    demo->material.model.AddPin(node->view.id, ne::PinKind::Input, 2, "Coat weight");
                    ++demo->material.model.revision;
                    demo->material.model.Resize();
                }
            }
            if (frames == 34) {
                auto mix = std::find_if(demo->material.model.data.nodes.begin(),
                                        demo->material.model.data.nodes.end(),
                                        [](const auto &entry) { return entry.type == 7; });
                auto source = std::find_if(demo->material.model.data.nodes.begin(),
                                           demo->material.model.data.nodes.end(),
                                           [](const auto &entry) { return entry.type == 1; });
                if (mix != demo->material.model.data.nodes.end() &&
                    source != demo->material.model.data.nodes.end()) {
                    auto from = demo->material.model.Socket(source->view.id, ne::PinKind::Output, 1);
                    auto to = std::find_if(demo->material.model.data.pins.rbegin(),
                                           demo->material.model.data.pins.rend(), [&](const auto &pin) {
                                               return pin.view.node == mix->view.id && pin.name == "Coat weight";
                                           });
                    if (to != demo->material.model.data.pins.rend() &&
                        demo->material.model.Connect(from, to->view.id)) {
                        ++demo->material.model.revision;
                        demo->material.model.Evaluate();
                    }
                }
            }
        }
        if (verify) {
            auto& io = ImGui::GetIO();
            io.DeltaTime = 1.f / 60;
            io.ConfigInputTrickleEventQueue = false;
            if (frames >= 4) {
                io.AddMousePosEvent(demo->monitorHeader.x, demo->monitorHeader.y);
                const int burst = (frames - 4) % 24;
                // Leave a double-click timeout between bursts so each pair toggles again.
                io.AddMouseButtonEvent(0, burst == 0 || burst == 2);
            }
        }
        ImGui::NewFrame();
        demo->Draw();
        if (verify && collapsed != demo->model.data.nodes[2].view.collapsed) {
            collapsed = demo->model.data.nodes[2].view.collapsed;
            ++collapseChanges;
        }
        ImGui::Render();
        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
#ifdef IMKIT_NODE_EDITOR_METAL
        id<MTLRenderCommandEncoder> encoder=[commandBuffer renderCommandEncoderWithDescriptor:pass]; ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(),commandBuffer,encoder); [encoder endEncoding];
        if(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_ViewportsEnable){ImGui::UpdatePlatformWindows();ImGui::RenderPlatformWindowsDefault();}
        [commandBuffer presentDrawable:drawable];[commandBuffer commit];
#else
        glViewport(0, 0, w, h);
        glClearColor(.06f, .07f, .09f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        if(ImGui::GetIO().ConfigFlags&ImGuiConfigFlags_ViewportsEnable){ImGui::UpdatePlatformWindows();ImGui::RenderPlatformWindowsDefault();glfwMakeContextCurrent(window);}
#endif
        ++frames;
#ifndef IMKIT_NODE_EDITOR_METAL
        if (smoke && frames == 4 && argc > 2)
            imkit::design::SaveBackbuffer(argv[2], w, h);
        if (capture) {
            char name[32];
            std::snprintf(name, sizeof(name), "frame-%03d.png", frames - 1);
            imkit::design::SaveBackbuffer(captureDirectory / name, w, h);
        }
        glfwSwapBuffers(window);
#else
        }
#endif
    }
#ifndef IMKIT_NODE_EDITOR_METAL
    if (capture) {
        int framebufferWidth=0,framebufferHeight=0;
        glfwGetFramebufferSize(window,&framebufferWidth,&framebufferHeight);
        std::ofstream metadata(captureDirectory / "capture.txt");
        metadata << "scenario=node-editor\nframes=" << frames
                 << "\nsize=" << framebufferWidth << "x" << framebufferHeight
                 << "\nsource=native OpenGL backbuffer; deterministic host-owned "
                    "zoom, pan, dynamic socket, link and preview state\n";
    }
#endif
#ifdef IMKIT_NODE_EDITOR_METAL
    demo->icons.Clear(); demo->texture=nil; for(auto &texture:demo->iconTextures) texture=nil; ImGui_ImplMetal_Shutdown();
#else
    glDeleteTextures(1, &demo->texture);
    glDeleteTextures(int(demo->iconTextures.size()), demo->iconTextures.data());
    ImGui_ImplOpenGL3_Shutdown();
#endif
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
#ifndef IMKIT_NODE_EDITOR_METAL
    if (SUCCEEDED(com)) CoUninitialize();
#endif
    if (smoke)
        std::printf("node studio: host edit/undo/subgraph model and 5 native frames passed\n");
    if (capture)
        std::printf("node studio: captured %d deterministic native frames at %dx%d\n",
                    frames,captureWidth,captureHeight);
    if (verify) {
        std::printf("Output monitor: 60 public-IO clicks, %d collapse/expand transitions\n", collapseChanges);
        return collapseChanges >= 20 ? 0 : 1;
    }
    return 0;
}
