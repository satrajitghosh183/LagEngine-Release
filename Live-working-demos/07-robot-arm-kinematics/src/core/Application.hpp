#pragma once

#include <vector>
#include <memory>

#include "Eigen/Dense"
#include "Robot.hpp"
#include "rendering/Camera.hpp"
#include "rendering/Shader.hpp"
#include "Event.hpp"

#include "VulkanBase.hpp"


class Application
{
private:
	enum class DrawMode
	{
		MODE_MESH,
		MODE_MESH_WIREFRAME
	};

	// Push constant structure: 128 bytes exactly (minimum guaranteed by Vulkan spec)
	// Layout: mat4 viewProj (64) + vec4 lightPos (16) + vec4 viewPos (16) + vec4 lightColor (16) = 112
	// Plus pad to 128 for alignment.
	struct PushConstantData
	{
		// 64 bytes
		Eigen::Matrix4f viewProj;
		// 16 bytes (vec3 + padding)
		float lightPosX, lightPosY, lightPosZ, _pad0;
		// 16 bytes (vec3 + padding)
		float viewPosX, viewPosY, viewPosZ, _pad1;
		// 16 bytes (vec3 + padding)
		float lightColorR, lightColorG, lightColorB, _pad2;
	};

public:
	Application();
	virtual         ~Application(void) {}

	void			createWindow(int width, int height);
	void			run(void);
	void			render(VkCommandBuffer cmd);
	void			update(float dt);
	void			applyJointControls();
	void			updateJointControlSliders();
	void			abortRunningIkSolution();
	void			setIkTarget();

	void			handleEvent(const Event& ev);

private:
	Application(const Application&); // forbid copy
	Application& operator=       (const Application&); // forbid assignment

	// Vulkan pipeline setup
	void			createGraphicsPipeline();
	void			destroyGraphicsPipeline();
	void			uploadVertexData(const std::vector<Vertex>& vertices);

private:
	Camera				camera_;
	DrawMode			drawmode_;
	bool				shading_toggle_;
	bool				shading_mode_changed_;
	std::vector<float>  joint_angle_controls_;
	std::vector<float>	prev_controls_;

	uint64_t time_end_;
	float temp = 0;
	bool running_ik_solution_ = false;

	float pid_p = 0.035f;
	float pid_i = 0;
	float pid_d = 0;

	Eigen::Vector3f ray_start_, ray_end_;

	std::unique_ptr<Robot>  robot_;

	// Vulkan resources
	vkdemo::VulkanBase      vkBase_;
	VkPipeline              pipeline_ = VK_NULL_HANDLE;
	VkPipeline              wireframePipeline_ = VK_NULL_HANDLE;
	VkPipelineLayout        pipelineLayout_ = VK_NULL_HANDLE;
	VkShaderModule          vertModule_ = VK_NULL_HANDLE;
	VkShaderModule          fragModule_ = VK_NULL_HANDLE;

	// Dynamic vertex buffer (re-uploaded each frame)
	vkdemo::GPUBuffer       vertexBuffer_;
	uint32_t                vertexCount_ = 0;
};
