#include "FileCloneUtility.h"

#include <algorithm>
#include <execution>
#include <filesystem>

#include "utils.h"

namespace fs = std::filesystem;

constexpr char BackupFolder[] = "./backup";

void FileCloneUtility::BackupMainFile(const powe::details::DirectoryTree& dirTree, const std::vector<std::string>& modsFiles)
{
	std::for_each(modsFiles.begin(), modsFiles.end(), [&dirTree,
		searchFolder = std::string_view(m_SearchFolderPath)](const std::string& value)
		{
			const fs::path fileName{value};
			if (auto pathToFile = dirTree.find(fileName.stem().string()); pathToFile != dirTree.end())
			{
				const auto& path{ pathToFile->second };

				const fs::path fileAfterTopLevelFolder{ path.substr(searchFolder.size() + 1) }; // get rid of parent folder
				const std::string pathToBackupFolder{ fs::path(BackupFolder / fileAfterTopLevelFolder.parent_path()).string() };
				MakeBackup(path, pathToBackupFolder);
			}
		});
}

FileCloneUtility::FileCloneUtility(const CVarReader& cVarReader)
	: m_OutputFolder(cVarReader.ReadCVar("-out"))
	, m_SearchFolderPath(cVarReader.ReadCVar("-path"))
{
	if (m_OutputFolder.empty() || m_SearchFolderPath.empty())
	{
		throw std::runtime_error("Error: -out, -path are required arguments");
	}
}
