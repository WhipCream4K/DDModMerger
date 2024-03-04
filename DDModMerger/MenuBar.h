#pragma once
#include "Widget.h"
#include "Command.h"
#include "Types.h"


class ContentManager;
class RefreshTask : public Command
{
public:

	RefreshTask(std::shared_ptr<ContentManager> contentManager)
		: m_ContentManager(contentManager)
	{
	}

	virtual void Execute();

	~RefreshTask() = default;

private:

	std::shared_ptr<ContentManager> m_ContentManager;
};

class MenuBar : public Widget
{
public:

	void Draw() override;
	~MenuBar() = default;

private:

	std::unique_ptr<RefreshTask> m_RefreshTask;
	bool m_MergeButtonPressed{};
	int m_ThreadCount{int(std::thread::hardware_concurrency())};
	int m_NewThreadCount{int(std::thread::hardware_concurrency())};
};

