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

	DirTreeCreator(const CVarReader& cVarReader, const std::shared_ptr<powe::ThreadPool>& threadPool);

	powe::details::DirectoryTree CreateDirTree(bool measureTime = true) const;
	std::future<powe::details::DirectoryTree> CreateDirTreeAsync(bool measureTime = true) const;

	static constexpr const char* DirTreeFolder = "/cache";
	static constexpr const char* DirTreeJSONFileName = "dirTree.json";

private:

	powe::details::DirectoryTree CreateDirTreeIntern() const;

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;
	
	std::string m_SearchFolderPath;
	std::string m_InterestedExtension;
	std::string m_OutputFilePath;
};

