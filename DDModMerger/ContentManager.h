#pragma once

#include <future>
#include "CVarReader.h"
#include "Types.h"

class ContentManager
{
public:

	ContentManager(
		const CVarReader& cVarReader);

	void LoadModsContentAsync();
	const powe::details::ModsOverwriteOrder& GetAllModsOverwriteOrder();

	bool IsFinished();
	std::string_view GetModsFilePath() const { return m_ModsFilePath; }

private:


	powe::details::ModsOverwriteOrder m_ModsOverwriteOrder;
	std::future<powe::details::ModsOverwriteOrder> m_LoadModsContentFuture;

	std::string m_ModsFilePath;
	std::string m_InterestedExtension;
};