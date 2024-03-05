#pragma once
#include "Widget.h"

#include "Types.h"

class ContentManager;
class MergeArea : public Widget
{
public:

	MergeArea(
		const std::shared_ptr<ContentManager>& contentManager)
		: m_ContentManager(contentManager)
	{
	}

	void SetMergeAreaVisible(bool visible);

	void Draw() override;
	~MergeArea() = default;

private:

	std::shared_ptr<ContentManager> m_ContentManager;

	powe::details::ModsOverwriteOrder m_ModsOverwriteOrderTemp{};

	std::string m_SelectedMainFileName{};
	int m_SelectedModFileIndex{-1};

	bool m_MergeAreaVisible{};
	bool m_RefreshModsContent{};
};

