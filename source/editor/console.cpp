#include <imgui.h>

#include "console.h"
#include "renderer/device.h"

void ashenvale::editor::console::initialize()
{
    ashenvale::renderer::device::g_device->QueryInterface(ashenvale::renderer::device::g_infoQueue.put());

    D3D11_INFO_QUEUE_FILTER filter = {};

    D3D11_MESSAGE_SEVERITY severities[] = {D3D11_MESSAGE_SEVERITY_ERROR, D3D11_MESSAGE_SEVERITY_WARNING,
                                           D3D11_MESSAGE_SEVERITY_CORRUPTION, D3D11_MESSAGE_SEVERITY_MESSAGE,
                                           D3D11_MESSAGE_SEVERITY_INFO};

    D3D11_MESSAGE_ID blockedIDs[] = {D3D11_MESSAGE_ID_OMSETRENDERTARGETS_UNBINDDELETINGOBJECT};

    filter.AllowList.NumSeverities = _countof(severities);
    filter.AllowList.pSeverityList = severities;
    filter.DenyList.pIDList = blockedIDs;
    filter.DenyList.NumIDs = _countof(blockedIDs);

    ashenvale::renderer::device::g_infoQueue->PushRetrievalFilter(&filter);
}

void ashenvale::editor::console::render()
{
    ImGui::Begin("Console");

    if (ImGui::Button("Clear"))
        renderer::device::g_infoQueue->ClearStoredMessages();
    ImGui::Separator();

    ImGui::BeginChild("LogRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (renderer::device::g_infoQueue)
    {
        UINT64 numMessages = renderer::device::g_infoQueue->GetNumStoredMessagesAllowedByRetrievalFilter();

        for (UINT64 i = 0; i < numMessages; i++)
        {
            SIZE_T messageLength = 0;
            renderer::device::g_infoQueue->GetMessage(i, nullptr, &messageLength);

            std::vector<char> messageData(messageLength);
            auto message = reinterpret_cast<D3D11_MESSAGE *>(messageData.data());
            renderer::device::g_infoQueue->GetMessage(i, message, &messageLength);

            ImVec4 color;
            switch (message->Severity)
            {
            case D3D11_MESSAGE_SEVERITY_CORRUPTION:
                color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                break; // bright red
            case D3D11_MESSAGE_SEVERITY_ERROR:
                color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                break; // red
            case D3D11_MESSAGE_SEVERITY_WARNING:
                color = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break; // yellow
            case D3D11_MESSAGE_SEVERITY_INFO:
                color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
                break; // blue-ish
            case D3D11_MESSAGE_SEVERITY_MESSAGE:
                color = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                break; // white
            default:
                color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                break; // gray
            }

            std::string severity;
            switch (message->Severity)
            {
            case D3D11_MESSAGE_SEVERITY_CORRUPTION:
                severity = "[D3D11 CORRUPTION] ";
                break;
            case D3D11_MESSAGE_SEVERITY_ERROR:
                severity = "[D3D11 ERROR] ";
                break;
            case D3D11_MESSAGE_SEVERITY_WARNING:
                severity = "[D3D11 WARNING] ";
                break;
            case D3D11_MESSAGE_SEVERITY_INFO:
                severity = "[D3D11 INFO] ";
                break;
            case D3D11_MESSAGE_SEVERITY_MESSAGE:
                severity = "[D3D11 MESSAGE] ";
                break;
            default:
                severity = "[D3D11 UNKNOWN] ";
                break;
            }

            std::string fullMsg = severity + std::string(message->pDescription);

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            // Draw severity label (without wrapping)
            ImGui::TextUnformatted(severity.c_str());

            // Pop color, so rest is default color
            ImGui::PopStyleColor();

            // Draw the rest of the message on the same line, wrapped if needed
            ImGui::SameLine();
            ImGui::TextWrapped("%s", message->pDescription);
        }
    }
    ImGui::EndChild();

    ImGui::End();
}