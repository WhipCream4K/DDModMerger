#pragma once

#include <future>
#include <memory>
#include <filesystem>
#include <iostream>
#include <barrier>

#include "CVarReader.h"
#include "Types.h"
#include "thread_pool/thread_pool.h"

struct CompareDirectoriesArgs
{
	CompareDirectoriesArgs(
		std::atomic<int>& activeTasks,
		std::condition_variable& waitCV)
		: activeTasks(activeTasks)
		, waitCV(waitCV)
	{
	}

	std::reference_wrapper<std::atomic<int>> activeTasks;
	std::reference_wrapper<std::condition_variable> waitCV;

	std::vector<std::string> filesToMove;
	std::mutex filesToMoveMutex;

	std::shared_ptr<powe::ThreadPool> threadPool;
};

extern void CallARCTool(std::string_view pathToItem, std::string_view pathToARCTool);
extern void RecursiveCompareDirAsync(std::string_view baseSource, const std::string& source, std::string_view target, std::shared_ptr<CompareDirectoriesArgs> args);

class ModMerger
{
public:

	ModMerger(
		const CVarReader& cVarReader,
		std::shared_ptr<powe::ThreadPool>& threadPool);

	void MergeContent(
		const powe::details::DirectoryTree& dirTree,
		const powe::details::ModsOverwriteOrder& overwriteOrder,
		bool backup = true,
		bool measureTime = true);

	void MergeContentAsync(
		const powe::details::DirectoryTree& dirTree,
		const powe::details::ModsOverwriteOrder& overwriteOrder,
		bool backup = true,
		bool measureTime = true);

	void SetThreadPool(const std::shared_ptr<powe::ThreadPool>& threadPool) { m_ThreadPool = threadPool; }

	bool IsARCToolExist() const;
	bool IsReadyToMerge() const;

private:

	template<typename T, typename U>
	std::future<void> UnpackAsync(T&& sourcePath, U&& targetPath);
	std::future<void> UnpackAsync(std::string_view sourcePath, std::string_view targetPath);

	template<typename T, typename U>
	void UnpackBarrier(
		T&& sourcePath,
		U&& targetPath,
		std::barrier<>& barrier);

	template<typename T, typename U>
	std::future<std::vector<std::string>> CompareDirectoriesAsync(
		T&& sourcePath,
		U&& targetPath);

	std::future<void> MergeAsync(std::string_view mainFilePath, 
		const std::vector<std::string>& pathToMods,
		const powe::details::DirectoryTree& dirTree);

	std::vector<std::string> PrepareForMerge(
		std::string_view mainFilePath,
		std::string_view unpackPath,
		const std::vector<std::string>& modsPath);

	void MergeContentIntern(
		const powe::details::DirectoryTree& dirTree,
		const powe::details::ModsOverwriteOrder& overwriteOrder);

	std::shared_ptr<powe::ThreadPool> m_ThreadPool;

	std::future<void> m_MergeTask;

	std::string m_ModFolderPath;
	std::string m_ARCToolScriptPath;
	std::string m_OutputFolderPath;
	std::string m_SearchFolderPath;
};

template<typename T, typename U>
inline std::enable_if_t<std::is_convertible_v<T, std::string_view>&& std::is_convertible_v<U, std::string_view>, bool>
MakeBackup(T sourcePath, U targetPath)
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

template<typename T, typename U>
inline std::future<void> ModMerger::UnpackAsync(T&& sourcePath, U&& targetPath)
{
	auto copyAndUnpack = [lsourcePath = std::forward<T>(sourcePath), ltargetPath = std::forward<U>(targetPath),
		arctool = std::string_view(m_ARCToolScriptPath)]() -> void
		{
			MakeBackup(lsourcePath, ltargetPath);
			const std::filesystem::path newBackupFilePath{ std::filesystem::path(ltargetPath) / std::filesystem::path(lsourcePath).filename() };
			CallARCTool(newBackupFilePath.string(), arctool);
		};

	return m_ThreadPool->enqueue(copyAndUnpack);
}

template<typename T, typename U>
inline void ModMerger::UnpackBarrier(T&& sourcePath, U&& targetPath, std::barrier<>& barrier)
{
	auto copyAndUnpack = [
		lsourcePath = std::forward<T>(sourcePath),
			ltargetPath = std::forward<U>(targetPath),
			arctool = std::string_view(m_ARCToolScriptPath), &barrier]() -> void
		{
			MakeBackup(lsourcePath, ltargetPath);
			const std::filesystem::path newBackupFilePath{ std::filesystem::path(ltargetPath) / std::filesystem::path(lsourcePath).filename() };
			CallARCTool(newBackupFilePath.string(), arctool);
			barrier.arrive_and_wait();
		};

		m_ThreadPool->enqueue_detach(copyAndUnpack);
}

template<typename T, typename U>
inline std::future<std::vector<std::string>> ModMerger::CompareDirectoriesAsync(T&& sourcePath, U&& targetPath)
{
	//auto comparePromise{ std::make_shared<std::promise<std::vector<std::string>>>() };
	//std::future<std::vector<std::string>> compareFuture{ comparePromise->get_future() };

	auto compareCheck = [threadPool = m_ThreadPool,
		lbaseSource = std::forward<T>(sourcePath),
		lpathToTarget = std::forward<U>(targetPath)]()
		{
			std::vector<std::future<void>> compareFutures;
			std::mutex compareFuturesMutex;

			std::atomic<int> activeTasks{};
			std::mutex activeTasksMutex;
			std::condition_variable waitCV;

			// Compare the directories
			std::shared_ptr<CompareDirectoriesArgs> compareArgs{ std::make_shared<CompareDirectoriesArgs>(activeTasks,waitCV) };
			compareArgs->threadPool = threadPool;

			const std::string sourcePath{ lbaseSource };

			activeTasks.fetch_add(1, std::memory_order_relaxed);
			threadPool->enqueue_detach(
				RecursiveCompareDirAsync,
				std::string_view(lbaseSource),
				lbaseSource,
				std::string_view(lpathToTarget),
				compareArgs);

			std::unique_lock lock(activeTasksMutex);
			waitCV.wait(lock, [&activeTasks = std::as_const(activeTasks)]
				{
					return activeTasks.load(std::memory_order_relaxed) == 0;
				});

			return compareArgs->filesToMove;
			//comparePromise->set_value(compareArgs->filesToMove);
		};

	return m_ThreadPool->enqueue(compareCheck);
}