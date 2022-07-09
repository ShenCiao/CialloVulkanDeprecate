﻿#include "pch.hpp"
#include "ScenePanel.hpp"

#include <filesystem>
#include <imgui_impl_vulkan.h>
#include <stb_image.h>

#include "Device.hpp"

namespace ciallo::gui
{
	void ScenePanel::draw() const
	{
		ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar;
		ImGui::Begin("Scene", nullptr, flags);
		// ImGui::Image()
		ImVec2 sizeToShow{static_cast<float>(m_canvas->width()), static_cast<float>(m_canvas->height())};
		ImGui::Image(m_canvasTextureId, sizeToShow);
		ImGui::End();
	}

	void ScenePanel::genCanvas(vulkan::Device* d, vk::CommandBuffer cb)
	{
		int width, height, channels;
		spdlog::info("Current working directory: {}", std::filesystem::current_path().string());

		auto data = stbi_load("images/takagi3.png", &width, &height, &channels,
		                      STBI_rgb_alpha);
		if (!data)
		{
			throw std::runtime_error("Unable to load image");
		}

		VmaAllocationCreateInfo info = {};
		info.usage = VMA_MEMORY_USAGE_AUTO;

		m_canvas = std::make_unique<vulkan::Image>(*d, info, width, height,
		                                           vk::ImageUsageFlagBits::eSampled |
		                                           vk::ImageUsageFlagBits::eTransferDst|
		                                           vk::ImageUsageFlagBits::eColorAttachment);
		if (m_canvas->hostVisible())
		{
			spdlog::info("it's host visible");
		}
		auto barrier = m_canvas->createLayoutTransitionMemoryBarrier(vk::ImageLayout::eGeneral);

		cb.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
		                   barrier);
		m_canvas->setImageLayout(vk::ImageLayout::eGeneral);
		m_canvas->upload(cb, data);
		m_canvasTextureId = ImGui_ImplVulkan_AddTexture(*m_sampler, m_canvas->imageView(),
		                                                static_cast<VkImageLayout>(vk::ImageLayout::eGeneral));
		stbi_image_free(data);

		cb.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands, vk::PipelineStageFlagBits::eAllCommands, {}, {}, {},
		                   {});

		m_triangleTest = std::make_unique<rendering::TriangleTest>(d, m_canvas.get());
		m_triangleTest->renderDynamic(cb);
		// m_canvasRenderer = std::make_unique<rendering::CanvasRenderer>(d);
		// m_canvasRenderer->setTarget(m_canvas.get());
		// m_canvasRenderer->render(cb);
	}

	void ScenePanel::genSampler(vk::Device device)
	{
		vk::SamplerCreateInfo info{};
		info.magFilter = vk::Filter::eNearest;
		info.minFilter = vk::Filter::eNearest;
		info.addressModeU = vk::SamplerAddressMode::eClampToBorder;
		info.addressModeV = vk::SamplerAddressMode::eClampToBorder;
		info.addressModeW = vk::SamplerAddressMode::eClampToBorder;
		info.anisotropyEnable = VK_FALSE;
		info.borderColor = vk::BorderColor::eIntOpaqueBlack;
		info.unnormalizedCoordinates = VK_FALSE;
		info.compareEnable = VK_FALSE;
		info.compareOp = vk::CompareOp::eAlways;
		m_sampler = device.createSamplerUnique(info);
	}
}
