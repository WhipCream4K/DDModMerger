#pragma once

#include <future>
#include "CVarReader.h"
#include "Types.h"

class ContentManager
{
public:

	ContentManager(
		const CVarReader& cVarReader,
		const std::shared_ptr<powe::ThreadPool>& threadPool);

	void LoadModsContentAsync();
	const powe::details::ModsOverwriteOrder& GetAllModsOverwriteOrder();

	std::string_view GetModsFilePath() const { return m_ModsFilePath; }

private:

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;

	powe::details::ModsOverwriteOrder m_ModsOverwriteOrder;
	std::future<powe::details::ModsOverwriteOrder> m_LoadModsContentFuture;

	std::string m_ModsFilePath;
	std::string m_InterestedExtension;
};