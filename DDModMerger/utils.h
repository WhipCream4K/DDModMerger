#pragma once

#include <filesystem>
#include <string>
#include <iostream>
#include "Types.h"

powe::details::DirectoryTree RecursiveFileSearch(
	std::string_view searchFolderPath,
	std::string_view interestedExtension);

std::string GetModName(std::string_view modsFodlerPath, std::string_view modPath);

template<typename T, typename U>
inline std::enable_if_t<std::is_convertible_v<T, std::string_view>&& std::is_convertible_v<U, std::string_view>, bool>
MakeBackup(T sourcePath, U targetPath)
{
	try
	{
		const std::filesystem::path pathToBackup = std::filesystem::path(targetPath) / std::filesystem::path(sourcePath).filename();
		std::filesystem::create_directories(pathToBackup.parent_path()); // Create outputFolder if it doesn't exist
		bool copyResult{ std::filesystem::copy_file(sourcePath, pathToBackup, std::filesystem::copy_options::overwrite_existing) };

#ifdef _DEBUG
		if (copyResult)
		{
			std::cerr << "Backup of " << targetPath << " is successful\n";
		}
		else
		{
			std::cerr << "Backup of " << targetPath << " is failed\n";
		}
#endif

		return copyResult;
	}
	catch (const std::filesystem::filesystem_error& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
	}

}