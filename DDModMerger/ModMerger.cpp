#include "ModMerger.h"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <execution>
#include <regex>

#include "nlohmann/json.hpp"
#include "EnvironmentVariables.h"
#include "openssl/sha.h"
#include "thread_pool/thread_pool.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace fs = std::filesystem;

void RenameFileToFolder(std::string_view sourceFilePath, std::string_view destinationFolder)
{
	const fs::path sourcePath{ sourceFilePath };
	const fs::path dest{ destinationFolder / sourcePath.filename() };

	fs::rename(sourceFilePath, dest);
}

void RenameFileToFolder(const fs::path& sourceFilePath, const fs::path& destinationFolder)
{
	const fs::path dest{ destinationFolder / sourceFilePath.filename() };
	fs::rename(sourceFilePath, dest);
}

/// <summary>
/// This function will call ARCTool from FluffyQuack to unpack our arc files
/// </summary>
void CallARCTool(const fs::path& itemPath, const fs::path& arctoolPath)
{
#ifdef _WIN32

	STARTUPINFO si{ sizeof(si) };
	PROCESS_INFORMATION pi{};

	const std::string cmdLine = fs::absolute(arctoolPath).string() + " " + fs::absolute(itemPath).string();

	// Convert the command line string to wide character string
	int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, NULL, 0);
	std::vector<wchar_t> wideCmdLine(wideCharLength);
	MultiByteToWideChar(CP_UTF8, 0, cmdLine.c_str(), -1, wideCmdLine.data(), wideCharLength);

	// CreateProcess requires a non-const wchar_t* as the command line argument, hence the copy
	std::vector<wchar_t> cmdLineCStr(wideCmdLine.begin(), wideCmdLine.end());
	cmdLineCStr.push_back('\0'); // Null-terminate the string

	// Start the child process
	if (!CreateProcess(
		NULL,           // No module name (use command line)
		cmdLineCStr.data(), // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory
		&si,            // Pointer to STARTUPINFO structure
		&pi))            // Pointer to PROCESS_INFORMATION structure)
	{
		std::cerr << "CreateProcess failed (" << GetLastError() << ").\n";
	}
	else {
		// Wait until child process exits
		WaitForSingleObject(pi.hProcess, INFINITE);

		// Close process and thread handles
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

#endif
}

void CallARCTool(std::string_view pathToItem, std::string_view pathToARCTool)
{
	CallARCTool(fs::path(pathToItem), fs::path(pathToARCTool));
}

// Function to calculate SHA-256 hash of a file
std::vector<unsigned char> CalculateSHA256(const fs::path& filePath) {
	std::ifstream fileStream(filePath, std::ios::binary);
	std::vector<unsigned char> fileBuffer(std::istreambuf_iterator<char>(fileStream), {});

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256(fileBuffer.data(), fileBuffer.size(), hash);

	return std::vector<unsigned char>(hash, hash + SHA256_DIGEST_LENGTH);
}


void RecursiveCompareDirAsync(std::string_view baseSource, const std::string& source, std::string_view target, std::shared_ptr<CompareDirectoriesArgs> args)
{
	for (const auto& entry : fs::directory_iterator(source)) {

		if (entry.is_regular_file()) {

			fs::path relativePath = fs::relative(entry.path(), baseSource);
			fs::path comparisonPath = target / relativePath;

			// Check for existence
			if (!fs::exists(comparisonPath)) {
				std::cout << "Missing: " << comparisonPath << std::endl;
				continue;
			}

			// Quick check: Compare file sizes
			//if (fs::file_size(entry.path()) != fs::file_size(comparisonPath)) {
			//	std::cout << "Size differs: " << comparisonPath << std::endl;
			//}

			auto hash1 = CalculateSHA256(entry.path());
			auto hash2 = CalculateSHA256(comparisonPath);
			if (hash1 != hash2)
			{
				std::cout << "Content differs: " << comparisonPath << std::endl;

				{
					std::scoped_lock lock(args->filesToMoveMutex);
					args->filesToMove.emplace_back(comparisonPath.string());
				}
			}

		}
		else if (entry.is_directory())
		{
			args->activeTasks.get().fetch_add(1, std::memory_order_relaxed);
			args->threadPool->enqueue_detach(RecursiveCompareDirAsync, baseSource, entry.path().string(), target, args);
		}
	}

	args->activeTasks.get().fetch_sub(1, std::memory_order_relaxed);
	args->waitCV.get().notify_all();
}

std::future<void> ModMerger::UnpackAsync(std::string_view sourcePath, std::string_view targetPath)
{
	std::shared_ptr<std::promise<void>> threadPromise{ std::make_shared<std::promise<void>>() };
	std::future<void> threadFuture{ threadPromise->get_future() };

	auto copyAndUnpack = [sourcePath, targetPath, threadPromise,
		arctool = std::string_view(m_ARCToolScriptPath)]()
		{
			MakeBackup(sourcePath, targetPath);
			const fs::path newBackupFilePath{ fs::path(targetPath) / fs::path(sourcePath).filename() };
			CallARCTool(newBackupFilePath, arctool);
			threadPromise->set_value();
		};

	m_ThreadPool->enqueue_detach(copyAndUnpack);

	return threadFuture;
}

std::future<void> ModMerger::MergeAsync(std::string_view mainFilePath, const std::vector<std::string>& pathToMods)
{
	//auto threadPromise{ std::make_shared<std::promise<void>>() };
	//std::future<void> threadFuture{ threadPromise->get_future() };

	// The merge can only happen when the file compare is done by order
	// that means we have to wait for the first compare to finish then we can start the merge
	// and then checks again if the next compare is done and so on

	auto merge = [mainFilePath, &pathToMods, this]()
		{
			fs::path tempFolder{ m_OutputFolderPath };

			const fs::path mainFileFS{ mainFilePath };
			const std::string mainFileName{ mainFileFS.stem().string() };

			tempFolder = tempFolder / "temp" / mainFileName;

			const fs::path mainUnpackFolder{ (tempFolder / mainFileName) };


			{

				// now we just go a list of files that we need to move to sourcePath
				auto cleanFilesToMove{ PrepareForMerge(mainFilePath, tempFolder.string(), pathToMods) };

				std::for_each(std::execution::par_unseq, cleanFilesToMove.begin(), cleanFilesToMove.end(),
					[&tempFolder, this](const std::string& file)
					{
						// fast find substring of the modsID
						std::regex modsIDPattern{ R"([\\/](\d+)[\\/])" };
						std::smatch match;

						if (std::regex_search(file, match, modsIDPattern))
						{
							const std::string extractedString{ match.suffix().str() };
							fs::rename(file, (tempFolder / extractedString));
						}
					});
			}

			// Repack
			CallARCTool(mainUnpackFolder, m_ARCToolScriptPath);

			// move the main file to respective folder
			// should be out/nativePC/rom
			{
				fs::path outFilePath{ m_OutputFolderPath };
				outFilePath /= mainFilePath.substr(mainFilePath.find(DEFAULT_DD_TOPLEVEL_FOLDER));
				fs::create_directories(outFilePath.parent_path());
				fs::rename((tempFolder / mainFileFS.filename()), outFilePath);
			}

			fs::remove_all(tempFolder);
		};


	return m_ThreadPool->enqueue(merge);
}

std::vector<std::string> ModMerger::PrepareForMerge(std::string_view mainFilePath, std::string_view unpackPath, const std::vector<std::string>& modsPath)
{
	// compare mod files to the main file and keep the result then
	// check the result with another mod files if it has any matches

	std::vector<std::vector<std::string>> modsLooseFiles{};
	modsLooseFiles.reserve(modsPath.size());


	// move file to operation folder
	const fs::path unpackFS{ unpackPath };
	const fs::path mainFileFS{ fs::path(mainFilePath).filename() };

	// Unpack files
	{
		// we expect all mods file and one main file to unpack and main thread
		std::barrier barrier{ uint32_t(modsPath.size() + 2) };

		UnpackBarrier(mainFilePath, unpackPath, barrier);

		for (size_t i = 0; i < modsPath.size(); i++)
		{
			UnpackBarrier(modsPath[i], (unpackFS / std::to_string(i)).string(), barrier);
		}

		barrier.arrive_and_wait();
	}


	// Compare files
	{
		std::vector<std::future<std::vector<std::string>>> compareFutures{};
		compareFutures.reserve(modsPath.size());

		const fs::path unpackBaseSource{ unpackPath / mainFileFS.stem() };

		for (size_t i = 0; i < modsPath.size(); i++)
		{
			compareFutures.emplace_back(CompareDirectoriesAsync(
				unpackBaseSource.string(),
				(unpackFS / std::to_string(i) / mainFileFS.stem()).string()));
		}

		for (auto& future : compareFutures)
		{
			auto filesToMove{ future.get() };
			modsLooseFiles.emplace_back(filesToMove);
		}
	}


	std::unordered_set<std::string>  uniqueFiles{};

	for (const auto& modFiles : modsLooseFiles) {
		for (const auto& file : modFiles) {
			std::string filename = fs::path(file).filename().string();
			// Insert file into the map if not already present; this respects priority due to the loop order
			uniqueFiles.insert(file);
		}
	}

	return std::vector<std::string>(uniqueFiles.begin(), uniqueFiles.end());

}


void ModMerger::MergeContentIntern(const powe::details::DirectoryTree& dirTree, const powe::details::ModsOverwriteOrder& overwriteOrder)
{
	fs::path outputFolder{ m_OutputFolderPath };
	outputFolder /= DEFAULT_DD_TOPLEVEL_FOLDER;

	std::vector<std::future<void>> mergeFutures{};

	for (const auto& [fileName, pathToMods] : overwriteOrder)
	{
		if (pathToMods.size() > 1)
		{
			// copy the main file to the temp folder
			if (const auto findItr = dirTree.find(fileName); findItr != dirTree.end())
			{
				// for example the pathToMergeFolder will look like out/nativePC/rom
				std::string_view mainFilePath{ findItr->second };
				const std::string fileAfterTopLevelFolder{ mainFilePath.substr(mainFilePath.find(DEFAULT_DD_TOPLEVEL_FOLDER) + sizeof(DEFAULT_DD_TOPLEVEL_FOLDER) + 1) };
				const fs::path pathToMergeFolder{ outputFolder / fileAfterTopLevelFolder };

				mergeFutures.emplace_back(MergeAsync(findItr->second, pathToMods));
			}
		}
	}

	for (auto& future : mergeFutures)
	{
		future.get();
	}
}

ModMerger::ModMerger(
	const CVarReader& cVarReader,
	const std::shared_ptr<powe::ThreadPool>& threadPool)
	: m_ThreadPool(threadPool)
{
	m_ModFolderPath = cVarReader.ReadCVar("-mods");
	m_OutputFolderPath = cVarReader.ReadCVar("-out");
	m_ARCToolScriptPath = cVarReader.ReadCVar("-arctool");


	if (!threadPool)
	{
		throw std::runtime_error("Error: Thread pool is not initialized");
	}

	// check all variables if they are empty then throw an exception
	if (m_ModFolderPath.empty() || m_OutputFolderPath.empty() || m_ARCToolScriptPath.empty())
	{
		throw std::runtime_error("Error: One or more of the required arguments are missing");
	}
}

void ModMerger::MergeContent(const powe::details::DirectoryTree& dirTree, const powe::details::ModsOverwriteOrder& overwriteOrder, bool backup, bool measureTime)
{
	if (overwriteOrder.empty())
	{
		std::cerr << "No mods to merge\n";
		return;
	}

	if (backup)
	{
		const std::string backupFolder{ m_OutputFolderPath + "/backup" };

		std::for_each(std::execution::par_unseq, overwriteOrder.begin(), overwriteOrder.end(), [&dirTree, back = std::string_view(backupFolder)](const auto& value)
			{
				if (auto pathToFile = dirTree.find(value.first); pathToFile != dirTree.end())
				{
					const auto& path{ pathToFile->second };

					const fs::path fileAfterTopLevelFolder{ path.substr(path.find(DEFAULT_DD_TOPLEVEL_FOLDER)) };
					const std::string pathToBackupFolder{ fs::path(back / fileAfterTopLevelFolder.parent_path()).string() };
					MakeBackup(path, pathToBackupFolder);
				}
			});
	}

	if (measureTime)
	{
		// measure time
		auto start = std::chrono::high_resolution_clock::now();
		MergeContentIntern(dirTree, overwriteOrder);
		auto end = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = end - start;
		std::cout << "Merge Elapsed time: " << elapsed.count() << "s\n";
	}
	else
	{
		MergeContentIntern(dirTree, overwriteOrder);
	}
}

void ModMerger::MergeContentAsync(const powe::details::DirectoryTree& dirTree, const powe::details::ModsOverwriteOrder& overwriteOrder, bool backup, bool measureTime)
{
	if (overwriteOrder.empty())
	{
		std::cerr << "No mods to merge\n";
		return;
	}

	if (m_MergeTask.valid())
	{
		std::cerr << "Merge task is already running\n";
		return;
	}

	if (backup)
	{
		const std::string backupFolder{ m_OutputFolderPath + "/backup" };

		std::for_each(std::execution::par_unseq, overwriteOrder.begin(), overwriteOrder.end(), [&dirTree, back = std::string_view(backupFolder)](const auto& value)
			{
				if (auto pathToFile = dirTree.find(value.first); pathToFile != dirTree.end())
				{
					const auto& path{ pathToFile->second };

					const fs::path fileAfterTopLevelFolder{ path.substr(path.find(DEFAULT_DD_TOPLEVEL_FOLDER)) };
					const std::string pathToBackupFolder{ fs::path(back / fileAfterTopLevelFolder.parent_path()).string() };
					MakeBackup(path, pathToBackupFolder);
				}
			});
	}

	if (measureTime)
	{
		auto merge = [this, &dirTree, &overwriteOrder]()
			{
				// measure time
				auto start = std::chrono::high_resolution_clock::now();
				MergeContentIntern(dirTree, overwriteOrder);
				auto end = std::chrono::high_resolution_clock::now();
				std::chrono::duration<double> elapsed = end - start;
				std::cout << "Merge Elapsed time: " << elapsed.count() << "s\n";
			};

		m_MergeTask = m_ThreadPool->enqueue(merge);
	}
	else
	{
		auto merge = [this, &dirTree, &overwriteOrder]()
			{
				MergeContentIntern(dirTree, overwriteOrder);
			};

		m_MergeTask = m_ThreadPool->enqueue(merge);
	}

}

bool ModMerger::IsARCToolExist() const
{
	const fs::path arctool{ m_ARCToolScriptPath };
	return fs::exists(arctool);
}

bool ModMerger::IsReadyToMerge() const
{
	return !m_MergeTask.valid();
}

