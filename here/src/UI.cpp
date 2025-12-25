#include "UI.hpp"
#include "ParticleWorld.hpp"
#include <imgui.h>
#include <imgui-SFML.h>

UI::UI(sf::RenderWindow& window, ParticleWorld* worldPtr) : world(worldPtr) {
    // ImGui-SFML initialization is handled in SandSimApp
}

void UI::update(sf::RenderWindow& window, sf::Time deltaTime, bool& simRunning, float frameTime) {
    ImGui::SFML::Update(window, deltaTime);

    // Create a Window that can be moved and resized
    ImGui::Begin("Simulation Control");

    ImGui::Text("Performance: %.1f FPS", 1000.0f / frameTime);
    ImGui::Checkbox("Simulation Running", &simRunning);
    
    ImGui::Separator();
    
    ImGui::SliderFloat("Brush Radius", &selectionRadius, MIN_SELECTION_RADIUS, MAX_SELECTION_RADIUS);

    if (ImGui::Button("Clear Canvas", ImVec2(-1, 0))) {
        world->clear();
    }
    
    if (ImGui::Button("Save World", ImVec2(-1, 0))) {
        world->saveWorld("world");
    }

    ImGui::Spacing();
    ImGui::Text("Elements");
    
    drawMaterialTabs();

    ImGui::End();
}

void UI::drawMaterialTabs() {
    if (ImGui::BeginTabBar("Categories")) {
        
        auto renderGroup = [&](const char* label, MaterialGroup group) {
            if (ImGui::BeginTabItem(label)) {
                // Automatically create buttons for any material in this group
                for (const auto& mat : ALL_MATERIALS) {
                    if (mat.group == group) {
                        // Style button with the material's color
                        ImVec4 col = ImVec4(mat.color.r/255.f, mat.color.g/255.f, mat.color.b/255.f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Button, col);
                        
                        if (ImGui::Button(mat.name.c_str(), ImVec2(80, 40))) {
                            currentMaterial = mat.id;
                        }
                        
                        // Highlight if selected
                        if (currentMaterial == mat.id) {
                            ImGui::GetWindowDrawList()->AddRect(
                                ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), 
                                IM_COL32(255, 255, 0, 255), 0, 0, 2.0f
                            );
                        }

                        ImGui::PopStyleColor();
                        
                        // Simple grid layout: stay on same line if there's room
                        if (ImGui::GetItemRectMax().x < ImGui::GetWindowPos().x + ImGui::GetWindowWidth() - 90)
                            ImGui::SameLine();
                    }
                }
                ImGui::EndTabItem();
            }
        };

        renderGroup("Solids", MaterialGroup::MovableSolid);
        renderGroup("Static", MaterialGroup::ImmovableSolid);
        renderGroup("Liquids", MaterialGroup::Liquid);
        renderGroup("Gases", MaterialGroup::Gas);
        renderGroup("Special", MaterialGroup::Special);

        ImGui::EndTabBar();
    }
}

void UI::render(sf::RenderWindow& window) {
    ImGui::SFML::Render(window);
}

bool UI::isMouseOverUI() const {
    return ImGui::GetIO().WantCaptureMouse;
}