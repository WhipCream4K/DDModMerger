#pragma once

#include <string>

#include "Types.h"
#include "CVarReader.h"

class FileCloneUtility
{
public:

	FileCloneUtility(const CVarReader& cVarReader);

	void BackupMainFile(
		const powe::details::DirectoryTree& dirTree,
		const std::vector<std::string>& modsFiles);

private:

	std::string m_SearchFolderPath;
	std::string m_OutputFolder;
	
};

