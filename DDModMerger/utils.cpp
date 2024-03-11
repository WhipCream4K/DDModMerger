#include "utils.h"

#include <future>
#include <execution>

#include "LFQueue.h"
#include "Types.h"
#include "ThreadPool.h"

namespace fs = std::filesystem;

struct FileSearchArgs
{
	FileSearchArgs(
		std::atomic_int32_t& activeTasks,
		std::condition_variable& waitCV)
		: activeTasks(activeTasks)
		, waitCV(waitCV)
	{}

	std::reference_wrapper<std::atomic_int32_t> activeTasks;
	std::reference_wrapper<std::condition_variable> waitCV;

	powe::LFQueue<powe::details::DirectoryTree> fileSearchResultQueue;
	powe::details::DirectoryTree fileSearchResult;
	std::mutex fileSearchResultMutex;
};

void RecursiveFileSearchAsync(const std::string& source, std::string_view extension, std::shared_ptr<FileSearchArgs> args)
{
	powe::details::DirectoryTree fileMap;

	for (const auto& entry : fs::directory_iterator(source, fs::directory_options::skip_permission_denied)) {
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			const std::string filename{ entry.path().stem().string() };
			const std::string filePath{ entry.path().string() };
			//std::replace( filePath.begin(), filePath.end(), '\\', '/');

			//{
			//	std::scoped_lock lock(args->fileSearchResultMutex);
			//	args->fileSearchResult[filename] = filePath;
			//}
			fileMap[filename] = filePath;
		}
		else if (entry.is_directory()) {

			// For directories, we'll initiate an async call
			args->activeTasks.get().fetch_add(1, std::memory_order_relaxed);

			ThreadPool::EnqueueDetach(
				RecursiveFileSearchAsync,
				entry.path().string(),
				extension,
				args);
		}
	}

	args->activeTasks.get().fetch_sub(1, std::memory_order_relaxed);
	args->fileSearchResultQueue.Push(std::move(fileMap));
	args->waitCV.get().notify_all();
}

powe::details::DirectoryTree RecursiveFileSearch(std::string_view searchFolderPath, std::string_view interestedExtension)
{
	std::atomic_int32_t activeTasks{};
	std::mutex activeTaskMutex;
	std::condition_variable waitCV{};

	std::shared_ptr<FileSearchArgs> fileSearchArgs{ std::make_shared<FileSearchArgs>(activeTasks,waitCV) };

	activeTasks.fetch_add(1, std::memory_order_relaxed);

	const std::string& sourceRef{ searchFolderPath.data() };

	//std::async(std::launch::async,RecursiveFileSearchAsync, sourceRef, interestedExtension, fileSearchArgs);
	std::jthread(RecursiveFileSearchAsync, sourceRef, interestedExtension, fileSearchArgs).detach();

	//ThreadPool::EnqueueDetach(
	//	RecursiveFileSearchAsync,
	//	sourceRef,
	//	interestedExtension,
	//	fileSearchArgs);

	// Wait for all of threads finish works
	std::unique_lock lock(activeTaskMutex);
	waitCV.wait(lock, [&activeTasks = std::as_const(activeTasks)]
		{
			return activeTasks.load(std::memory_order_relaxed) == 0;
		});

	powe::details::DirectoryTree outFileMap;


	while (!fileSearchArgs->fileSearchResultQueue.Empty())
	{
		auto fileMap = fileSearchArgs->fileSearchResultQueue.Front();
		outFileMap.insert(fileMap.begin(), fileMap.end());

		fileSearchArgs->fileSearchResultQueue.Pop();
	}

	//for (const auto& [fileName, path] : fileSearchArgs->fileSearchResult)
	//{
	//	//std::replace(std::execution::par_unseq, outFileMap[fileName].begin(), outFileMap[fileName].end(), '\\', '/');
	//	std::cerr << "File: " << fileName << " Path: " << path << '\n';
	//}

	return outFileMap;
}

std::string GetModName(std::string_view modsFodlerPath, std::string_view modPath)
{
	std::string modsName{ modPath };
	modsName = modsName.substr(modsFodlerPath.size() + 1); // + 1 for the '/'
	modsName = modsName.substr(0, modsName.find_first_of("/\\"));
	return modsName;
}
