#include "planets/Renderer.hpp"
#include "planets/GUI.hpp"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cmath>

GUI::GUI() {
    fpsHistory.clear();
}

GUI::~GUI() {
    shutdown();
}

bool GUI::init(GLFWwindow* win) {
    window = win;
    // Renderer is responsible for ImGui initialization (context + backends).
    // GUI only stores the window pointer and uses ImGui APIs afterward.
    return true;
}

bool GUI::wantsCaptureKeyboard() const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

void GUI::shutdown() {
    // GUI does not own ImGui context; Renderer handles ImGui shutdown.
    window = nullptr;
}

void GUI::newFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GUI::render(Simulation& sim, Camera& camera, Renderer& renderer, float deltaTime) {
    if (!visible) {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return;
    }
    
    // Calculate FPS
    float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 0.0f;
    fpsHistory.push_back(fps);
    if (fpsHistory.size() > FPS_HISTORY_SIZE) {
        fpsHistory.pop_front();
    }
    lastFPS = fps;
    
    auto& planets = sim.getPlanets();
    lastPlanetCount = static_cast<int>(planets.size());
    
    // Main control window pinned to left column with fixed width
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)PANEL_WIDTH, io.DisplaySize.y));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::Begin("Simulation Control", &visible, flags)) {
        // === STATS SECTION ===
        if (ImGui::CollapsingHeader("Statistics", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Text("FPS: %.1f", lastFPS);
            
            // FPS graph
            float fpsArray[FPS_HISTORY_SIZE];
            size_t count = std::min(fpsHistory.size(), FPS_HISTORY_SIZE);
            for (size_t i = 0; i < count; ++i) {
                fpsArray[i] = fpsHistory[i];
            }
            ImGui::PlotLines("##FPS", fpsArray, static_cast<int>(count), 0, nullptr, 0.0f, 120.0f, ImVec2(0, 50));
            
            ImGui::Separator();
            ImGui::Text("Bodies: %d", lastPlanetCount);
            ImGui::Text("Zoom: %.3f", camera.getZoom());
        }
        
        ImGui::Spacing();
        
        // === CONTROLS SECTION ===
        if (ImGui::CollapsingHeader("Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            // Pause/Play
            if (isPaused) {
                if (ImGui::Button("Play", ImVec2(100, 0))) {
                    isPaused = false;
                }
            } else {
                if (ImGui::Button("Pause", ImVec2(100, 0))) {
                    isPaused = true;
                }
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Reset Camera", ImVec2(170, 0))) {
                camera.reset();
            }
            
            // Time scale (linear slider from 0.01x to 10.0x)
            ImGui::SliderFloat("Time Scale", &timeScale, 0.01f, 10.0f, "%.2f x");
            
            ImGui::Separator();
            
            // Gravity controls (using multipliers)
            if (ImGui::SliderFloat("Gravity", &gravityMultiplier, 0.1f, 5.0f, "%.2f x")) {
                sim.setGravityParams(BASE_GRAVITY * gravityMultiplier, BASE_SOFTENING * softeningMultiplier);
            }
            
            if (ImGui::SliderFloat("Softening", &softeningMultiplier, 0.1f, 5.0f, "%.2f x")) {
                sim.setGravityParams(BASE_GRAVITY * gravityMultiplier, BASE_SOFTENING * softeningMultiplier);
            }
            
            ImGui::Separator();
            
            // Collisions removed from UI
        }
        
        ImGui::Spacing();
        
        // === SIMULATION SECTION ===
        if (ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Reinitialize (12 bodies)", ImVec2(-1, 0))) {
                sim.initRandom(12, static_cast<unsigned>(ImGui::GetTime() * 1000));
                // Clear both renderer-managed trails and per-planet trails
                renderer.clearTrails();
                for (auto &p : sim.getPlanets()) p.clearTrail();
                camera.reset();
                restartTriggered = true;
            }
            
            static int bodyCount = 20;
            ImGui::InputInt("Body Count", &bodyCount);
            bodyCount = std::max(1, std::min(bodyCount, 200));
            
            if (ImGui::Button("Create Custom Simulation", ImVec2(-1, 0))) {
                sim.initRandom(bodyCount, static_cast<unsigned>(ImGui::GetTime() * 1000));
                renderer.clearTrails();
                for (auto &p : sim.getPlanets()) p.clearTrail();
                camera.reset();
                restartTriggered = true;
            }
        }
        
        ImGui::Spacing();
        
        // === CAMERA SECTION ===
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            float zoomSpeed = 0.1f;
            if (ImGui::Button("-", ImVec2(40, 0))) {
                camera.zoomBy(1.0f - zoomSpeed);
            }
            ImGui::SameLine();
            if (ImGui::Button("+", ImVec2(40, 0))) {
                camera.zoomBy(1.0f + zoomSpeed);
            }
            ImGui::SameLine();
            ImGui::Text("Zoom Controls");

            float outlierMult = camera.getOutlierMultiplier();
            if (ImGui::SliderFloat("Outlier Multiplier", &outlierMult, 1.0f, 10.0f, "%.1f x")) {
                camera.setOutlierMultiplier(outlierMult);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Exclude planets farther than this multiple of the median distance to COM from auto-zoom");
            }
        }
        
        ImGui::Spacing();
        ImGui::Separator();
    }
    ImGui::End();
    
    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool GUI::wantsCaptureMouse() const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}
