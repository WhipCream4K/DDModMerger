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

			const uint32_t threadCount{ ThreadPool::Size() };


			for (const auto& entry : fs::directory_iterator(modsPath, fs::directory_options::skip_permission_denied))
			{
				// for every directory, we'll initiate an async call
				if (entry.is_directory())
				{
					//searchFutures.emplace_back(std::async(RecursiveFileSearch, entry.path().string(), extension));

					// Pretty much we wait until some of the works gets finished so we don't occupy all the threads
					if (searchFutures.size() > threadCount / 2)
					{
						for (auto& contentFuture : searchFutures)
						{
							auto fileMap = contentFuture.get();
							for (const auto& [fileName, path] : fileMap)
							{
								modsOverwriteOrder[fileName].emplace_back(path);
							}
						}

						searchFutures.clear();
					}

					searchFutures.emplace_back(
						ThreadPool::Enqueue(
							RecursiveFileSearch,
							entry.path().string(), extension));
				}
			}

			for (auto& future : searchFutures)
			{
				auto fileMap = future.get();
				for (const auto& [fileName, path] : fileMap)
				{
					modsOverwriteOrder[fileName].emplace_back(path);
				}
			}



			return modsOverwriteOrder;
		};

		m_LoadModsContentFuture = std::async(std::launch::async, loadModsContent);
		//m_LoadModsContentFuture = ThreadPool::Enqueue(loadModsContent);
}

const powe::details::ModsOverwriteOrder& ContentManager::GetAllModsOverwriteOrder()
{
	return m_ModsOverwriteOrder;
}

bool ContentManager::IsFinished()
{
	if (m_LoadModsContentFuture.valid() && 
		m_LoadModsContentFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
	{
		m_ModsOverwriteOrder = m_LoadModsContentFuture.get();
		return true;
	}

	return !m_ModsOverwriteOrder.empty();
}

