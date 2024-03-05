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
	void CreateDirTreeAsync(bool measureTime = true);

	const powe::details::DirectoryTree& GetDirTree();

	static constexpr const char* DirTreeFolder = "/cache";
	static constexpr const char* DirTreeJSONFileName = "dirTree.json";

private:

	powe::details::DirectoryTree CreateDirTreeIntern() const;

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;
	
	powe::details::DirectoryTree m_DirTree;
	std::future<powe::details::DirectoryTree> m_CreateDirTreeFuture;
	
	std::string m_SearchFolderPath;
	std::string m_InterestedExtension;
	std::string m_OutputFilePath;
};

