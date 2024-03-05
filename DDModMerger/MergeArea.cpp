#include "MergeArea.h"

#include <execution>

#include "ContentManager.h"

#include "imgui.h"
#include "EnvironmentVariables.h"

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
		// Add additional vertical spacing using ImGui::Dummy()
		ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeightWithSpacing()));

		ImGui::Text("Overriding ARC Files");

		ImGui::Dummy(ImVec2(0.0f, ImGui::GetFrameHeight()));


		if (!m_RefreshModsContent)
		{
			m_ModsOverwriteOrderTemp = m_ContentManager->GetAllModsOverwriteOrder();
			m_RefreshModsContent = true;
		}

		auto& modsOverwriteOrder{ m_ModsOverwriteOrderTemp };


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

			// TODO: Do all the list of selectables files here
			for (auto& [fileName, path] : modsOverwriteOrder)
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
					//ImGui::SetTooltip(path[0].c_str());
				}


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
					modsName = modsName.substr(m_ContentManager->GetModsFilePath().size() + 1); // + 1 for the '/'
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

					ImGui::Spacing();
				}
			}

			ImGui::EndChild();
		}

	}

}


