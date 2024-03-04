#include "DirTreeCreator.h"

#include <iostream>

#include "Types.h"
#include "nlohmann/json.hpp"
#include "EnvironmentVariables.h"
#include "utils.h"
#include "thread_pool/thread_pool.h"

namespace fs = std::filesystem;


// Read file from json and return an unordered_map of file names and file paths
// could be empty if file is not found or empty
std::unordered_map<std::string, std::string> ReadJSONFile(const std::string& filePath)
{
	// read file from json
	std::ifstream fileStream(filePath);

	std::unordered_map<std::string, std::string> fileMap;

	// Check if the file is open
	if (!fileStream.is_open()) {
		std::cerr << "Error: Failed to open file: " << filePath << std::endl;
		return fileMap;
	};

	// Parse the file stream into a json object
	try
	{
		nlohmann::json json;

		fileStream >> json;

		// Iterate over the JSON object and populate the unordered_map

		for (const auto& [key, value] : json.items()) {
			fileMap[key] = value;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
	}


	return fileMap;
}


DirTreeCreator::DirTreeCreator(const CVarReader& cVarReader, const std::shared_ptr<powe::ThreadPool>& threadPool)
	: m_ThreadPool(threadPool)
{
	m_SearchFolderPath = cVarReader.ReadCVar("-path");
	m_InterestedExtension = cVarReader.ReadCVar("-ext");
	m_OutputFilePath = cVarReader.ReadCVar("-out");

	if (!m_ThreadPool)
	{
		throw std::runtime_error("Error: ThreadPool is not initialized");
	}

	const fs::path searchFolderPath{ m_SearchFolderPath };
	if (searchFolderPath.string().find(DEFAULT_DD_TOPLEVEL_FOLDER) == std::string::npos)
	{
		throw std::runtime_error("Error: -path should be a subfolder of " + std::string(DEFAULT_DD_TOPLEVEL_FOLDER));
	}

	// check all variables if they are empty then throw an exception
	if (m_SearchFolderPath.empty() || m_InterestedExtension.empty() || m_OutputFilePath.empty()) {
		throw std::runtime_error("Error: -path, -ext, -out are required arguments");
	}
}

powe::details::DirectoryTree DirTreeCreator::CreateDirTree(bool measureTime) const
{
	if (measureTime)
	{
		powe::details::DirectoryTree fileMap;
		// measure time
		auto start = std::chrono::high_resolution_clock::now();
		fileMap = CreateDirTreeIntern();
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		std::cout << "Create DirTree elapsed time: " << elapsed.count() << "s\n";
	}

	return CreateDirTreeIntern();
}

std::future<powe::details::DirectoryTree> DirTreeCreator::CreateDirTreeAsync(bool measureTime) const
{
	if (measureTime)
	{
		auto createDirTree = [this]() -> powe::details::DirectoryTree
			{
				powe::details::DirectoryTree fileMap;
				auto start = std::chrono::high_resolution_clock::now();
				fileMap = CreateDirTreeIntern();
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				std::cout << "Create DirTree elapsed time: " << elapsed.count() << "s\n";
				return fileMap;
			};

		return m_ThreadPool->enqueue(createDirTree);
	}

	auto createDirTree = [this]() -> powe::details::DirectoryTree
		{
			return CreateDirTreeIntern();
		};

	return m_ThreadPool->enqueue(createDirTree);

}

powe::details::DirectoryTree DirTreeCreator::CreateDirTreeIntern() const
{
	fs::path searchFolderPath{ m_SearchFolderPath };
	fs::path outputPath{ m_OutputFilePath + DirTreeFolder + "/" + DirTreeJSONFileName };

	// Check if the output file already exists
	if (fs::exists(outputPath)) {

		auto tempFileMap = ReadJSONFile(outputPath.string());

		if (tempFileMap.empty())
		{
			std::cerr << "Error: Failed to read file: " << outputPath << std::endl;
		}

		return tempFileMap;
	}

	auto outFileMap{ RecursiveFileSearch(m_SearchFolderPath, m_InterestedExtension, m_ThreadPool) };

	std::ofstream outputFile(outputPath);

	if (!outputFile.is_open())
	{
		fs::create_directories(outputPath.parent_path());
		if (!outputFile.is_open())
			throw std::runtime_error("Error: Failed to open file: " + outputPath.string());
	}

	try
	{
		nlohmann::json jsonWriter = outFileMap;
		outputFile << jsonWriter.dump(4);
	}
	catch (const std::exception&)
	{
		std::cerr << "Error: Failed to write to file: " << outputPath << std::endl;
		return outFileMap;
	}

	std::cout << "Search completed. Results are saved in: " << outputPath << '\n';

	return outFileMap;
}
