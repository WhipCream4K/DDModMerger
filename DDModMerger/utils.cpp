#include "utils.h"

#include <future>

#include "LFQueue.h"
#include "Types.h"
#include "thread_pool/thread_pool.h"

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
	std::shared_ptr<powe::ThreadPool> threadPool;
};

void RecursiveFileSearchAsync(const std::string& source, std::string_view extension, std::shared_ptr<FileSearchArgs> args)
{
	powe::details::DirectoryTree fileMap;

	for (const auto& entry : fs::directory_iterator(source, fs::directory_options::skip_permission_denied)) {
		if (entry.is_regular_file() && entry.path().extension() == extension)
		{
			std::string filename = entry.path().stem().string();
			std::string filePath = entry.path().string();
			std::replace(filePath.begin(), filePath.end(), '\\', '/');

			fileMap[filename] = filePath;
		}
		else if (entry.is_directory()) {

			// For directories, we'll initiate an async call
			args->activeTasks.get().fetch_add(1, std::memory_order_relaxed);

			args->threadPool->enqueue_detach(
				RecursiveFileSearchAsync,
				entry.path().string(),
				extension,
				args);
		}
	}

	args->activeTasks.get().fetch_sub(1, std::memory_order_relaxed);
	args->fileSearchResultQueue.Push(std::move(fileMap));
	args->waitCV.get().notify_one();
}

powe::details::DirectoryTree RecursiveFileSearch(std::string_view searchFolderPath, std::string_view interestedExtension, std::shared_ptr<powe::ThreadPool> threadPool)
{
	std::atomic_int32_t activeTasks{};
	std::mutex activeTaskMutex;
	std::condition_variable waitCV{};

	std::shared_ptr<FileSearchArgs> fileSearchArgs{ std::make_shared<FileSearchArgs>(activeTasks,waitCV) };
	fileSearchArgs->threadPool = threadPool;

	activeTasks.fetch_add(1, std::memory_order_relaxed);

	const std::string& sourceRef{ searchFolderPath.data() };

	threadPool->enqueue_detach(
		RecursiveFileSearchAsync,
		sourceRef,
		interestedExtension,
		fileSearchArgs);

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

	return outFileMap;
}
