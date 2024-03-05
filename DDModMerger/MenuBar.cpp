#include "MenuBar.h"

#include "imgui.h"
#include "ContentManager.h"
#include "MergeArea.h"
#include "DirTreeCreator.h"
#include "ModMerger.h"

void MenuBar::Draw()
{

	if (ImGui::Button("Refresh"))
	{
		m_RefreshTask->Execute();
	}

	ImGui::SameLine(); // Align the next item to the right of the previous item

	// Merge Files
	{
		if (ImGui::Button("Merge"))
		{
			// Handle Merge button click event
			m_MergeButtonPressed = true;
		}

		// Render the merge confirmation box
		if (m_MergeButtonPressed)
		{
			ImGui::OpenPopup("Merge Confirmation");

			if (ImGui::BeginPopupModal("Merge Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Are you sure you want to merge?");
				ImGui::Separator();

				if (ImGui::Button("Yes", ImVec2(120.0f, 0.0f)))
				{
					// Close the merge confirmation box
					m_MergeTask->Execute();
					m_MergeButtonPressed = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("No", ImVec2(120, 0)))
				{
					// Close the merge confirmation box
					m_MergeButtonPressed = false;
				}

				ImGui::EndPopup();
			}
		}
	}

	ImGui::SameLine(); // Align the next item to the right of the previous item

	// Inside the ImGui window
	ImGui::SetNextItemWidth(100.0f);
	auto& threadCount = m_ThreadCount;
	auto& newThreadCount = m_NewThreadCount;

	if (ImGui::InputInt("Thread Count", &threadCount, 1, 100))
	{
		// Find the nearest power of 2 value
		if (threadCount > newThreadCount)
		{
			newThreadCount *= 2;
			threadCount = newThreadCount;
		}
		else
		{
			newThreadCount /= 2;
			threadCount = newThreadCount;
		}

		if (threadCount > 48)
		{
			threadCount = 48;
			newThreadCount = 48;
		}
		else if (threadCount < 1)
		{
			threadCount = 1;
			newThreadCount = 1;
		}
	}
}

void RefreshTask::Execute()
{
	m_ContentManager->LoadModsContentAsync();
	m_MergeArea->SetMergeAreaVisible(true);
	m_DirTreeCreator->CreateDirTreeAsync();
}

void MergeTask::Execute()
{
	m_ModMerger->MergeContentAsync(
		m_DirTreeCreator->GetDirTree(),
		m_MergeArea->GetUserModsOverwriteOrder(),
		true);
}
