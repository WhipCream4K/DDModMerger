// DDModMerger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <any>
#include <variant>

#include "DirTreeCreator.h"
#include "ModMerger.h"
#include "ContentManager.h"
#include "Types.h"
#include "ThreadPool.h"
#include "CVarReader.h"
#include "MenuBar.h"
#include "MergeArea.h"
#include "FileCloneUtility.h"
#include "LowFrequencyThreadPool.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"


namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

enum UserContextVariable
{
	ThreadCount,
	NewThreadCount,
	MergeConfirmation,
	Refresh,
	ModsOverwriteSelection
};

void framebuffer_size_callback([[maybe_unused]] GLFWwindow* window, int width, int height)
{
	// Calculate the new size of the ImGui window
	ImVec2 newSize = ImVec2(float(width), float(height));

	// Calculate the position of the ImGui window to be in the middle of the GLFW window
	ImVec2 newPosition = ImVec2((width - newSize.x) / 2.0f, (height - newSize.y) / 2.0f);

	// Update the size and position of the ImGui window
	ImGui::SetWindowSize("ModMerger", newSize);
	ImGui::SetWindowPos("ModMerger", newPosition);
}

int main(int argc, char* argv[])
{
	CVarReader cvReader{};
	cvReader.ParseArguments(argc, argv);

	if (!cvReader.CheckArgs())
		return -1;

	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return -1;

	// GL 3.0 + GLSL 130
	const std::string glslVersion{ "#version 130" };
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	const int windowWidth{ 640 };
	const int windowHeight{ 480 };

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "DDModMerger", NULL, NULL);
	if (window == NULL)
		return -1;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // Enable vsync

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glslVersion.c_str()); // Pass your OpenGL version here
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	// Initialize DDModManager Global Context Variables

	ThreadPool::Init(std::thread::hardware_concurrency());
	LowFrequencyThreadPool::Init(std::thread::hardware_concurrency() / 2);

	std::shared_ptr<ContentManager> contentManager{ std::make_shared<ContentManager>(cvReader) };
	std::shared_ptr<DirTreeCreator> dirTreeCreator{ std::make_shared<DirTreeCreator>(cvReader) };
	std::shared_ptr<FileCloneUtility> cloneUtility{ std::make_shared<FileCloneUtility>(cvReader) };
	std::shared_ptr<ModMerger> modMerger{ std::make_shared<ModMerger>(cvReader) };

	// Initialize Widgets
	std::shared_ptr<MergeArea> mergeArea{ std::make_shared<MergeArea>(contentManager,dirTreeCreator,modMerger) };
	std::shared_ptr<MenuBar> menuBar{ std::make_shared<MenuBar>(
		std::make_unique<RefreshTask>(contentManager,mergeArea,dirTreeCreator),
		std::make_unique<MergeTask>(modMerger,mergeArea,dirTreeCreator,cloneUtility,cvReader.ReadCVar("-arctool"))) };


	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// (1) Render your GUI with ImGui functions here
		ImGui::Begin("ModMerger", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		ImGui::SetWindowSize("ModMerger", ImVec2(windowWidth, windowHeight), ImGuiCond_Once);

		menuBar->Draw();

		ImGui::Dummy(ImVec2(0.0f, 10.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		if (ImGui::BeginChild("ChildR", ImVec2(-1.0f, -1.0f), ImGuiChildFlags_Border))
		{
			ImGui::PopStyleVar();

			mergeArea->Draw();

			ImGui::EndChild();
		}


		ImGui::End();

		// Rendering
		ImGui::Render();
		int display_w, display_h;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Exiting
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

