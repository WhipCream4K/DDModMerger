#include "ContentManager.h"
#include "thread_pool/thread_pool.h"

ContentManager::ContentManager(const CVarReader& cVarReader, const std::shared_ptr<powe::ThreadPool>& threadPool)
	: m_ThreadPool(threadPool)
{
}


void ContentManager::LoadModsOverwriteOrder()
{

}
