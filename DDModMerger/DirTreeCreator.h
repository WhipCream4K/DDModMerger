#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <future>

#include "CVarReader.h"
#include "Types.h"

class DirTreeCreator
{
public:

	DirTreeCreator(const CVarReader& cVarReader);

	powe::details::DirectoryTree CreateDirTree(bool measureTime = true) const;
	void CreateDirTreeAsync(bool measureTime = true);
	const powe::details::DirectoryTree& GetDirTree();
	bool IsFinished();


private:

	powe::details::DirectoryTree CreateDirTreeIntern() const;
	
	powe::details::DirectoryTree m_DirTree;
	std::future<powe::details::DirectoryTree> m_CreateDirTreeFuture;
	
	std::string m_SearchFolderPath;
	std::string m_InterestedExtension;
	std::string m_OutputFilePath;
};

