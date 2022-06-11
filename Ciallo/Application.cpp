﻿#include "pch.hpp"
#include "Application.hpp"
#include "vulkanTest.hpp"
#include "MainPassRenderer.hpp"

void ciallo::Application::run()
{
	auto w = std::make_unique<vulkan::Window>(1000, 1000, "hi");
	auto inst = std::make_shared<vulkan::Instance>();
	uint32_t extensionsCount;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensionsCount);
	std::vector<const char*> glfwExtensions{extensions, extensions+extensionsCount};
	inst->addInstanceExtensions(glfwExtensions);
	inst->create();
	w->setInstance(inst);
	w->initResources();

	vulkan::Test t(w.get());
}

void ciallo::Application::loadSettings()
{
	m_mainWindowWidth = 400;
	m_mainWindowHeight = 400;
}
