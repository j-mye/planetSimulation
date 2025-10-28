#include "planets/Renderer.hpp"
#include "planets/GUI.hpp"
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <cmath>

GUI::GUI() {
    fpsHistory.clear();
}

GUI::~GUI() {
    shutdown();
}

bool GUI::init(GLFWwindow* win) {
    window = win;
    
    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    // Setup style
    ImGui::StyleColorsDark();
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    std::cout << "GUI initialized successfully" << std::endl;
    return true;
}

bool GUI::wantsCaptureKeyboard() const {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

void GUI::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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
            
            // Calculate center of mass
            if (!planets.empty()) {
                float totalMass = 0.0f;
                Vector2 com(0.0f, 0.0f);
                for (const auto& p : planets) {
                    float m = p.getMass();
                    com += p.getP() * m;
                    totalMass += m;
                }
                if (totalMass > 0.0f) {
                    com = com / totalMass;
                    ImGui::Text("COM: (%.2f, %.2f)", com.getX(), com.getY());
                }
                
                // Calculate total kinetic energy
                float kineticEnergy = 0.0f;
                for (const auto& p : planets) {
                    float v2 = p.getV().getX() * p.getV().getX() + p.getV().getY() * p.getV().getY();
                    kineticEnergy += 0.5f * p.getMass() * v2;
                }
                ImGui::Text("KE: %.2f", kineticEnergy);
            }
            
            ImGui::Text("Zoom: %.3f", camera.getZoom());
            ImGui::Text("Camera: (%.2f, %.2f)", camera.getPosition().x, camera.getPosition().y);
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
            
            // Time scale (logarithmic slider for wide range)
            ImGui::SliderFloat("Time Scale", &timeScale, 0.001f, 200.0f, "%.3f x", ImGuiSliderFlags_Logarithmic);
            
            ImGui::Separator();
            
            // Gravity controls
            float g = sim.getGravityParams().first;
            if (ImGui::SliderFloat("Gravity (G)", &g, 0.01f, 0.2f, "%.3f")) {
                sim.setGravityParams(g, sim.getGravityParams().second);
            }
            
            float softening = sim.getGravityParams().second;
            if (ImGui::SliderFloat("Softening", &softening, 0.001f, 0.1f, "%.3f")) {
                sim.setGravityParams(sim.getGravityParams().first, softening);
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
        if (ImGui::CollapsingHeader("Camera")) {
            bool autoZoom = camera.isAutoZoomEnabled();
            if (ImGui::Checkbox("Auto-Zoom", &autoZoom)) {
                camera.setAutoZoom(autoZoom);
            }
            
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
        ImGui::TextDisabled("Press 'H' to toggle this panel");
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
