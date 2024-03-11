#include "MergeArea.h"

#include <execution>

#include "DirTreeCreator.h"
#include "ContentManager.h"
#include "ModMerger.h"

#include "imgui.h"
#include "EnvironmentVariables.h"

namespace fs = std::filesystem;

void MergeArea::SetMergeAreaVisible(bool visible)
{
	m_MergeAreaVisible = visible;
	m_RefreshModsContent = false;
}

void MergeArea::Draw()
{
	if (!m_MergeAreaVisible)
		ImGui::Text("If nothing shows up here, click the 'Refresh' button above.");

	if (m_MergeAreaVisible)
	{
		auto modMerger = m_ModMerger.lock();
		auto dirTreeCreator = m_DirTreeCreator.lock();
		auto contentManager = m_ContentManager.lock();

		if (!modMerger || !dirTreeCreator || !contentManager)
			return;

		ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));

		ImGui::Text("Overwrite ARC Files");

		if (!modMerger->IsReadyToMerge())
		{
			ImGui::SameLine(0.0f, 10.0f);

			// Add animation here
			static float time = 0.0f;
			time += ImGui::GetIO().DeltaTime;
			float alpha = (sinf(time * 2.0f) + 1.0f) * 0.5f;
			ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, alpha);
			ImGui::TextColored(textColor, "Merging...");
		}

		if (!m_RefreshModsContent)
		{
			if (!contentManager->IsFinished())
				return;
			else
			{
				m_ModsOverwriteOrderTemp = contentManager->GetAllModsOverwriteOrder();
			}

			if (!dirTreeCreator->IsFinished())
				return;
			else
			{
				m_DirTreeTemp = &dirTreeCreator->GetDirTree();
			}

			SortsOverwriteFileName(m_ModsOverwriteOrderTemp);

			m_RefreshModsContent = true;
		}

		auto& modsOverwriteOrder{ m_ModsOverwriteOrderTemp };

		ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight()));

		if (m_SelectedModFileIndex >= 0)
		{
			ImGui::SameLine(m_WindowContext.width * 0.5f);
			auto& modsOrder{ modsOverwriteOrder.at(m_SelectedMainFileName) };

			if (ImGui::ArrowButton("##ModUp", ImGuiDir_Up) && m_SelectedModFileIndex > 0)
			{
				std::swap(modsOrder[m_SelectedModFileIndex], modsOrder[m_SelectedModFileIndex - 1]);
				m_SelectedModFileIndex--;
			}

			ImGui::SameLine(0.0f, 10.0f);

			if (ImGui::ArrowButton("##ModDown", ImGuiDir_Down) && m_SelectedModFileIndex < modsOrder.size() - 1)
			{
				std::swap(modsOrder[m_SelectedModFileIndex], modsOrder[m_SelectedModFileIndex + 1]);
				m_SelectedModFileIndex++;
			}
		}


		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.8f, 0.8f, 0.8f, 0.2f));
		if (ImGui::BeginChild("##ARCFiles1", ImVec2(m_WindowContext.width * 0.4f, ImGui::GetTextLineHeightWithSpacing() * 8), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY))
		{
			ImGui::PopStyleColor();

			std::vector<std::string> overwriteFileNames;

			if (ImGui::BeginTabBar("##ModsCountBar",
				ImGuiTabBarFlags_FittingPolicyResizeDown | ImGuiTabBarFlags_FittingPolicyScroll))
			{

				for (const auto& [count, fileNames] : m_OverwriteCountList)
				{
					if (ImGui::BeginTabItem(std::to_string(count).c_str()))
					{
						overwriteFileNames = fileNames;
						ImGui::EndTabItem();
					}
				}

				ImGui::EndTabBar();
			}

			// TODO: Do all the list of selectables files here
			for (const auto& fileName : overwriteFileNames)
			{

				if (ImGui::Selectable(fileName.c_str(), m_SelectedMainFileName == fileName))
				{
					if (m_SelectedMainFileName == fileName)
					{
						m_SelectedMainFileName.clear();
						m_SelectedModFileIndex = -1;
					}
					else
					{
						m_SelectedMainFileName = fileName;
						m_SelectedModFileIndex = -1;
					}
				}

				if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
				{
					ImGui::SetTooltip(m_DirTreeTemp->at(fileName).c_str());
				}

				if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
				{
					ImGui::OpenPopup("##MainFilesPopupMenu");
					m_PopupFileName = fileName;
				}
			}

			if (ImGui::BeginPopup("##MainFilesPopupMenu"))
			{
				if (ImGui::MenuItem("Open in Explorer"))
				{
					const fs::path path{ m_DirTreeTemp->at(m_PopupFileName) };
					const std::string command{ "explorer " + fs::absolute(path.parent_path()).string() };
					system(command.c_str());
				}

				ImGui::EndPopup();
			}

			ImGui::EndChild();
		}



		ImGui::SameLine(0.0f, 50.0f);
		if (ImGui::BeginChild("##ModFiles1", ImVec2(-1.0f, ImGui::GetTextLineHeightWithSpacing() * 8), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY))
		{
			if (m_SelectedMainFileName.empty())
			{
				ImGui::Text("Select a file to view its mods");
			}
			else
			{
				const auto& modsOrder{ modsOverwriteOrder.at(m_SelectedMainFileName) };

				for (size_t i = 0; i < modsOrder.size(); i++)
				{
					std::string modsName{ modsOrder[i] };
					modsName = modsName.substr(contentManager->GetModsFilePath().size() + 1); // + 1 for the '/'
					modsName = modsName.substr(0, modsName.find_first_of("/\\"));
					modsName = std::to_string(i) + "\t" + modsName;

					if (ImGui::Selectable(modsName.c_str(), m_SelectedModFileIndex == i))
					{
						if (m_SelectedModFileIndex == i)
						{
							m_SelectedModFileIndex = -1;
						}
						else
						{
							m_SelectedModFileIndex = int(i);
						}
					}

#ifdef _WIN32
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
					{
						ImGui::OpenPopup("##MainFilesPopupMenu");
						m_PopupFileName = modsOrder[i];
					}
#endif

					ImGui::Spacing();
				}

#ifdef _WIN32
				if (ImGui::BeginPopup("##MainFilesPopupMenu"))
				{
					if (ImGui::MenuItem("Open in Explorer"))
					{
						const fs::path path{ m_PopupFileName };
						const std::string command{ "explorer " + fs::absolute(path.parent_path()).string() };
						system(command.c_str());
					}

					ImGui::EndPopup();
				}
#endif
			}

			ImGui::EndChild();
		}

	}

}

void MergeArea::SortsOverwriteFileName(const powe::details::ModsOverwriteOrder& modsOverwriteOrder)
{
	for (const auto& [fileName, path] : modsOverwriteOrder)
	{
		std::vector<std::string> modFiles;
		m_OverwriteCountList[int(path.size())].emplace_back(fileName);
	}
}



