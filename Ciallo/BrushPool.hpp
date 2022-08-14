#pragma once
#include "ArticulatedLine.hpp"
#include "Image.hpp"
#include "Device.hpp"
#include "imgui.h"
#include "CommonBrushComponents.hpp"

namespace ciallo::editor
{
	class BrushPool
	{
		entt::registry m_registry;
		std::vector<entt::entity> m_brushes = {};

	public:
		BrushPool() = default;


		void loadPresetBrushes(const vulkan::Device* device)
		{
			entt::entity brush = m_registry.create();
			m_registry.emplace<rendering::EngineType>(brush, rendering::EngineType::ArticulatedLine);

			for (int i : views::iota(0, 10))
			{
				m_brushes.push_back(m_registry.create());
			}

			VmaAllocationCreateInfo allocationCreateInfo{{}, VMA_MEMORY_USAGE_AUTO};
			editor::brush::DemoCpo demo{
				vulkan::Image(*device, allocationCreateInfo, vk::Format::eR8G8B8A8Unorm, 400, 100,
				              vk::SampleCountFlagBits::e1,
				              vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment)
			};

			m_registry.insert<editor::brush::DemoCpo>(m_brushes.begin(), m_brushes.end(), demo);
		}
	};
}
