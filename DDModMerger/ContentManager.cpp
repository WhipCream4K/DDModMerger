#include "ContentManager.h"

#include <iostream>
#include <filesystem>

#include "utils.h"
#include "ThreadPool.h"

namespace fs = std::filesystem;

ContentManager::ContentManager(const CVarReader& cVarReader)
	: m_ModsFilePath(cVarReader.ReadCVar("-mods"))
	, m_InterestedExtension(cVarReader.ReadCVar("-ext"))
{
	if (m_InterestedExtension.empty() || m_ModsFilePath.empty())
	{
		throw std::runtime_error("Error: -mods, -ext are required arguments");
	}
}


void ContentManager::LoadModsContentAsync()
{
	auto loadModsContent = [
		modsPath = std::string_view(m_ModsFilePath),
			extension = std::string_view(m_InterestedExtension)]() -> powe::details::ModsOverwriteOrder
		{
			powe::details::ModsOverwriteOrder modsOverwriteOrder;
			std::vector<std::future<powe::details::DirectoryTree>> searchFutures;

			for (const auto& entry : fs::directory_iterator(modsPath, fs::directory_options::skip_permission_denied))
			{
				// for every directory, we'll initiate an async call
				if (entry.is_directory())
				{
					searchFutures.emplace_back(
						ThreadPool::Enqueue(
						RecursiveFileSearch,
						entry.path().string(),extension));
				}
			}

			for (auto& future : searchFutures)
			{
				auto fileMap = future.get();
				for (const auto& [fileName,path] : fileMap)
				{
					modsOverwriteOrder[fileName].emplace_back(path);
				}
			}

			return modsOverwriteOrder;
		};

	m_LoadModsContentFuture = ThreadPool::Enqueue(loadModsContent);
}

const powe::details::ModsOverwriteOrder& ContentManager::GetAllModsOverwriteOrder()
{
	if (m_LoadModsContentFuture.valid())
	{
		m_ModsOverwriteOrder = m_LoadModsContentFuture.get();
	}

	return m_ModsOverwriteOrder;
}

