#include "LayerTree.h"
#include <algorithm>
#include <iostream>
#include <stack>

namespace Walrus {

	// ===============================
	// LayerTreeNode Implementation
	// ===============================

	LayerTreeNode::LayerTreeNode(std::shared_ptr<Layer> layer, const std::string& name)
		: m_Layer(layer), m_Name(name)
	{
		if (m_Name.empty() && m_Layer) {
			// Generate a default name if none provided
			m_Name = "Layer_" + std::to_string(reinterpret_cast<uintptr_t>(m_Layer.get()));
		}
	}

	void LayerTreeNode::AddChild(std::shared_ptr<LayerTreeNode> child)
	{
		if (child) {
			m_Children.push_back(child);
		}
	}

	void LayerTreeNode::RemoveChild(std::shared_ptr<LayerTreeNode> child)
	{
		auto it = std::find(m_Children.begin(), m_Children.end(), child);
		if (it != m_Children.end()) {
			m_Children.erase(it);
		}
	}

	void LayerTreeNode::RemoveChild(const std::string& name)
	{
		auto it = std::find_if(m_Children.begin(), m_Children.end(),
			[&name](const std::shared_ptr<LayerTreeNode>& node) {
				return node->GetName() == name;
			});
		
		if (it != m_Children.end()) {
			m_Children.erase(it);
		}
	}

	std::shared_ptr<LayerTreeNode> LayerTreeNode::FindChild(const std::string& name) const
	{
		auto it = std::find_if(m_Children.begin(), m_Children.end(),
			[&name](const std::shared_ptr<LayerTreeNode>& node) {
				return node->GetName() == name;
			});
		
		return (it != m_Children.end()) ? *it : nullptr;
	}

	void LayerTreeNode::UpdateSubtree(float timestep, ftl::TaskScheduler* scheduler)
	{
		// First update this node
		UpdateNode(timestep);
		
		// Then update all children in parallel if they exist
		if (HasChildren()) {
			UpdateChildrenInParallel(timestep, scheduler);
		}
	}

	void LayerTreeNode::UpdateNode(float timestep)
	{
		if (m_Layer) {
			m_Layer->OnUpdate(timestep);
		}
	}

	void LayerTreeNode::UpdateChildrenInParallel(float timestep, ftl::TaskScheduler* scheduler)
	{
		if (m_Children.empty() || !scheduler) {
			return;
		}

		// Create a wait group for all child subtree updates
		ftl::WaitGroup wg(scheduler);
		
		// Create tasks for each child subtree update
		std::vector<ftl::Task> childTasks;
		childTasks.reserve(m_Children.size());
		
		// Structure to pass both the node and timestep to the fiber
		struct UpdateData {
			LayerTreeNode* node;
			float timestep;
			ftl::TaskScheduler* scheduler;
		};
		
		std::vector<UpdateData> updateDataList;
		updateDataList.reserve(m_Children.size());
		
		for (const auto& child : m_Children) {
			updateDataList.push_back({child.get(), timestep, scheduler});
			
			childTasks.push_back({
				[](ftl::TaskScheduler*, void* arg) {
					auto* data = static_cast<UpdateData*>(arg);
					data->node->UpdateSubtree(data->timestep, data->scheduler);
				},
				&updateDataList.back()
			});
		}
		
		// Submit all child subtree update tasks
		scheduler->AddTasks(
			static_cast<uint32_t>(childTasks.size()),
			childTasks.data(),
			ftl::TaskPriority::Normal,
			&wg
		);
		
		// Wait for all child subtrees to complete
		// pinToCurrentThread = false allows fibers to resume on any thread for better performance
		wg.Wait(false);
	}

	void LayerTreeNode::PrintTree(int depth) const
	{
		std::string indent(depth * 2, ' ');
		std::cout << indent << "- " << m_Name;
		if (m_Layer) {
			std::cout << " (Layer: " << typeid(*m_Layer).name() << ")";
		}
		std::cout << std::endl;
		
		for (const auto& child : m_Children) {
			child->PrintTree(depth + 1);
		}
	}

	size_t LayerTreeNode::GetTotalNodeCount() const
	{
		size_t count = 1; // Count this node
		for (const auto& child : m_Children) {
			count += child->GetTotalNodeCount();
		}
		return count;
	}

	int LayerTreeNode::GetMaxDepth() const
	{
		if (m_Children.empty()) {
			return 1;
		}
		
		int maxChildDepth = 0;
		for (const auto& child : m_Children) {
			maxChildDepth = std::max(maxChildDepth, child->GetMaxDepth());
		}
		return 1 + maxChildDepth;
	}

	// ===============================
	// LayerTree Implementation
	// ===============================

	void LayerTree::AddRootNode(std::shared_ptr<LayerTreeNode> node)
	{
		if (node) {
			m_RootNodes.push_back(node);
		}
	}

	void LayerTree::RemoveRootNode(std::shared_ptr<LayerTreeNode> node)
	{
		auto it = std::find(m_RootNodes.begin(), m_RootNodes.end(), node);
		if (it != m_RootNodes.end()) {
			m_RootNodes.erase(it);
		}
	}

	void LayerTree::RemoveRootNode(const std::string& name)
	{
		auto it = std::find_if(m_RootNodes.begin(), m_RootNodes.end(),
			[&name](const std::shared_ptr<LayerTreeNode>& node) {
				return node->GetName() == name;
			});
		
		if (it != m_RootNodes.end()) {
			m_RootNodes.erase(it);
		}
	}

	std::shared_ptr<LayerTreeNode> LayerTree::FindRootNode(const std::string& name) const
	{
		auto it = std::find_if(m_RootNodes.begin(), m_RootNodes.end(),
			[&name](const std::shared_ptr<LayerTreeNode>& node) {
				return node->GetName() == name;
			});
		
		return (it != m_RootNodes.end()) ? *it : nullptr;
	}

	std::shared_ptr<LayerTreeNode> LayerTree::CreateRootNode(std::shared_ptr<Layer> layer, const std::string& name)
	{
		auto node = std::make_shared<LayerTreeNode>(layer, name);
		AddRootNode(node);
		return node;
	}

	std::shared_ptr<LayerTreeNode> LayerTree::CreateChildNode(const std::string& parentName, std::shared_ptr<Layer> layer, const std::string& childName)
	{
		auto parent = FindNode(parentName);
		if (parent) {
			auto child = std::make_shared<LayerTreeNode>(layer, childName);
			parent->AddChild(child);
			return child;
		}
		return nullptr;
	}

	void LayerTree::UpdateTree(float timestep, ftl::TaskScheduler* scheduler)
	{
		if (m_RootNodes.empty() || !scheduler) {
			return;
		}
		
		// Update all root nodes in parallel
		UpdateRootNodesInParallel(timestep, scheduler);
	}

	void LayerTree::Clear()
	{
		m_RootNodes.clear();
	}

	size_t LayerTree::GetTotalNodeCount() const
	{
		size_t count = 0;
		for (const auto& root : m_RootNodes) {
			count += root->GetTotalNodeCount();
		}
		return count;
	}

	int LayerTree::GetMaxDepth() const
	{
		int maxDepth = 0;
		for (const auto& root : m_RootNodes) {
			maxDepth = std::max(maxDepth, root->GetMaxDepth());
		}
		return maxDepth;
	}

	std::shared_ptr<LayerTreeNode> LayerTree::FindNode(const std::string& name) const
	{
		std::vector<std::shared_ptr<LayerTreeNode>> results;
		for (const auto& root : m_RootNodes) {
			FindNodeRecursive(name, root, results);
			if (!results.empty()) {
				return results[0]; // Return first match
			}
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<LayerTreeNode>> LayerTree::FindAllNodes(const std::string& name) const
	{
		std::vector<std::shared_ptr<LayerTreeNode>> results;
		for (const auto& root : m_RootNodes) {
			FindNodeRecursive(name, root, results);
		}
		return results;
	}

	void LayerTree::PrintTree() const
	{
		std::cout << "LayerTree Structure:" << std::endl;
		std::cout << "Total nodes: " << GetTotalNodeCount() << std::endl;
		std::cout << "Max depth: " << GetMaxDepth() << std::endl;
		std::cout << "Root nodes: " << m_RootNodes.size() << std::endl;
		std::cout << "================================" << std::endl;
		
		for (const auto& root : m_RootNodes) {
			root->PrintTree(0);
		}
		std::cout << "================================" << std::endl;
	}

	void LayerTree::OnAttachAll()
	{
		// Traverse the tree and call OnAttach on all layers
		std::stack<std::shared_ptr<LayerTreeNode>> nodeStack;
		
		// Add all root nodes to the stack
		for (const auto& root : m_RootNodes) {
			nodeStack.push(root);
		}
		
		// Process all nodes in the tree
		while (!nodeStack.empty()) {
			auto current = nodeStack.top();
			nodeStack.pop();
			
			// Attach the current layer
			if (current->GetLayer()) {
				current->GetLayer()->OnAttach();
			}
			
			// Add all children to the stack
			for (const auto& child : current->GetChildren()) {
				nodeStack.push(child);
			}
		}
	}

	void LayerTree::OnDetachAll()
	{
		// Traverse the tree and call OnDetach on all layers
		std::stack<std::shared_ptr<LayerTreeNode>> nodeStack;
		
		// Add all root nodes to the stack
		for (const auto& root : m_RootNodes) {
			nodeStack.push(root);
		}
		
		// Process all nodes in the tree
		while (!nodeStack.empty()) {
			auto current = nodeStack.top();
			nodeStack.pop();
			
			// Detach the current layer
			if (current->GetLayer()) {
				current->GetLayer()->OnDetach();
			}
			
			// Add all children to the stack
			for (const auto& child : current->GetChildren()) {
				nodeStack.push(child);
			}
		}
	}

	void LayerTree::UpdateRootNodesInParallel(float timestep, ftl::TaskScheduler* scheduler)
	{
		// Create a wait group for all root node subtree updates
		ftl::WaitGroup wg(scheduler);
		
		// Create tasks for each root node subtree update
		std::vector<ftl::Task> rootTasks;
		rootTasks.reserve(m_RootNodes.size());
		
		// Structure to pass both the node and timestep to the fiber
		struct UpdateData {
			LayerTreeNode* node;
			float timestep;
			ftl::TaskScheduler* scheduler;
		};
		
		std::vector<UpdateData> updateDataList;
		updateDataList.reserve(m_RootNodes.size());
		
		for (const auto& root : m_RootNodes) {
			updateDataList.push_back({root.get(), timestep, scheduler});
			
			rootTasks.push_back({
				[](ftl::TaskScheduler*, void* arg) {
					auto* data = static_cast<UpdateData*>(arg);
					data->node->UpdateSubtree(data->timestep, data->scheduler);
				},
				&updateDataList.back()
			});
		}
		
		// Submit all root subtree update tasks
		scheduler->AddTasks(
			static_cast<uint32_t>(rootTasks.size()),
			rootTasks.data(),
			ftl::TaskPriority::Normal,
			&wg
		);
		
		// Wait for all root subtrees to complete
		// pinToCurrentThread = false allows fibers to resume on any thread for better performance
		wg.Wait(false);
	}

	void LayerTree::FindNodeRecursive(const std::string& name, std::shared_ptr<LayerTreeNode> current, std::vector<std::shared_ptr<LayerTreeNode>>& results) const
	{
		if (current->GetName() == name) {
			results.push_back(current);
		}
		
		for (const auto& child : current->GetChildren()) {
			FindNodeRecursive(name, child, results);
		}
	}

	// ===============================
	// LayerTreeBuilder Implementation
	// ===============================

	LayerTreeBuilder::LayerTreeBuilder()
		: m_Tree(std::make_shared<LayerTree>())
	{
	}

	LayerTreeBuilder& LayerTreeBuilder::Root(std::shared_ptr<Layer> layer, const std::string& name)
	{
		auto rootNode = m_Tree->CreateRootNode(layer, name);
		m_Context.clear(); // Clear any existing context
		m_Context.push_back(rootNode); // Root becomes the current context
		return *this;
	}

	LayerTreeBuilder& LayerTreeBuilder::Child(std::shared_ptr<Layer> layer, const std::string& name)
	{
		auto current = GetCurrentContext();
		if (current) {
			auto child = std::make_shared<LayerTreeNode>(layer, name);
			current->AddChild(child);
			m_Context.push_back(child); // Child becomes the new current context
		}
		return *this;
	}

	LayerTreeBuilder& LayerTreeBuilder::Back()
	{
		if (m_Context.size() > 1) {
			m_Context.pop_back(); // Move up one level
		}
		return *this;
	}

	LayerTreeBuilder& LayerTreeBuilder::ToRoot()
	{
		if (!m_Context.empty()) {
			// Keep only the first (root) element
			auto root = m_Context[0];
			m_Context.clear();
			m_Context.push_back(root);
		}
		return *this;
	}

	LayerTreeBuilder& LayerTreeBuilder::To(const std::string& name)
	{
		auto node = m_Tree->FindNode(name);
		if (node) {
			m_Context.clear();
			m_Context.push_back(node);
		}
		return *this;
	}

	std::shared_ptr<LayerTree> LayerTreeBuilder::Build()
	{
		auto result = m_Tree;
		m_Tree = std::make_shared<LayerTree>(); // Reset for potential reuse
		m_Context.clear();
		return result;
	}

	std::shared_ptr<LayerTreeNode> LayerTreeBuilder::GetCurrentContext()
	{
		return m_Context.empty() ? nullptr : m_Context.back();
	}

}