#pragma once
#include "Widget.h"
#include "Command.h"
#include "Types.h"


class ContentManager;
class MergeArea;
class DirTreeCreator;
class RefreshTask : public Command
{
public:

	explicit RefreshTask(
		std::shared_ptr<ContentManager> contentManager, 
		std::shared_ptr<MergeArea> mergeArea,
		std::shared_ptr<DirTreeCreator> dirTreeCreator)
		: m_ContentManager(contentManager)
		, m_MergeArea(mergeArea)
		, m_DirTreeCreator(dirTreeCreator)
	{
	}

	virtual void Execute();

	~RefreshTask() = default;

private:

	std::weak_ptr<ContentManager> m_ContentManager;
	std::weak_ptr<MergeArea> m_MergeArea;
	std::weak_ptr<DirTreeCreator> m_DirTreeCreator;
};

class ModMerger;
class FileCloneUtility;
class MergeTask : public Command
{
public:

	MergeTask(std::shared_ptr<ModMerger> modMerger,
		std::shared_ptr<MergeArea> mergeArea,
		std::shared_ptr<DirTreeCreator> dirTreeCreator,
		std::shared_ptr<FileCloneUtility> cloneUtility,
		const std::string& arctoolPath)
		: m_ModMerger(modMerger)
		, m_MergeArea(mergeArea)
		, m_DirTreeCreator(dirTreeCreator)
		, m_CloneUtility(cloneUtility)
		, m_ARCToolPath(arctoolPath)
	{
	}

	bool IsARCToolExist() const;
	bool IsFinished() const;

	virtual void Execute();

	~MergeTask() = default;

private:

	std::weak_ptr<ModMerger> m_ModMerger;
	std::weak_ptr<DirTreeCreator> m_DirTreeCreator;
	std::weak_ptr<MergeArea> m_MergeArea;
	std::weak_ptr<FileCloneUtility> m_CloneUtility;
	std::string m_ARCToolPath;
};


class MenuBar : public Widget
{
public:

	MenuBar(std::unique_ptr<RefreshTask>&& refreshTask,
			std::unique_ptr<MergeTask>&& mergeTask)
		: m_RefreshTask(std::move(refreshTask))
		, m_MergeTask(std::move(mergeTask))
	{
	}

	void Draw() override;
	~MenuBar() = default;

private:

	std::unique_ptr<RefreshTask> m_RefreshTask;
	std::unique_ptr<MergeTask> m_MergeTask;

	int m_ThreadCount{ int(std::thread::hardware_concurrency()) };
	int m_NewThreadCount{  int(std::thread::hardware_concurrency()) };

	bool m_MergeButtonPressed{};
	bool m_RefButtonPressed{};
	bool m_ShowNonOverwriteMods{};
};

