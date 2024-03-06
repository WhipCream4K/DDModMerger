#pragma once

#include <filesystem>
#include <string>
#include "Types.h"

//struct FileSearchArgs;

//void RecursiveFileSearch(const std::string& source, std::string_view extension, std::shared_ptr<FileSearchArgs> args);

powe::details::DirectoryTree RecursiveFileSearch(
	std::string_view searchFolderPath, 
	std::string_view interestedExtension,
	std::shared_ptr<powe::ThreadPool> threadPool);

std::string GetModName(std::string_view modsParentPath,std::string_view modPath);

