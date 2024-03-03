// DDModMerger.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <any>
#include <variant>

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

enum UserContextVariable
{
	ThreadCount,
	NewThreadCount,
	MergeConfirmation,
	Refresh,
};

using UserVariable = std::variant<bool, int>;

template<typename T>
T& Get(UserVariable& val)
{
	return std::get<T>(val);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
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
    CVarReader cvReader{};
    cvReader.ParseArguments(argc, argv);

    std::shared_ptr<powe::ThreadPool> threadPool{ std::make_shared<powe::ThreadPool>(std::thread::hardware_concurrency()) };
    std::shared_ptr<ContentManager> contentManager{ std::make_shared<ContentManager>(cvReader,threadPool) };

    std::unordered_map<UserContextVariable, UserVariable> userContextVariables{};

    userContextVariables[ThreadCount] = int(std::thread::hardware_concurrency());
    userContextVariables[NewThreadCount] = int(std::thread::hardware_concurrency());
    userContextVariables[MergeConfirmation] = false;

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

        if (ImGui::Button("Refresh"))
        {
            // Handle Refresh button click event
            // TODO: Implement Refresh functionality
        }

        ImGui::SameLine(); // Align the next item to the right of the previous item

        // Merge Files
        {
            if (ImGui::Button("Merge"))
            {
                // Handle Merge button click event
                // TODO: Implement Merge functionality
                userContextVariables[MergeConfirmation] = true;
            }

            auto& mergeConfirmationOpen = Get<bool>(userContextVariables[MergeConfirmation]);

            // Render the merge confirmation box
            if (mergeConfirmationOpen)
            {
                ImGui::OpenPopup("Merge Confirmation");

                if (ImGui::BeginPopupModal("Merge Confirmation", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    ImGui::Text("Are you sure you want to merge?");
                    ImGui::Separator();

                    if (ImGui::Button("Yes", ImVec2(120.0f, 0.0f)))
                    {
                        // Handle the merge confirmation
                        // TODO: Implement Merge functionality

                        // Close the merge confirmation box
                        mergeConfirmationOpen = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("No", ImVec2(120, 0)))
                    {
                        // Close the merge confirmation box
                        mergeConfirmationOpen = false;
                    }

                    ImGui::EndPopup();
                }
            }
        }

        ImGui::SameLine(); // Align the next item to the right of the previous item

        // Inside the ImGui window
        ImGui::SetNextItemWidth(100.0f);
        auto& threadCount = Get<int>(userContextVariables[ThreadCount]);
        auto& newThreadCount = Get<int>(userContextVariables[NewThreadCount]);

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

        if (ImGui::Button("Create ARC Directory Tree"))
        {
            // Handle Merge button click event
            // TODO: Implement Merge functionality
        }


        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("ChildR", ImVec2(-1.0f, -1.0f), ImGuiChildFlags_Border);
        ImGui::Text("If nothing shows up here, click the 'Refresh' button above.");

        // Add additional vertical spacing using ImGui::Dummy()
        ImGui::Dummy(ImVec2(0.0f, 50.0f));

        ImGui::Text("Overriding ARC Files");
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.8f, 0.8f, 0.8f, 0.2f));
        if (ImGui::BeginChild("##ARCFiles1", ImVec2(windowHeight * 0.4f, ImGui::GetTextLineHeightWithSpacing() * 8), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeY))
        {
            // TODO: Do all the list of detectable files here
        }

        ImGui::PopStyleColor();
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::EndChild();

        ImGui::End();

        ImGui::ShowDemoWindow();

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

