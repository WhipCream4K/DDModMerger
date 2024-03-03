// DDModMerger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "DirTreeCreator.h"
#include "ModMerger.h"
#include "ContentManager.h"
#include "Types.h"
#include "thread_pool/thread_pool.h"

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"

namespace fs = std::filesystem;

static void glfw_error_callback(int error, const char* description) {
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Calculate the new size of the ImGui window
    ImVec2 newSize = ImVec2(width, height);

    // Calculate the position of the ImGui window to be in the middle of the GLFW window
    ImVec2 newPosition = ImVec2((width - newSize.x) / 2, (height - newSize.y) / 2);

    // Update the size and position of the ImGui window
    ImGui::SetWindowSize("Resizable Window", newSize);
    ImGui::SetWindowPos("Resizable Window", newPosition);
}

int main(int argc, char* argv[])
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return -1;

	// GL 3.0 + GLSL 130
	const std::string glslVersion{ "#version 130" };
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

	const int windowWidth{ 1024 };
	const int windowHeight{ 720 };

	// Create window with graphics context
	GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Dear ImGui GLFW+OpenGL3 example", NULL, NULL);
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

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		// Start the Dear ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// (1) Render your GUI with ImGui functions here

		// Create the ImGui window
		//ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
        //ImGui::SetNextWindowPos(ImVec2(windowWidth / 2, windowHeight / 2), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::Begin("Resizable Window", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);

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


	//CVarReader cvReader;
	//cvReader.ParseArguments(argc, argv);

	//std::shared_ptr<powe::ThreadPool> threadPool{ std::make_shared<powe::ThreadPool>(std::thread::hardware_concurrency()) };

	//DirTreeCreator dirTreeCreator(cvReader, threadPool);
	//std::unordered_map<std::string, std::string> dirTree{ dirTreeCreator.CreateDirTree() };

	//// Create overwrite order for ModMerger
	//std::shared_ptr<ContentManager> contentManager{ std::make_shared<ContentManager>(cvReader,threadPool) };
	//contentManager->LoadModsOverwriteOrder();

	//std::shared_ptr<ModMerger> modMerger{ std::make_shared<ModMerger>(cvReader,threadPool) };
	//modMerger->MergeContent(dirTree, contentManager->GetAllModsOverwriteOrder());


	return 0;
}

