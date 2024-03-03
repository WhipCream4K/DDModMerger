#include "DirTreeCreator.h"

#include <iostream>

#include "Types.h"
#include "nlohmann/json.hpp"
#include "EnvironmentVariables.h"
#include "thread_pool/thread_pool.h"

using OutputFileType = std::unordered_map<std::string, std::string>;

namespace fs = std::filesystem;

OutputFileType ProcessDir(
	const fs::path& path,
	const std::string& extension,
	dp::thread_pool<>& pool,
	std::vector<std::future<OutputFileType>>& futures,
	std::mutex& futuresMutex)
{
	std::unordered_map<std::string, std::string> localFileMap;

	for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied)) {
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			std::string filename = entry.path().stem().string();
			std::string filePath = entry.path().string();
			std::replace(filePath.begin(), filePath.end(), '\\', '/');

			localFileMap[filename] = filePath;
		}
		else if (entry.is_directory()) {

			// For directories, we'll initiate an async call

			{
				std::scoped_lock lock(futuresMutex);

				futures.push_back(pool.enqueue(
					ProcessDir,
					entry.path(),
					extension,
					std::ref(pool),
					std::ref(futures),
					std::ref(futuresMutex)));
			}

		}
	}

	return localFileMap;
}

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

		for (const auto& [key,value] : json.items()) {
			fileMap[key] = value;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
	}


	return fileMap;
}


DirTreeCreator::DirTreeCreator(const CVarReader& cVarReader,const std::shared_ptr<powe::ThreadPool>& threadPool)
	: m_ProgramThreadPool(threadPool)
{
	m_SearchFolderPath = cVarReader.ReadCVar("-path");
	m_InterestedExtension = cVarReader.ReadCVar("-ext");
	m_OutputFilePath = cVarReader.ReadCVar("-out");

	if(!m_ProgramThreadPool)
	{
		throw std::runtime_error("Error: ThreadPool is not initialized");
	}

	const fs::path searchFolderPath{m_SearchFolderPath};
	if(searchFolderPath.string().find(DEFAULT_DD_TOPLEVEL_FOLDER) == std::string::npos)
	{
		throw std::runtime_error("Error: -path should be a subfolder of " + std::string(DEFAULT_DD_TOPLEVEL_FOLDER));
	}

	// check all variables if they are empty then throw an exception
	if (m_SearchFolderPath.empty() || m_InterestedExtension.empty() || m_OutputFilePath.empty()) {
		throw std::runtime_error("Error: -path, -ext, -out are required arguments");
	}
}

std::unordered_map<std::string, std::string> DirTreeCreator::CreateDirTree(bool measureTime)
{
	OutputFileType fileMap;

	if (measureTime)
	{
		// measure time
		auto start = std::chrono::high_resolution_clock::now();
		fileMap = CreateDirTreeIntern();
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		std::cout << "Create DirTree elapsed time: " << elapsed.count() << "s\n";
	}
	else
	{
		fileMap = CreateDirTreeIntern();
	}


	return fileMap;
}

std::unordered_map<std::string, std::string> DirTreeCreator::CreateDirTreeIntern()
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

	std::unordered_map<std::string, std::string> fileMap;
	dp::thread_pool pool{};
	std::vector<std::future<OutputFileType>> futures;
	std::mutex futuresMutex;

	futures.emplace_back(pool.enqueue(
		ProcessDir,
		m_SearchFolderPath,
		m_InterestedExtension,
		std::ref(pool),
		std::ref(futures),
		std::ref(futuresMutex)));

	// Since ProcessDir might enqueue more tasks and add futures, you need to check and wait again.
	// This can be done by repeatedly processing the futures vector until no new futures are added.

	bool allFuturesProcessed = false;
	while (!allFuturesProcessed) {

		// std::this_thread::sleep_for(std::chrono::microseconds(10)); // Sleep for a bit (or use a condition variable to wait for new futures to be added

		std::vector<std::future<OutputFileType>> pendingFutures;

		{
			std::scoped_lock lock(futuresMutex);
			std::swap(pendingFutures, futures); // Move pending futures for processing
		}

		if (pendingFutures.empty()) {
			allFuturesProcessed = true; // No more futures to process
		}
		else {
			for (auto& future : pendingFutures) {

				//future.wait(); // Ensure the task is complete
				auto localFileMap = future.get(); // Retrieve the result

				// Merge the result into the main fileMap
				fileMap.insert(localFileMap.begin(), localFileMap.end());
			}
		}
	}

	std::ofstream outputFile(outputPath);

	if (!outputFile.is_open())
	{
		fs::create_directories(outputPath.parent_path());
		if(!outputFile.is_open())
			throw std::runtime_error("Error: Failed to open file: " + outputPath.string());
	}

	try
	{
		nlohmann::json jsonWriter = fileMap;
		outputFile << jsonWriter.dump(4);
	}
	catch (const std::exception&)
	{
		std::cerr << "Error: Failed to write to file: " << outputPath << std::endl;
		return fileMap;
	}

	std::cout << "Search completed. Results are saved in: " << outputPath << '\n';

	return fileMap;

}
