#include "pch.h"
#include "dependency_resolver.h"

#include "engine/utils/logger/logger.h"
#include "engine/core/exception/base_exception.h"

class kfe::DependencyResolver::Impl
{
public:
	_NODISCARD _Check_return_
	std::vector<IManager*> GraphSort();

	void GraphDFS(
		_In_	IManager* node,
		_Inout_ std::unordered_map<IManager*, bool>& visited,
		_Inout_ std::unordered_map<IManager*, bool>& stack,
		_Inout_ std::vector<IManager*>& sorted
	);

	std::unordered_map<IManager*, bool>					  m_registeredManagers{};
	std::unordered_map<IManager*, std::vector<IManager*>> m_connections		  {};
	std::vector<IManager*>								  m_managerNames	  {};
	std::vector<IManager*>								  m_initOrder		  {};
};

_Use_decl_annotations_
std::vector<kfe::IManager*> kfe::DependencyResolver::Impl::GraphSort()
{
    std::unordered_map<IManager*, bool> visited;
    std::unordered_map<IManager*, bool> stack;

    std::vector<IManager*> post;
    std::vector<IManager*> sorted;

    for (auto it = m_registeredManagers.begin(); it != m_registeredManagers.end(); ++it)
    {
        IManager* node = it->first;
        if (!visited.contains(node))
        {
            GraphDFS(node, visited, stack, post);

            if (post.empty())
            {
                sorted.clear();
                return sorted;
            }
        }
    }

    sorted.assign(post.rbegin(), post.rend());
    return sorted;
}

_Use_decl_annotations_
void kfe::DependencyResolver::Impl::GraphDFS(
    _In_    IManager* node,
    _Inout_ std::unordered_map<IManager*, bool>& visited,
    _Inout_ std::unordered_map<IManager*, bool>& stack,
    _Inout_ std::vector<IManager*>& post)
{
    if (stack.contains(node))
    {
        post.clear();
        return;
    }

    if (visited.contains(node))
    {
        return;
    }

    stack[node] = true;
    visited[node] = true;

    if (m_connections.contains(node))
    {
        auto& deps = m_connections.at(node);
        for (auto it = deps.begin(); it != deps.end(); ++it)
        {
            IManager* early = *it;
            GraphDFS(early, visited, stack, post);

            if (post.empty())
            {
                return;
            }
        }
    }

    stack.erase(node);
    post.push_back(node);
}

_Use_decl_annotations_
kfe::DependencyResolver::DependencyResolver()
	: m_impl(std::make_unique<kfe::DependencyResolver::Impl>())
{}

void kfe::DependencyResolver::Register(kfe::IManager* instance)
{
	if (instance && !m_impl->m_registeredManagers.contains(instance))
	{
		m_impl->m_registeredManagers[instance] = true;
	}
}

void kfe::DependencyResolver::Clear()
{
	m_impl->m_connections		.clear();
	m_impl->m_initOrder			.clear();
	m_impl->m_managerNames		.clear();
	m_impl->m_registeredManagers.clear();
}

_Use_decl_annotations_
bool kfe::DependencyResolver::Init()
{
    m_impl->m_initOrder = m_impl->GraphSort();

    if (m_impl->m_initOrder.empty())
    {
        LOG_ERROR("Sorting Failed on init order — make sure the connections are correct");
        THROW_MSG("Failed to Initialize dependency resolver");
        return false;
    }

    const std::size_t total = m_impl->m_initOrder.size();

    std::size_t i = 0;
    for (auto it = m_impl->m_initOrder.begin(); it != m_impl->m_initOrder.end(); ++it, ++i)
    {
        kfe::IManager* mgr = *it;
        if (!mgr->Initialize())
        {
            LOG_ERROR("Failed to initialize manager: {}", mgr->GetName());
            return false;
        }
        else
        {
            LOG_INFO("initializing: {}", mgr->GetName());
        }
    }
    return true;
}

_Use_decl_annotations_
bool kfe::DependencyResolver::UpdateLoopStart(float deltaTime) const
{
    static bool first_update = true;
    if (first_update) { LOG_INFO("Update Loop Start!"); }

    if (m_impl->m_initOrder.empty()) { return true; }

    for (auto it = m_impl->m_initOrder.begin(); it != m_impl->m_initOrder.end(); ++it)
    {
        kfe::IManager* mgr = *it;
        if (first_update)
        {
            LOG_INFO("Updating (start): {}", mgr->GetName());
        }
        mgr->OnFrameBegin(deltaTime);
    }

    first_update = false;
    return true;
}

bool kfe::DependencyResolver::UpdateLoopEnd() const
{
    static bool first_update = true;
    if (first_update) { LOG_INFO("Update Loop End!"); }

    if (m_impl->m_initOrder.empty()) { return true; }

    for (auto it = m_impl->m_initOrder.rbegin(); it != m_impl->m_initOrder.rend(); ++it)
    {
        kfe::IManager* mgr = *it;
        if (first_update)
        {
            LOG_INFO("Updating (end): {}", mgr->GetName());
        }
        mgr->OnFrameEnd();
    }

    first_update = false;
    return true;
}

bool kfe::DependencyResolver::Shutdown()
{
    bool flag = true;
    if (m_impl->m_initOrder.empty()) { return true; }

    for (auto it = m_impl->m_initOrder.rbegin(); it != m_impl->m_initOrder.rend(); ++it)
    {
        kfe::IManager* mgr = *it;
        if (!mgr) { continue; }

        const auto name = mgr->GetName();

        if (!mgr->Release())
        {
            flag = false;
            LOG_ERROR("Failed to properly destroy: {}", name);
        }
    }
    return flag;
}
