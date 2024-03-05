#pragma once
#include "Widget.h"

#include "Types.h"

class ContentManager;
class DirTreeCreator;
class MergeArea : public Widget
{
public:

	MergeArea(
		const std::shared_ptr<ContentManager>& contentManager,
		const std::shared_ptr<DirTreeCreator>& dirTreeCreator)
		: m_ContentManager(contentManager)
		, m_DirTreeCreator(dirTreeCreator)
	{
	}

	void SetMergeAreaVisible(bool visible);

	const powe::details::ModsOverwriteOrder& GetUserModsOverwriteOrder() const
	{
		return m_ModsOverwriteOrderTemp;
	}

	void Draw() override;
	~MergeArea() = default;

private:

	std::shared_ptr<ContentManager> m_ContentManager;
	std::shared_ptr<DirTreeCreator> m_DirTreeCreator;

	powe::details::ModsOverwriteOrder m_ModsOverwriteOrderTemp{};
	const powe::details::DirectoryTree* m_DirTreeTemp{};

	std::string m_SelectedMainFileName{};
	std::string m_PopupFileName{};
	int m_SelectedModFileIndex{ -1 };

	bool m_MergeAreaVisible{};
	bool m_RefreshModsContent{};
};

