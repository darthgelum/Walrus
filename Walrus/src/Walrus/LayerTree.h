#ifndef WALRUS_LAYER_TREE_H
#define WALRUS_LAYER_TREE_H

#include "Layer.h"
#include <ftl/task_scheduler.h>
#include <ftl/wait_group.h>
#include <memory>
#include <vector>
#include <string>

namespace Walrus {

	/**
	 * @brief A node in the layer tree that can have multiple children
	 * 
	 * LayerTreeNode represents a single node in an n-ary tree structure where:
	 * - Each node contains a Layer to be updated
	 * - Nodes at the same tree level can be updated in parallel 
	 * - If a node has children, it waits for all children to complete before continuing
	 * - Tree traversal uses fibers for parallel execution
	 */
	class LayerTreeNode
	{
	public:
		LayerTreeNode(std::shared_ptr<Layer> layer, const std::string& name = "");
		~LayerTreeNode() = default;

		// Tree structure management
		void AddChild(std::shared_ptr<LayerTreeNode> child);
		void RemoveChild(std::shared_ptr<LayerTreeNode> child);
		void RemoveChild(const std::string& name);
		std::shared_ptr<LayerTreeNode> FindChild(const std::string& name) const;
		
		// Getters
		std::shared_ptr<Layer> GetLayer() const { return m_Layer; }
		const std::vector<std::shared_ptr<LayerTreeNode>>& GetChildren() const { return m_Children; }
		const std::string& GetName() const { return m_Name; }
		bool HasChildren() const { return !m_Children.empty(); }
		size_t GetChildCount() const { return m_Children.size(); }
		
		// Tree traversal and update methods
		void UpdateSubtree(float timestep, ftl::TaskScheduler* scheduler);
		void UpdateNode(float timestep);
		
		// Utility methods
		void PrintTree(int depth = 0) const;
		size_t GetTotalNodeCount() const;
		int GetMaxDepth() const;

	private:
		std::shared_ptr<Layer> m_Layer;
		std::vector<std::shared_ptr<LayerTreeNode>> m_Children;
		std::string m_Name;
		
		// Internal update methods
		void UpdateChildrenInParallel(float timestep, ftl::TaskScheduler* scheduler);
	};

	/**
	 * @brief Manages a tree of layers with hierarchical parallel updates
	 * 
	 * LayerTree provides:
	 * - Root-level management of layer trees
	 * - Parallel execution of sibling nodes using fibers
	 * - Sequential execution of parent->children relationships
	 * - Tree manipulation and query operations
	 */
	class LayerTree
	{
	public:
		LayerTree() = default;
		~LayerTree() = default;

		// Root node management
		void AddRootNode(std::shared_ptr<LayerTreeNode> node);
		void RemoveRootNode(std::shared_ptr<LayerTreeNode> node);
		void RemoveRootNode(const std::string& name);
		std::shared_ptr<LayerTreeNode> FindRootNode(const std::string& name) const;
		
		// Convenience methods for creating nodes
		std::shared_ptr<LayerTreeNode> CreateRootNode(std::shared_ptr<Layer> layer, const std::string& name = "");
		std::shared_ptr<LayerTreeNode> CreateChildNode(const std::string& parentName, std::shared_ptr<Layer> layer, const std::string& childName = "");
		
		// Tree operations
		void UpdateTree(float timestep, ftl::TaskScheduler* scheduler);
		void Clear();
		
		// Query operations
		const std::vector<std::shared_ptr<LayerTreeNode>>& GetRootNodes() const { return m_RootNodes; }
		bool IsEmpty() const { return m_RootNodes.empty(); }
		size_t GetRootNodeCount() const { return m_RootNodes.size(); }
		size_t GetTotalNodeCount() const;
		int GetMaxDepth() const;
		
		// Tree traversal operations
		std::shared_ptr<LayerTreeNode> FindNode(const std::string& name) const;
		std::vector<std::shared_ptr<LayerTreeNode>> FindAllNodes(const std::string& name) const;
		
		// Utility methods
		void PrintTree() const;
		
		// Event handlers for tree lifecycle
		void OnAttachAll();
		void OnDetachAll();

	private:
		std::vector<std::shared_ptr<LayerTreeNode>> m_RootNodes;
		
		// Internal helper methods
		void UpdateRootNodesInParallel(float timestep, ftl::TaskScheduler* scheduler);
		void FindNodeRecursive(const std::string& name, std::shared_ptr<LayerTreeNode> current, std::vector<std::shared_ptr<LayerTreeNode>>& results) const;
	};

	/**
	 * @brief Builder pattern for constructing layer trees
	 * 
	 * LayerTreeBuilder provides a fluent API for building complex layer hierarchies:
	 * 
	 * Example usage:
	 * auto tree = LayerTreeBuilder()
	 *     .Root(myRootLayer, "root")
	 *         .Child(myChildLayer1, "child1")
	 *             .Child(myGrandchildLayer, "grandchild")
	 *         .Back()
	 *         .Child(myChildLayer2, "child2")
	 *     .Build();
	 */
	class LayerTreeBuilder
	{
	public:
		LayerTreeBuilder();
		~LayerTreeBuilder() = default;

		// Root node creation
		LayerTreeBuilder& Root(std::shared_ptr<Layer> layer, const std::string& name = "");
		
		// Child node creation (adds to current context)
		LayerTreeBuilder& Child(std::shared_ptr<Layer> layer, const std::string& name = "");
		
		// Navigation
		LayerTreeBuilder& Back();  // Move up one level in the tree
		LayerTreeBuilder& ToRoot(); // Move to root level
		LayerTreeBuilder& To(const std::string& name); // Move to specific named node
		
		// Build the final tree
		std::shared_ptr<LayerTree> Build();

	private:
		std::shared_ptr<LayerTree> m_Tree;
		std::vector<std::shared_ptr<LayerTreeNode>> m_Context; // Stack of current nodes for nested building
		
		std::shared_ptr<LayerTreeNode> GetCurrentContext();
	};

}

#endif // WALRUS_LAYER_TREE_H