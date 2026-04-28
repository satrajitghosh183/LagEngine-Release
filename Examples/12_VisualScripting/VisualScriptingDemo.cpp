#include "../../Engine/Core/Application.hpp"
#include "../../Engine/Core/EntryPoint.hpp"
#include "../../Engine/Core/Logger.hpp"
#include "../../Engine/Scripting/VisualScript/VisualScriptGraph.hpp"
#include "../../Engine/Scripting/VisualScript/VisualScriptVM.hpp"
#include "../../Engine/Scripting/VisualScript/VisualScriptNodes.hpp"
#include "../../Engine/Scripting/VisualScript/VisualScriptSerializer.hpp"
#include <imgui.h>

using namespace GameEngine;

class VisualScriptDemoApp : public Application {
public:
    VisualScriptDemoApp() : Application("Visual Scripting Demo") {}

protected:
    void OnInit() override {
        registerBuiltinNodes();

        // Create a simple demo graph: EventUpdate -> Print "Hello"
        m_Graph = CreateRef<VisualScriptGraph>();
        m_Graph->setName("Demo Graph");

        // Add an Event Update node
        auto updateNode = CreateRef<EventUpdateNode>();
        uint32_t updateID = m_Graph->addNode(updateNode);

        // Add a print node
        auto printNode = CreateRef<PrintNode>();
        uint32_t printID = m_Graph->addNode(printNode);
        printNode->setPinValue(printNode->m_Message, std::string("Frame update!"));

        // Connect flow: Update -> Print
        m_Graph->addLink(updateID, updateNode->m_FlowOut,
                          printID, printNode->m_FlowIn);

        // Create VM
        m_VM = CreateRef<VisualScriptVM>();
        m_VM->initialize(m_Graph);
        m_VM->setLogCallback([this](const std::string& msg) {
            m_LogMessages.push_back(msg);
            if (m_LogMessages.size() > 20) m_LogMessages.erase(m_LogMessages.begin());
        });

        GE_CORE_INFO("Visual Script demo initialized with {} nodes, {} links",
                      m_Graph->getNodes().size(), m_Graph->getLinks().size());
    }

    void OnUpdate(float dt) override {
        m_FrameCount++;
        // Only execute every 60 frames to avoid log spam
        if (m_Running && m_FrameCount % 60 == 0) {
            m_VM->update(dt);
        }
    }

    // Application doesn't expose a dedicated ImGui hook — it's part of
    // OnRender(). We provide an inline UI helper called from OnRender.
    void OnImGuiRender() {
        ImGui::Begin("Visual Scripting Demo");

        ImGui::Text("Graph: %s", m_Graph->getName().c_str());
        ImGui::Text("Nodes: %zu | Links: %zu",
                     m_Graph->getNodes().size(), m_Graph->getLinks().size());
        ImGui::Text("Executed nodes: %d", m_VM->getExecutedNodeCount());

        ImGui::Separator();

        if (ImGui::Button(m_Running ? "Stop" : "Run")) {
            m_Running = !m_Running;
        }
        ImGui::SameLine();
        if (ImGui::Button("Step")) {
            m_VM->update(1.0f / 60.0f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset")) {
            m_VM->reset();
            m_LogMessages.clear();
        }

        ImGui::Separator();
        ImGui::Text("Available Node Types:");
        auto types = VisualScriptNodeRegistry::instance().getRegisteredTypes();
        for (auto& type : types) {
            ImGui::BulletText("%s", type.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Log:");
        for (auto& msg : m_LogMessages) {
            ImGui::TextWrapped("%s", msg.c_str());
        }

        ImGui::Separator();
        if (ImGui::Button("Save Graph to JSON")) {
            VisualScriptSerializer::saveToFile(*m_Graph, "demo_graph.vsgraph");
            m_LogMessages.push_back("Saved to demo_graph.vsgraph");
        }

        ImGui::End();
    }

private:
    Ref<VisualScriptGraph> m_Graph;
    Ref<VisualScriptVM> m_VM;
    bool m_Running = false;
    int m_FrameCount = 0;
    std::vector<std::string> m_LogMessages;
};

Application* GameEngine::CreateApplication() {
    return new VisualScriptDemoApp();
}
