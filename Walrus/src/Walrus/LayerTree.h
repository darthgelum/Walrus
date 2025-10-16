#ifndef WALRUS_LAYER_TREE_H
#define WALRUS_LAYER_TREE_H

#include "Layer.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Walrus {

class LayerNode : public std::enable_shared_from_this<LayerNode> {
private:
  std::string id;
  std::shared_ptr<Walrus::Layer> layer;
  std::weak_ptr<LayerNode> parent;
  std::vector<std::shared_ptr<LayerNode>> children;

  static std::unordered_map<std::string, std::weak_ptr<LayerNode>>
      layer_search_map;

public:
  LayerNode(std::shared_ptr<LayerNode> parent,
            std::shared_ptr<Walrus::Layer> layer, const std::string &id = "");

  std::shared_ptr<LayerNode> AddChild(std::shared_ptr<Walrus::Layer> layer,
                                      const std::string &id = "");

  std::shared_ptr<Walrus::Layer> GetLayer() const;
  std::shared_ptr<LayerNode> GetParent() const;

  void OnUpdate(float ts);
  void OnDetach();

  static std::shared_ptr<LayerNode> FindByName(const std::string &name);
  // static void Remove(const std::string &name);
  // static std::shared_ptr<LayerNode> GetRoot();
};

// extern std::shared_ptr<LayerNode> Root;

} // namespace Walrus

#endif // WALRUS_LAYER_TREE_H
