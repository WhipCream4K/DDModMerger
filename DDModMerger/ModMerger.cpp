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
#include "utils.h"

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


void SetConsoleColor(uint16_t color) {
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
#endif
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

		// Assuming pi.hProcess is the handle to the external process
		DWORD timeout = (DWORD)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::minutes(3)).count(); // Timeout of 3 minutes
		DWORD result = WaitForSingleObject(pi.hProcess, timeout);

		if (result == WAIT_TIMEOUT) {
			DWORD exitCode;
			if (!GetExitCodeProcess(pi.hProcess, &exitCode)) {
				SetConsoleColor(FOREGROUND_RED); // Set text color to red
				std::cerr << "Failed to get exit code (" << GetLastError() << ").\n";
				SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Reset text color to default
			}
			else if (exitCode == STILL_ACTIVE) {
				// Process is still running, attempt to terminate
				if (!TerminateProcess(pi.hProcess, 0)) {
					SetConsoleColor(FOREGROUND_RED); // Set text color to red
					std::cerr << "Failed to terminate the process (" << GetLastError() << ").\n";
					SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Reset text color to default
				}
			}
		}

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
				std::cout << "Missing from main: " << comparisonPath << std::endl;
				continue;
			}


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
			ThreadPool::EnqueueDetach(RecursiveCompareDirAsync, baseSource, entry.path().string(), target, args);
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

	ThreadPool::EnqueueDetach(copyAndUnpack);

	return threadFuture;
}

std::future<void> ModMerger::MergeAsync(
	std::string_view mainFilePath,
	const std::vector<std::string>& pathToMods,
	const powe::details::DirectoryTree& dirTree)
{
	// The merge can only happen when the file compare is done by order
	// that means we have to wait for the first compare to finish then we can start the merge
	// and then checks again if the next compare is done and so on

	auto merge = [mainFilePath, &pathToMods, &dirTree, this]()
		{
			fs::path tempFolder{ "./mergeRoom" };

			const fs::path mainFileFS{ mainFilePath };
			const std::string mainFileName{ mainFileFS.stem().string() };

			tempFolder /= mainFileName;

			const fs::path mainUnpackFolder{ (tempFolder / mainFileName) };


			{
				// now we just go a list of files that we need to move to sourcePath
				auto cleanFilesToMove{ PrepareForMerge(mainFilePath, tempFolder.string(), pathToMods) };

				std::for_each(std::execution::par_unseq, cleanFilesToMove.begin(), cleanFilesToMove.end(),
					[&tempFolder, &dirTree, this](const std::string& file)
					{
						std::string modName{ file.substr(tempFolder.string().size() + 1) }; // get rid of temp folder's name
						modName = modName.substr(modName.find_first_of("/\\") + 1); // get rid of mod's name
						fs::rename(file, (tempFolder / modName));
					});
			}

			// Repack
			CallARCTool(mainUnpackFolder, m_ARCToolScriptPath);

			// move the main file to respective folder
			// should be out/nativePC/rom
			{
				fs::path outFilePath{ m_OutputFolderPath };
				outFilePath /= mainFilePath.substr(m_SearchFolderPath.size() + 1); // get rid of top level folder
				fs::create_directories(outFilePath.parent_path());
				fs::rename((tempFolder / mainFileFS.filename()), outFilePath);
			}

			fs::remove_all(tempFolder);
		};


	return std::async(std::launch::async, merge);
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

	std::vector<std::string> modsNames{};
	modsNames.reserve(modsPath.size());


	// Unpack files
	{
		//std::vector<std::future<void>> unpackFutures{};

		// we expect all mods file and one main file to unpack and main thread
		std::barrier barrier{ uint32_t(modsPath.size() + 2) };

		UnpackBarrier(mainFilePath, unpackPath, barrier);
		//unpackFutures.emplace_back(UnpackAsync(mainFilePath, unpackPath));

		for (size_t i = 0; i < modsPath.size(); i++)
		{
			// TODO: ARC Tool is inconsistent with the output folder name so mod folder will be name after index
			std::string modName{ modsPath[i] };
			modName = modName.substr(m_ModFolderPath.size() + 1, 10); // + 1 for the '/'
			modName.erase(std::remove_if(std::execution::par_unseq,
				modName.begin(), modName.end(), [](char c) { return std::isspace(c) || c == '.'; }), modName.end());

			modsNames.emplace_back(modName);
			//unpackFutures.emplace_back(UnpackAsync(modsPath[i], (unpackFS / modName).string()));
			UnpackBarrier(modsPath[i], (unpackFS / modName).string(), barrier);
		}

		barrier.arrive_and_wait();

		//for (auto& future : unpackFutures)
		//{
		//	future.get();
		//}
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
				(unpackFS / modsNames[i] / mainFileFS.stem()).string()));
		}

		for (auto& future : compareFutures)
		{
			auto filesToMove{ future.get() };
			modsLooseFiles.emplace_back(filesToMove);
		}
	}


	std::unordered_map<std::string, std::string>  uniqueFiles{};

	for (const auto& modFiles : modsLooseFiles) {
		for (const auto& file : modFiles) {
			std::string filename = fs::path(file).filename().string();
			// Insert file into the map if not already present; this respects priority due to the loop order
			//uniqueFiles.insert(file);
			uniqueFiles[filename] = file;
		}
	}

	std::vector<std::string> outMoveFiles{};
	for (const auto& [fileName, path] : uniqueFiles)
	{
		outMoveFiles.emplace_back(path);
	}

	return outMoveFiles;

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
				mergeFutures.emplace_back(MergeAsync(findItr->second, pathToMods, dirTree));
			}
		}
	}

	for (auto& future : mergeFutures)
	{
		future.get();
	}

	m_MergeTask = std::future<void>{};
}

ModMerger::ModMerger(
	const CVarReader& cVarReader)
{
	m_ModFolderPath = cVarReader.ReadCVar("-mods");
	m_OutputFolderPath = cVarReader.ReadCVar("-out");
	m_ARCToolScriptPath = cVarReader.ReadCVar("-arctool");
	m_SearchFolderPath = cVarReader.ReadCVar("-path");

	// check all variables if they are empty then throw an exception
	if (m_ModFolderPath.empty() || m_OutputFolderPath.empty() || m_ARCToolScriptPath.empty())
	{
		throw std::runtime_error("Error: One or more of the required arguments are missing");
	}
}

void ModMerger::MergeContent(const powe::details::DirectoryTree& dirTree, const powe::details::ModsOverwriteOrder& overwriteOrder, bool measureTime)
{
	if (overwriteOrder.empty())
	{
		std::cerr << "No mods to merge\n";
		return;
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

void ModMerger::MergeContentAsync(const powe::details::DirectoryTree& dirTree, const powe::details::ModsOverwriteOrder& overwriteOrder, bool measureTime)
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

		m_MergeTask = std::async(std::launch::async, merge);
	}
	else
	{
		auto merge = [this, &dirTree, &overwriteOrder]()
			{
				MergeContentIntern(dirTree, overwriteOrder);
			};

		m_MergeTask = std::async(std::launch::async, merge);
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

