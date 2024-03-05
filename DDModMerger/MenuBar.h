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

	std::shared_ptr<ContentManager> m_ContentManager;
	std::shared_ptr<MergeArea> m_MergeArea;
	std::shared_ptr<DirTreeCreator> m_DirTreeCreator;
};

class ModMerger;
class MergeTask : public Command
{
public:

	MergeTask(std::shared_ptr<ModMerger> modMerger,
		std::shared_ptr<MergeArea> mergeArea,
		std::shared_ptr<DirTreeCreator> dirTreeCreator)
		: m_ModMerger(modMerger)
		, m_MergeArea(mergeArea)
		, m_DirTreeCreator(dirTreeCreator)
	{
	}

	virtual void Execute();

	~MergeTask() = default;

private:

	std::shared_ptr<ModMerger> m_ModMerger;
	std::shared_ptr<DirTreeCreator> m_DirTreeCreator;
	std::shared_ptr<MergeArea> m_MergeArea;
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
	bool m_MergeButtonPressed{};
	int m_ThreadCount{ int(std::thread::hardware_concurrency()) };
	int m_NewThreadCount{ int(std::thread::hardware_concurrency()) };
};

