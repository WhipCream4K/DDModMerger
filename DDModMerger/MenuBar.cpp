#include "MenuBar.h"

#include "imgui.h"
#include "ContentManager.h"
#include "MergeArea.h"
#include "DirTreeCreator.h"
#include "ModMerger.h"
#include "ThreadPool.h"
#include "FileCloneUtility.h"

void MenuBar::Draw()
{

	if (ImGui::Button("Refresh"))
	{
		if (m_MergeTask->IsFinished())
		{
			ThreadPool::Init(m_ThreadCount);
		}

		m_RefreshTask->Execute();
		m_RefButtonPressed = true;
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
			if (m_MergeTask->IsARCToolExist() && m_MergeTask->IsFinished() && m_RefButtonPressed)
			{
				ImGui::OpenPopup("Merge Confirmation");
			}
			else
			{
				ImGui::OpenPopup("Merge Error");
			}

			if (ImGui::BeginPopupModal("Merge Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (!m_MergeTask->IsARCToolExist())
				{
					ImGui::Text("ARC Tool not found");
				}
				else if (!m_MergeTask->IsFinished())
				{
					ImGui::Text("Merge operation is already started");
				}

				if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
					ImGui::IsMouseClicked(ImGuiMouseButton_Right))
				{
					m_MergeButtonPressed = false;
				}

				if (!m_RefButtonPressed)
				{
					ImGui::Text("No mods to overwrite");
				}

				ImGui::EndPopup();
			}

			if (ImGui::BeginPopupModal("Merge Confirmation", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("Are you sure you want to merge?");
				ImGui::Separator();

				if (ImGui::Button("Yes", ImVec2(120.0f, 0.0f)))
				{

					ThreadPool::Init(m_ThreadCount);

					m_MergeTask->Execute();
					m_MergeButtonPressed = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("No", ImVec2(120.0f, 0)))
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

	ImGui::SameLine();

	// TODO: display mods that is inside mods folder
	ImGui::Checkbox("Show Non-Overwrite mods", &m_ShowNonOverwriteMods);

	ImGui::SameLine();

	if (ImGui::Button("Copy..."))
	{
		ImGui::OpenPopup("##Copy");
	}

	// TODO: Maybe do copy task
	if (ImGui::BeginPopup("##Copy"))
	{
		if(ImGui::MenuItem("Copy merge file to game folder"))
		{

		}

		if(ImGui::MenuItem("Copy backup file to game folder"))
		{
			
		}

		if(ImGui::MenuItem("Copy all mod files to game folder"))
		{
			
		}
		
		ImGui::EndPopup();
	}
}

void RefreshTask::Execute()
{
	auto contentManager = m_ContentManager.lock();
	auto mergeArea = m_MergeArea.lock();
	auto dirTreeCreator = m_DirTreeCreator.lock();

	if (contentManager && mergeArea && dirTreeCreator)
	{
		contentManager->LoadModsContentAsync();
		mergeArea->SetMergeAreaVisible(true);
		dirTreeCreator->CreateDirTreeAsync();
	}
}

bool MergeTask::IsARCToolExist() const
{
	return std::filesystem::exists(m_ARCToolPath);
}

bool MergeTask::IsFinished() const
{
	if (auto modMerger = m_ModMerger.lock())
	{
		return modMerger->IsReadyToMerge();
	}

	return false;
}


void MergeTask::Execute()
{
	auto modMerger = m_ModMerger.lock();
	auto mergeArea = m_MergeArea.lock();
	auto dirTreeCreator = m_DirTreeCreator.lock();
	auto fileCloneUtility = m_CloneUtility.lock();

	if (modMerger && mergeArea && dirTreeCreator)
	{
		const auto& userOverwriteOrder{ mergeArea->GetUserModsOverwriteOrder() };
		const auto& dirTree{ dirTreeCreator->GetDirTree() };

		if (fileCloneUtility)
		{
			for (const auto& [fileName, paths] : userOverwriteOrder)
			{
				fileCloneUtility->BackupMainFile(dirTree, paths);
			}
		}

		modMerger->MergeContentAsync(
			dirTreeCreator->GetDirTree(),
			mergeArea->GetUserModsOverwriteOrder(), true);
	}
}
