#include "ContentManager.h"
#include "thread_pool/thread_pool.h"

ContentManager::ContentManager(const CVarReader& cVarReader, const std::shared_ptr<powe::ThreadPool>& threadPool)
	: m_ThreadPool(threadPool)
	, m_ModsFilePath(cVarReader.ReadCVar("-mods"))
{
}



std::future<void> ContentManager::LoadModsContentAsync()
{
	auto loadModsContent = [modsPath = std::string_view(m_ModsFilePath)]()
	{
		 
	};

	return m_ThreadPool->enqueue(loadModsContent);
}

