#include <iostream>
#include <sstream>
#include <array>
#include <cstring>
#include <cmath>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "ImGuizmo.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

#include "Application.hpp"
#include "Utility.hpp"
#include "meshes/BoxMesh.hpp"
#include "meshes/SphereMesh.hpp"

#include "EventDispatcher.hpp"

using namespace Eigen;

// ── Embedded shaders (Vulkan / GLSL 450) ─────────────────────────────────────
// The vertex shader receives pre-multiplied viewProj as a push constant (mat4, 64 bytes)
// and lighting data packed into the remaining 48 bytes of push constant space.
// Since model is identity for this demo, world pos = vertex pos.

static const std::string kRobotVertShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 lightPos;      // xyz = position, w = unused
    vec4 viewPos;       // xyz = position, w = unused
    vec4 lightColor;    // xyz = color, w = unused
} pc;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Normal;
layout(location = 2) out vec4 vColor;

void main()
{
    // Model matrix is identity, so FragPos = aPosition
    FragPos = aPosition;
    Normal = aNormal;
    vColor = aColor;
    gl_Position = pc.viewProj * vec4(aPosition, 1.0);
}
)";

static const std::string kRobotFragShader = R"(
#version 450

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec4 lightPos;
    vec4 viewPos;
    vec4 lightColor;
} pc;

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec4 vColor;

layout(location = 0) out vec4 FragColor;

void main()
{
    vec3 lp = pc.lightPos.xyz;
    vec3 vp = pc.viewPos.xyz;
    vec3 lc = pc.lightColor.xyz;

    // ambient
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lc;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lp - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lc;

    // specular
    float specularStrength = 0.3;
    vec3 viewDir = normalize(vp - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8);
    vec3 specular = specularStrength * spec * lc;

    vec3 result = (ambient + diffuse + specular) * vColor.xyz;
    FragColor = vec4(result, vColor.w);
}
)";

// ── Application ──────────────────────────────────────────────────────────────

Application::Application():
	drawmode_(DrawMode::MODE_MESH),
	shading_toggle_(false),
	shading_mode_changed_(false),
	camera_(Vector3f(1, 0, 0), 4.0f, static_cast<float>(M_PI / 4), static_cast<float>(M_PI / 6))
{
	robot_ = std::make_unique<Robot>("src/resources/params.txt", Vector3f(1, 0, 0));

	// now robot knows how many joints it has, we can create sliders to control them
	joint_angle_controls_.resize(robot_->getNumJoints());
	prev_controls_.resize(robot_->getNumJoints());
}

void Application::createWindow(int width, int height) {
	// Initialize VulkanBase with ImGui enabled
	vkdemo::AppConfig config;
	config.Title = "Robot Arm Simulator (Vulkan)";
	config.Width = width;
	config.Height = height;
	config.EnableValidation = true;
	config.EnableImGui = true;

	vkBase_.Init(config);

	GLFWwindow* window = vkBase_.GetWindow();

	// Setup event handling
	EventDispatcher::SetApplication(this);
	glfwSetKeyCallback(window, EventDispatcher::KeyboardCallback);
	glfwSetCursorPosCallback(window, EventDispatcher::MouseMovedCallback);
	glfwSetMouseButtonCallback(window, EventDispatcher::MouseButtonCallback);
	glfwSetScrollCallback(window, EventDispatcher::MouseScrolledCallback);

	// Style ImGui
	{
		ImGuiStyle* style = &ImGui::GetStyle();

		style->WindowPadding = ImVec2(15, 15);
		style->WindowRounding = 5.0f;
		style->FramePadding = ImVec2(5, 5);
		style->FrameRounding = 4.0f;
		style->ItemSpacing = ImVec2(12, 8);
		style->ItemInnerSpacing = ImVec2(8, 6);
		style->IndentSpacing = 25.0f;
		style->ScrollbarSize = 15.0f;
		style->ScrollbarRounding = 9.0f;
		style->GrabMinSize = 5.0f;
		style->GrabRounding = 3.0f;

		style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
		style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
		style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
		style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
		style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
		style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
		style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
		style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
		style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
		style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
		style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
		style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
		style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
	}

	// Create Vulkan graphics pipeline
	createGraphicsPipeline();

	// Handle swapchain recreation (pipeline depends on render pass)
	vkBase_.OnResize = [this](int w, int h) {
		(void)w; (void)h;
		destroyGraphicsPipeline();
		createGraphicsPipeline();
	};
}

void Application::handleEvent(const Event& ev)
{
	camera_.handleEvent(ev);

	// handle right mouse clicks for IK target
	if (ev.type == EventType::KEY_DOWN && ev.key == GLFW_MOUSE_BUTTON_RIGHT)
	{
		setIkTarget();
		running_ik_solution_ = true;
	}
}

void Application::createGraphicsPipeline()
{
	VkDevice device = vkBase_.GetDevice();

	// Compile shaders from embedded GLSL strings
	auto vertSPIRV = compileGLSLToSPIRV(kRobotVertShader, shaderc_vertex_shader, "robot.vert");
	auto fragSPIRV = compileGLSLToSPIRV(kRobotFragShader, shaderc_fragment_shader, "robot.frag");

	vertModule_ = vkBase_.CreateShaderModule(vertSPIRV);
	fragModule_ = vkBase_.CreateShaderModule(fragSPIRV);

	// Shader stages
	VkPipelineShaderStageCreateInfo vertStage{};
	vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertStage.module = vertModule_;
	vertStage.pName = "main";

	VkPipelineShaderStageCreateInfo fragStage{};
	fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragStage.module = fragModule_;
	fragStage.pName = "main";

	VkPipelineShaderStageCreateInfo stages[] = {vertStage, fragStage};

	// Vertex input: position (vec3), normal (vec3), color (vec4)
	VkVertexInputBindingDescription binding{};
	binding.binding = 0;
	binding.stride = sizeof(Vertex);  // 3+3+4 floats = 40 bytes
	binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::array<VkVertexInputAttributeDescription, 3> attrs{};
	// position
	attrs[0].binding = 0;
	attrs[0].location = 0;
	attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrs[0].offset = offsetof(Vertex, position);
	// normal
	attrs[1].binding = 0;
	attrs[1].location = 1;
	attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attrs[1].offset = offsetof(Vertex, normal);
	// color (vec4)
	attrs[2].binding = 0;
	attrs[2].location = 2;
	attrs[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	attrs[2].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInput{};
	vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInput.vertexBindingDescriptionCount = 1;
	vertexInput.pVertexBindingDescriptions = &binding;
	vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrs.size());
	vertexInput.pVertexAttributeDescriptions = attrs.data();

	// Input assembly
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// Viewport & scissor (dynamic)
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	// Rasterizer (fill mode)
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;  // no culling for robot parts
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;

	// Multisampling
	VkPipelineMultisampleStateCreateInfo multisampling{};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// Depth stencil
	VkPipelineDepthStencilStateCreateInfo depthStencil{};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.stencilTestEnable = VK_FALSE;

	// Color blending (alpha blending enabled)
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
		VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlending{};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	// Dynamic state
	VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	// Push constant range: PushConstantData = 112 bytes
	// (mat4=64 + 3*vec4=48 = 112, well within 128 byte minimum)
	VkPushConstantRange pushRange{};
	pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushRange.offset = 0;
	pushRange.size = sizeof(PushConstantData);

	pipelineLayout_ = vkBase_.CreatePipelineLayout({}, {pushRange});

	// Create fill pipeline
	VkGraphicsPipelineCreateInfo pipelineCI{};
	pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCI.stageCount = 2;
	pipelineCI.pStages = stages;
	pipelineCI.pVertexInputState = &vertexInput;
	pipelineCI.pInputAssemblyState = &inputAssembly;
	pipelineCI.pViewportState = &viewportState;
	pipelineCI.pRasterizationState = &rasterizer;
	pipelineCI.pMultisampleState = &multisampling;
	pipelineCI.pDepthStencilState = &depthStencil;
	pipelineCI.pColorBlendState = &colorBlending;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.layout = pipelineLayout_;
	pipelineCI.renderPass = vkBase_.GetRenderPass();
	pipelineCI.subpass = 0;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &pipeline_) != VK_SUCCESS)
		throw std::runtime_error("Failed to create fill graphics pipeline");

	// Create wireframe pipeline variant
	VkPipelineRasterizationStateCreateInfo wireRasterizer = rasterizer;
	wireRasterizer.polygonMode = VK_POLYGON_MODE_LINE;
	wireRasterizer.lineWidth = 1.0f;
	pipelineCI.pRasterizationState = &wireRasterizer;

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &wireframePipeline_) != VK_SUCCESS)
		throw std::runtime_error("Failed to create wireframe graphics pipeline");
}

void Application::destroyGraphicsPipeline()
{
	VkDevice device = vkBase_.GetDevice();
	vkDeviceWaitIdle(device);

	if (pipeline_ != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, pipeline_, nullptr);
		pipeline_ = VK_NULL_HANDLE;
	}
	if (wireframePipeline_ != VK_NULL_HANDLE) {
		vkDestroyPipeline(device, wireframePipeline_, nullptr);
		wireframePipeline_ = VK_NULL_HANDLE;
	}
	if (pipelineLayout_ != VK_NULL_HANDLE) {
		vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
		pipelineLayout_ = VK_NULL_HANDLE;
	}
	if (vertModule_ != VK_NULL_HANDLE) {
		vkDestroyShaderModule(device, vertModule_, nullptr);
		vertModule_ = VK_NULL_HANDLE;
	}
	if (fragModule_ != VK_NULL_HANDLE) {
		vkDestroyShaderModule(device, fragModule_, nullptr);
		fragModule_ = VK_NULL_HANDLE;
	}
}

void Application::uploadVertexData(const std::vector<Vertex>& vertices)
{
	vertexCount_ = static_cast<uint32_t>(vertices.size());
	if (vertexCount_ == 0) return;

	VkDeviceSize bufferSize = sizeof(Vertex) * vertexCount_;

	// Destroy old buffer if it exists and is too small
	if (vertexBuffer_.Buffer != VK_NULL_HANDLE && vertexBuffer_.Size < bufferSize) {
		// Need to wait for GPU to finish using the old buffer
		vkDeviceWaitIdle(vkBase_.GetDevice());
		vkBase_.DestroyBuffer(vertexBuffer_);
		vertexBuffer_ = {};
	}

	if (vertexBuffer_.Buffer == VK_NULL_HANDLE) {
		// Create host-visible vertex buffer (re-uploaded each frame)
		vertexBuffer_ = vkBase_.CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}

	vkBase_.CopyToBuffer(vertexBuffer_, vertices.data(), bufferSize);
}

void Application::run(void) {
	std::vector<DhParam> robot_params = robot_->getDhParams();

	float last_time = static_cast<float>(glfwGetTime());
	float curr_time;

	// program loop
	while (true)
	{
		if (!vkBase_.BeginFrame())
			break;

		// calculate time delta
		curr_time = static_cast<float>(glfwGetTime());
		float dt = curr_time - last_time;
		last_time = curr_time;

		// ============= ImGui  ================
		{
			using namespace ImGui;
			vkBase_.ImGuiNewFrame();
			ImGuizmo::BeginFrame();

			// ImGuizmo
			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());

			int screenWidth = vkBase_.GetWidth();
			int screenHeight = vkBase_.GetHeight();

			ImGuizmo::SetRect(0, 0, static_cast<float>(screenWidth), static_cast<float>(screenHeight));
			Eigen::Matrix4f robot_transform = robot_->getWorldToBase().matrix();
			Eigen::Matrix4f camera_view = camera_.getViewMatrix();
			Eigen::Matrix4f camera_projection = camera_.getProjectionMatrix();

			ImGuizmo::Manipulate(
				camera_view.data(),
				camera_projection.data(),
				ImGuizmo::OPERATION::TRANSLATE,
				ImGuizmo::LOCAL,
				robot_transform.data()
			);

			if (ImGuizmo::IsUsing())
			{
				robot_->setWorldToBase(Eigen::Affine3f(robot_transform));
			}

			Begin("Joint angle controls:");
			for (int i = 0; i < static_cast<int>(robot_->getNumJoints()); i++) {
				PushID(i);
				Text("joint %d", i);
				SameLine();
				SliderAngle("", &joint_angle_controls_[i]);
				// user manually grabbing a slider aborts the running IK solution
				if (ImGui::IsItemActive() && running_ik_solution_)
				{
					abortRunningIkSolution();
				}
				PopID();
			}
			End();

			Begin("Joint PID controller gains:");
			SliderFloat("P", &pid_p, 0, 0.1f);
			SliderFloat("I", &pid_i, 0, 0.05f);
			SliderFloat("D", &pid_d, 0, 0.05f);
			robot_->setJointControllerPidGains(pid_p, pid_i, pid_d);
			End();

			Begin("TCP info");
			Vector3f tcp_pos = robot_->getTcpWorldPosition();
			Vector3f tcp_target = robot_->getTargetTcpPosition();
			VectorXf tcp_speed = robot_->getTcpSpeed();
			Columns(2);
			Text("TCP position:");
			NextColumn();
			Text("x: % 2.2f, y: % 2.2f, z: % 2.2f", tcp_pos.x(), tcp_pos.y(), tcp_pos.z());
			Separator();
			NextColumn();
			Text("TCP target:");
			NextColumn();
			Text("x: % 2.2f, y: % 2.2f, z: % 2.2f", tcp_target.x(), tcp_target.y(), tcp_target.z());
			Separator();
			NextColumn();
			Text("TCP speed:");
			NextColumn();
			Text("x: % 2.2f, y: % 2.2f, z: % 2.2f\nrx: % 2.2f, ry: % 2.2f, rz: % 2.2f",
				tcp_speed[0], tcp_speed[1], tcp_speed[2], tcp_speed[3], tcp_speed[4], tcp_speed[5]);
			End();

			Begin("Parameter editor");
			if (BeginTable("paramtable", static_cast<int>(robot_->getNumJoints()) + 1)) {
				// header row
				TableHeadersRow();
				TableNextColumn();
				Text("");
				TableNextColumn();
				Text("a");
				TableNextColumn();
				Text("d");
				TableNextColumn();
				Text("alpha");

				// table content rows
				for (int i = 0; i < static_cast<int>(robot_->getNumJoints()); i++) {
					PushID(i);
					TableNextRow();
					TableNextColumn();
					Text("joint %d", i);
					TableNextColumn();
					InputFloat("##a", &robot_params[i].a);
					TableNextColumn();
					InputFloat("##b", &robot_params[i].d);
					TableNextColumn();
					InputFloat("##c", &robot_params[i].alpha);
					PopID();
				}
				EndTable();
			}
			if (Button("Apply")) {
				robot_->setDhParams(robot_params);
			}
			End();

			Begin("Performance");
			Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / GetIO().Framerate, GetIO().Framerate);
			End();
		}

		update(dt);

		// Render
		VkCommandBuffer cmd = vkBase_.GetCurrentCommandBuffer();
		render(cmd);

		// Render ImGui
		vkBase_.ImGuiRender(cmd);

		vkBase_.EndFrame();
	}

	// Wait for device to be idle before cleanup
	vkDeviceWaitIdle(vkBase_.GetDevice());

	// Cleanup Vulkan resources
	if (vertexBuffer_.Buffer != VK_NULL_HANDLE) {
		vkBase_.DestroyBuffer(vertexBuffer_);
	}
	destroyGraphicsPipeline();

	vkBase_.Shutdown();
}

void Application::update(float dt)
{
	if (running_ik_solution_)
	{
		updateJointControlSliders();
		// check if the robot reached the IK target already
		bool finished = (robot_->getJointAngles() - robot_->getTargetJointAngles()).norm() < 0.01f;
		if (finished)
		{
			running_ik_solution_ = false;
		}
	}
	else
	{
		applyJointControls();
	}

	camera_.update(dt, vkBase_.GetWindow());
	robot_->update(dt);
}

void Application::setIkTarget() {
	GLFWwindow* window = vkBase_.GetWindow();
	// convert mouse pos to normalized image coordinates
	Vector2d mouse_pos;
	int window_width, window_height;
	glfwGetCursorPos(window, &mouse_pos.x(), &mouse_pos.y());
	glfwGetFramebufferSize(window, &window_width, &window_height);
	float x = static_cast<float>(mouse_pos.x()) / window_width;
	float y = static_cast<float>(mouse_pos.y()) / window_height;
	x = x * 2.0f - 1.0f;
	y = 1.0f - (2.0f * y);

	Vector4f ray_clip = {x, y, -1.0f, 1.0f};
	Vector4f ray_eye = camera_.getProjectionMatrix().inverse() * ray_clip;
	ray_eye.z() = -1.0f;
	ray_eye.w() = 0.0f;

	Vector3f ray_world = (camera_.getViewMatrix().inverse() * ray_eye).head<3>();
	ray_world.normalize();

	Vector3f camera_position = camera_.getPosition();

	// ray should be going from [camera_position] towards [ray_world]
	float t = 2;
	ray_start_ = camera_position;
	ray_end_ = camera_position + ray_world * t;

	robot_->setTargetTcpPosition(ray_end_);
}

void Application::abortRunningIkSolution() {
	// set robot target to its current angles
	robot_->setJointTargetAngles(robot_->getJointAngles());
	// return joint control to the sliders
	running_ik_solution_ = false;
}

void Application::updateJointControlSliders() {
	// set the joint slider values to actual joint angles
	for (size_t i = 0; i < joint_angle_controls_.size(); i++) {
		joint_angle_controls_[i] = robot_->getJointAngle(static_cast<unsigned>(i));
	}
}

void Application::applyJointControls() {
	// set robot joint rotations to slider values
	for (size_t i = 0; i < joint_angle_controls_.size(); i++) {
		float curr = joint_angle_controls_[i];
		float prev = prev_controls_[i];
		if (std::abs(curr - prev) > 0.0001f) {
			robot_->setJointTargetAngle(static_cast<unsigned>(i), joint_angle_controls_[i]);
			prev_controls_[i] = curr;
		}
	}
}

void Application::render(VkCommandBuffer cmd) {
	// Adjust aspect ratio
	int screenWidth = vkBase_.GetWidth();
	int screenHeight = vkBase_.GetHeight();
	float fAspect = static_cast<float>(screenWidth) / static_cast<float>(screenHeight);
	camera_.setAspectRatio(fAspect);

	// Set up transform matrices
	Matrix4f C = camera_.getViewMatrix();
	Matrix4f P = camera_.getProjectionMatrix();

	// Gather all vertices
	std::vector<Vertex> vertices = robot_->getMeshVertices();

	temp += 0.0001f;
	Vector3f lightpos = Vector3f(6.0f * std::sin(temp), 3.5f, 6.0f * std::cos(temp));
	BoxMesh lightmesh = BoxMesh(0.2f, 0.2f, 0.2f, Vector4f(1, 1, 1, 1));
	lightmesh.transform(Translation3f(lightpos) * AngleAxisf::Identity());
	std::vector<Vertex> lightverts = lightmesh.getVertices();
	vertices.insert(vertices.end(), lightverts.begin(), lightverts.end());

	SphereMesh ikmesh = SphereMesh(0.01f, Vector4f(1, 0.2f, 0.2f, 1.0f));
	ikmesh.transform(Translation3f(ray_end_) * AngleAxisf::Identity());
	std::vector<Vertex> ikverts = ikmesh.getVertices();
	vertices.insert(vertices.end(), ikverts.begin(), ikverts.end());

	float square_size = 0.2f;
	float grid_size = 50;

	std::vector<Vertex> planeVerts;

	for (int i = 0; i < static_cast<int>(grid_size); i++) {
		for (int j = 0; j < static_cast<int>(grid_size); j++) {
			Vector4f color = (i + j) % 2 == 0 ? Vector4f(0.8f, 0.8f, 0.8f, 1.0f) : Vector4f(0.3f, 0.3f, 0.3f, 1.0f);

			// left triangle
			Vertex p1, p2, p3;
			p1.position = Vector3f(i * square_size, 0, j * square_size);
			p2.position = Vector3f(i * square_size, 0, (j + 1) * square_size);
			p3.position = Vector3f((i + 1) * square_size, 0, j * square_size);

			p1.normal = p2.normal = p3.normal = Vector3f(0, 1, 0);
			p1.color = p2.color = p3.color = color;
			planeVerts.insert(planeVerts.end(), { p1,p2,p3 });

			// right triangle
			p1.position = Vector3f(i * square_size, 0, (j + 1) * square_size);
			p2.position = Vector3f((i + 1) * square_size, 0, (j + 1) * square_size);
			p3.position = Vector3f((i + 1) * square_size, 0, j * square_size);

			p1.normal = p2.normal = p3.normal = Vector3f(0, 1, 0);
			p1.color = p2.color = p3.color = color;
			planeVerts.insert(planeVerts.end(), { p1,p2,p3 });
		}
	}
	for (Vertex& v : planeVerts) {
		v.position -= Vector3f(grid_size * square_size / 2, 0, grid_size * square_size / 2);
	}
	vertices.insert(vertices.end(), planeVerts.begin(), planeVerts.end());

	SphereMesh sphere(0.2f, { 0.0f, 0.5f, 0.0f, 0.5f});
	Affine3f t = AngleAxisf::Identity() * Translation3f(0, 0.2f, 0);
	sphere.setToWorldTransform(t);
	std::vector<Vertex> sphere_verts = sphere.getVertices();
	vertices.insert(vertices.end(), sphere_verts.begin(), sphere_verts.end());

	// Upload vertex data
	uploadVertexData(vertices);

	if (vertexCount_ == 0) return;

	// Select pipeline based on draw mode
	VkPipeline activePipeline = (drawmode_ == DrawMode::MODE_MESH_WIREFRAME) ?
		wireframePipeline_ : pipeline_;

	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, activePipeline);

	// Bind vertex buffer
	VkBuffer vertexBuffers[] = {vertexBuffer_.Buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

	// Set up push constants
	// viewProj = projection * view (model is identity)
	Matrix4f viewProj = P * C;

	PushConstantData pc{};
	pc.viewProj = viewProj;
	pc.lightPosX = lightpos.x();
	pc.lightPosY = lightpos.y();
	pc.lightPosZ = lightpos.z();
	pc._pad0 = 0.0f;
	pc.viewPosX = camera_.getPosition().x();
	pc.viewPosY = camera_.getPosition().y();
	pc.viewPosZ = camera_.getPosition().z();
	pc._pad1 = 0.0f;
	pc.lightColorR = 1.0f;
	pc.lightColorG = 1.0f;
	pc.lightColorB = 1.0f;
	pc._pad2 = 0.0f;

	vkCmdPushConstants(cmd, pipelineLayout_,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0, sizeof(PushConstantData), &pc);

	// Draw all vertices
	vkCmdDraw(cmd, vertexCount_, 1, 0, 0);
}
