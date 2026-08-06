#pragma once

#include <imgui.h>

#include <vector>
#include <optional>

#include "geonames.hpp"

inline std::optional<GeoNames> GeoNamesImGui(int maxResults = 8)
{
    static char pattern[256];
    static std::vector<GeoNames> results;
    std::optional<GeoNames> result;
    static int index = -1;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputTextWithHint("##ImGuiGeoNames", "Location...", pattern, sizeof(pattern)))
    {
        GeoNamesQuery(results, maxResults, pattern);
        index = results.empty() ? -1 : 0;
    }
    if (!results.empty())
    {
        ImGui::BeginChild("##ImGuiGeoNamesResults", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * 6), true);
        if (ImGui::IsItemFocused() || ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
        {
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
            {
                index = (index + 1) % results.size();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
            {
                index = (index - 1 + results.size()) % results.size();
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Enter) && index >= 0)
            {
                result = results[index];
                results.clear();
                ImGui::EndChild();
                return result;
            }
        }
        for (int i = 0; i < results.size(); i++)
        {
            bool selected = (i == index);
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
            }
            if (ImGui::Selectable(results[i].Location.c_str(), selected))
            {
                result = results[i];
                results.clear();
                ImGui::PopStyleColor(selected ? 1 : 0);
                ImGui::EndChild();
                return result;
            }
            if (selected)
            {
                ImGui::PopStyleColor();
            }
        }
        ImGui::EndChild();
    }
    return result;
}