#pragma once

#include <filesystem>
#include <fstream>
#include <unordered_map>

#include "CVarReader.h"
#include "Types.h"

class DirTreeCreator
{
public:

	DirTreeCreator(const CVarReader& cVarReader,const std::shared_ptr<powe::ThreadPool>& threadPool);

	std::unordered_map<std::string, std::string> CreateDirTree(bool measureTime = true);
	const std::string& GetOutputFileName() const { return m_OutputFilePath; }
	
	static constexpr const char* DirTreeFolder = "/cache";
	static constexpr const char* DirTreeJSONFileName = "dirTree.json";

private:

	std::unordered_map<std::string, std::string> CreateDirTreeIntern();

	std::shared_ptr<powe::ThreadPool> m_ProgramThreadPool;
	std::string m_SearchFolderPath;
	std::string m_InterestedExtension;
	std::string m_OutputFilePath;
};

