#include "LayerTree.h"

using namespace Walrus;

std::unordered_map<std::string, std::weak_ptr<LayerNode>>
    LayerNode::layer_search_map;

LayerNode::LayerNode(std::shared_ptr<LayerNode> parent,
                     std::shared_ptr<Walrus::Layer> layer,
                     const std::string &id)
    : parent(parent), layer(std::move(layer)), id(id) {}

std::shared_ptr<LayerNode>
LayerNode::AddChild(std::shared_ptr<Walrus::Layer> layer,
                    const std::string &id) {
  auto self = shared_from_this();
  auto child = std::make_shared<LayerNode>(self, std::move(layer), id);
  children.emplace_back(child);

  if (!id.empty()) {
    layer_search_map[id] = child;
  }
  return child;
}

std::shared_ptr<Walrus::Layer> LayerNode::GetLayer() const { return layer; }

std::shared_ptr<LayerNode> LayerNode::GetParent() const {
  return parent.lock();
}

void LayerNode::OnUpdate(float ts) {
  for (auto &child : children) {
    if (child)
      child->OnUpdate(ts);
  }
  if (layer)
    layer->OnUpdate(ts);
}

void LayerNode::OnDetach() {
  for (auto &child : children) {
    if (child)
      child->OnDetach();
  }
  if (layer)
    layer->OnDetach();
}

std::shared_ptr<LayerNode> LayerNode::FindByName(const std::string &name) {
  auto it = layer_search_map.find(name);
  if (it != layer_search_map.end()) {
    return it->second.lock();
  }
  return nullptr;
}

// void LayerNode::Remove(const std::string &name) {
//   layer_search_map.erase(name);
// }

// std::shared_ptr<LayerNode> LayerNode::GetRoot() {
//   if (!root) {
//     root = std::make_shared<LayerNode>(nullptr, nullptr,
//     "walrus_root_layer_");
//   }
//   return root;
// }
