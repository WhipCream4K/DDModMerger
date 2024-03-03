// DDModMerger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "DirTreeCreator.h"
#include "ModMerger.h"
#include "ContentManager.h"
#include "Types.h"
#include "thread_pool/thread_pool.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
	try
	{
		CVarReader cvReader;
		cvReader.ParseArguments(argc, argv);

		std::shared_ptr<powe::ThreadPool> threadPool{ std::make_shared<powe::ThreadPool>(std::thread::hardware_concurrency()) };

		DirTreeCreator dirTreeCreator(cvReader,threadPool);
		std::unordered_map<std::string, std::string> dirTree{ dirTreeCreator.CreateDirTree() };

		// Create overwrite order for ModMerger
		std::shared_ptr<ContentManager> contentManager{ std::make_shared<ContentManager>(cvReader,threadPool) };
		contentManager->LoadModsOverwriteOrder();

		std::shared_ptr<ModMerger> modMerger{ std::make_shared<ModMerger>(cvReader,threadPool) };
		modMerger->MergeContent(dirTree,contentManager->GetAllModsOverwriteOrder());
	}
	catch (const std::exception& e)
	{
		std::cerr << "Error: " << e.what() << '\n';
	}


	return 0;
}

