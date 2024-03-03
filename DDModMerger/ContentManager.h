#pragma once

#include "CVarReader.h"
#include "Types.h"

class ContentManager
{
public:
	
	ContentManager(
			const CVarReader& cVarReader,
				const std::shared_ptr<powe::ThreadPool>& threadPool);

	void LoadModsOverwriteOrder();

	const powe::details::ModsOverwriteOrder& GetAllModsOverwriteOrder() const { return m_ModsOverwriteOrder; }

private:

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;

	std::string m_OverwriteOrderFilePath;
	powe::details::ModsOverwriteOrder m_ModsOverwriteOrder;
};