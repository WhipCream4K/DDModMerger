#pragma once
#include "Widget.h"

#include "Types.h"

#include <map>

class ContentManager;
class DirTreeCreator;
class ModMerger;
class MergeArea : public Widget
{
public:

	MergeArea(
		const std::shared_ptr<ContentManager>& contentManager,
		const std::shared_ptr<DirTreeCreator>& dirTreeCreator,
		const std::shared_ptr<ModMerger>& modMerger)
		: m_ContentManager(contentManager)
		, m_DirTreeCreator(dirTreeCreator)
		, m_ModMerger(modMerger)
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

	void SortsOverwriteFileName(const powe::details::ModsOverwriteOrder& modsOverwriteOrder );

	std::weak_ptr<ContentManager> m_ContentManager;
	std::weak_ptr<DirTreeCreator> m_DirTreeCreator;
	std::weak_ptr<ModMerger> m_ModMerger;

	powe::details::ModsOverwriteOrder m_ModsOverwriteOrderTemp{};
	const powe::details::DirectoryTree* m_DirTreeTemp{};

	std::map<int, std::vector<std::string>,std::greater<int>> m_OverwriteCountList{};

	std::string m_SelectedMainFileName{};
	std::string m_PopupFileName{};
	int m_SelectedModFileIndex{ -1 };

	bool m_MergeAreaVisible{};
	bool m_RefreshModsContent{};
};

