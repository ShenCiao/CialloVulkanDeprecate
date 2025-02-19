// -----------------------------------------------------------------------------
// Modified version by Ciallo:
//
// - Renamed "Renderpass" to "RenderPass".
// - Create pipeline and compute pipeline with vk::ShaderModule instead of vku::ShaderModule
// - Add support for dynamic rendering.
// - Add default value for weight and height in pipeline maker
// - Delete class Image, Buffer, ShaderModule, Device, Instance. We use our own.
// - Clean up unused #include and reformat whole code.
// - Add default offset and range to DescriptorUpdater::buffer.
// - Change pipeline blendEnable() same as blendBegin().
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
//
/// Vookoo high level C++ Vulkan interface.
//
/// (C) Vookoo Contributors, MIT License
//
/// This is a utility set alongside the vkcpp C++ interface to Vulkan which makes
/// constructing Vulkan pipelines and resources very easy for beginners.
//
/// It is expected that once familar with the Vulkan C++ interface you may wish
/// to "go it alone" but we hope that this will make the learning experience a joyful one.
//
/// You can use it with the demo framework, stand alone and mixed with C or C++ Vulkan objects.
/// It should integrate with game engines nicely.
//

#ifndef VKU_HPP
#define VKU_HPP

#include <utility>
#include <array>
#include <fstream>
#include <vector>
#include <functional>
#include <cstddef>

#ifdef VOOKOO_SPIRV_SUPPORT
  #include <unified1/spirv.hpp11>
#endif

#include <vulkan/vulkan.hpp>

namespace vku
{
	/// Printf-style formatting function.
	template <class ... Args>
	std::string format(const char* fmt, Args ... args)
	{
		int n = snprintf(nullptr, 0, fmt, args...);
		std::string result(n, '\0');
		snprintf(&*result.begin(), n + 1, fmt, args...);
		return result;
	}

	/// Utility function for finding memory types for uniforms and images.
	inline int findMemoryTypeIndex(const vk::PhysicalDeviceMemoryProperties& memprops, uint32_t memoryTypeBits,
	                               vk::MemoryPropertyFlags search)
	{
		for (int i = 0; i != memprops.memoryTypeCount; ++i, memoryTypeBits >>= 1)
		{
			if (memoryTypeBits & 1)
			{
				if ((memprops.memoryTypes[i].propertyFlags & search) == search)
				{
					return i;
				}
			}
		}
		return -1;
	}

	/// Scale a value by mip level, but do not reduce to zero.
	inline uint32_t mipScale(uint32_t value, uint32_t mipLevel)
	{
		return std::max(value >> mipLevel, (uint32_t)1);
	}

	/// Load a binary file into a vector.
	/// The vector will be zero-length if this fails.
	inline std::vector<uint8_t> loadFile(const std::string& filename)
	{
		std::ifstream is(filename, std::ios::binary | std::ios::ate);
		std::vector<uint8_t> bytes;
		if (!is.fail())
		{
			size_t size = is.tellg();
			is.seekg(0);
			bytes.resize(size);
			is.read((char*)bytes.data(), size);
		}

		return bytes;
	}
	
	/// Description of blocks for compressed formats.
	struct BlockParams
	{
		uint8_t blockWidth;
		uint8_t blockHeight;
		uint8_t bytesPerBlock;
	};

	/// Get the details of vulkan texture formats.
	inline BlockParams getBlockParams(vk::Format format)
	{
		switch (format)
		{
		case vk::Format::eR4G4UnormPack8: return BlockParams{1, 1, 1};
		case vk::Format::eR4G4B4A4UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eB4G4R4A4UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eR5G6B5UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eB5G6R5UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eR5G5B5A1UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eB5G5R5A1UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eA1R5G5B5UnormPack16: return BlockParams{1, 1, 2};
		case vk::Format::eR8Unorm: return BlockParams{1, 1, 1};
		case vk::Format::eR8Snorm: return BlockParams{1, 1, 1};
		case vk::Format::eR8Uscaled: return BlockParams{1, 1, 1};
		case vk::Format::eR8Sscaled: return BlockParams{1, 1, 1};
		case vk::Format::eR8Uint: return BlockParams{1, 1, 1};
		case vk::Format::eR8Sint: return BlockParams{1, 1, 1};
		case vk::Format::eR8Srgb: return BlockParams{1, 1, 1};
		case vk::Format::eR8G8Unorm: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Snorm: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Uscaled: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Sscaled: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Uint: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Sint: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8Srgb: return BlockParams{1, 1, 2};
		case vk::Format::eR8G8B8Unorm: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Snorm: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Uscaled: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Sscaled: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Uint: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Sint: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8Srgb: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Unorm: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Snorm: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Uscaled: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Sscaled: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Uint: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Sint: return BlockParams{1, 1, 3};
		case vk::Format::eB8G8R8Srgb: return BlockParams{1, 1, 3};
		case vk::Format::eR8G8B8A8Unorm: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Snorm: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Uscaled: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Sscaled: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Uint: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Sint: return BlockParams{1, 1, 4};
		case vk::Format::eR8G8B8A8Srgb: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Unorm: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Snorm: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Uscaled: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Sscaled: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Uint: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Sint: return BlockParams{1, 1, 4};
		case vk::Format::eB8G8R8A8Srgb: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8UnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8SnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8UscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8SscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8UintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8SintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA8B8G8R8SrgbPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10UnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10SnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10UscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10SscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10UintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2R10G10B10SintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10UnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10SnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10UscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10SscaledPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10UintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eA2B10G10R10SintPack32: return BlockParams{1, 1, 4};
		case vk::Format::eR16Unorm: return BlockParams{1, 1, 2};
		case vk::Format::eR16Snorm: return BlockParams{1, 1, 2};
		case vk::Format::eR16Uscaled: return BlockParams{1, 1, 2};
		case vk::Format::eR16Sscaled: return BlockParams{1, 1, 2};
		case vk::Format::eR16Uint: return BlockParams{1, 1, 2};
		case vk::Format::eR16Sint: return BlockParams{1, 1, 2};
		case vk::Format::eR16Sfloat: return BlockParams{1, 1, 2};
		case vk::Format::eR16G16Unorm: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Snorm: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Uscaled: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Sscaled: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Uint: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Sint: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16Sfloat: return BlockParams{1, 1, 4};
		case vk::Format::eR16G16B16Unorm: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Snorm: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Uscaled: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Sscaled: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Uint: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Sint: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16Sfloat: return BlockParams{1, 1, 6};
		case vk::Format::eR16G16B16A16Unorm: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Snorm: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Uscaled: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Sscaled: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Uint: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Sint: return BlockParams{1, 1, 8};
		case vk::Format::eR16G16B16A16Sfloat: return BlockParams{1, 1, 8};
		case vk::Format::eR32Uint: return BlockParams{1, 1, 4};
		case vk::Format::eR32Sint: return BlockParams{1, 1, 4};
		case vk::Format::eR32Sfloat: return BlockParams{1, 1, 4};
		case vk::Format::eR32G32Uint: return BlockParams{1, 1, 8};
		case vk::Format::eR32G32Sint: return BlockParams{1, 1, 8};
		case vk::Format::eR32G32Sfloat: return BlockParams{1, 1, 8};
		case vk::Format::eR32G32B32Uint: return BlockParams{1, 1, 12};
		case vk::Format::eR32G32B32Sint: return BlockParams{1, 1, 12};
		case vk::Format::eR32G32B32Sfloat: return BlockParams{1, 1, 12};
		case vk::Format::eR32G32B32A32Uint: return BlockParams{1, 1, 16};
		case vk::Format::eR32G32B32A32Sint: return BlockParams{1, 1, 16};
		case vk::Format::eR32G32B32A32Sfloat: return BlockParams{1, 1, 16};
		case vk::Format::eR64Uint: return BlockParams{1, 1, 8};
		case vk::Format::eR64Sint: return BlockParams{1, 1, 8};
		case vk::Format::eR64Sfloat: return BlockParams{1, 1, 8};
		case vk::Format::eR64G64Uint: return BlockParams{1, 1, 16};
		case vk::Format::eR64G64Sint: return BlockParams{1, 1, 16};
		case vk::Format::eR64G64Sfloat: return BlockParams{1, 1, 16};
		case vk::Format::eR64G64B64Uint: return BlockParams{1, 1, 24};
		case vk::Format::eR64G64B64Sint: return BlockParams{1, 1, 24};
		case vk::Format::eR64G64B64Sfloat: return BlockParams{1, 1, 24};
		case vk::Format::eR64G64B64A64Uint: return BlockParams{1, 1, 32};
		case vk::Format::eR64G64B64A64Sint: return BlockParams{1, 1, 32};
		case vk::Format::eR64G64B64A64Sfloat: return BlockParams{1, 1, 32};
		case vk::Format::eB10G11R11UfloatPack32: return BlockParams{1, 1, 4};
		case vk::Format::eE5B9G9R9UfloatPack32: return BlockParams{1, 1, 4};
		case vk::Format::eD16Unorm: return BlockParams{1, 1, 4};
		case vk::Format::eX8D24UnormPack32: return BlockParams{1, 1, 4};
		case vk::Format::eD32Sfloat: return BlockParams{1, 1, 4};
		case vk::Format::eS8Uint: return BlockParams{1, 1, 1};
		case vk::Format::eD16UnormS8Uint: return BlockParams{1, 1, 3};
		case vk::Format::eD24UnormS8Uint: return BlockParams{1, 1, 4};
		case vk::Format::eD32SfloatS8Uint: return BlockParams{0, 0, 0};
		case vk::Format::eBc1RgbUnormBlock: return BlockParams{4, 4, 8};
		case vk::Format::eBc1RgbSrgbBlock: return BlockParams{4, 4, 8};
		case vk::Format::eBc1RgbaUnormBlock: return BlockParams{4, 4, 8};
		case vk::Format::eBc1RgbaSrgbBlock: return BlockParams{4, 4, 8};
		case vk::Format::eBc2UnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc2SrgbBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc3UnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc3SrgbBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc4UnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc4SnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc5UnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc5SnormBlock: return BlockParams{4, 4, 16};
		case vk::Format::eBc6HUfloatBlock: return BlockParams{0, 0, 0};
		case vk::Format::eBc6HSfloatBlock: return BlockParams{0, 0, 0};
		case vk::Format::eBc7UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eBc7SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8A1UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8A1SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8A8UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEtc2R8G8B8A8SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEacR11UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEacR11SnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEacR11G11UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eEacR11G11SnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc4x4UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc4x4SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc5x4UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc5x4SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc5x5UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc5x5SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc6x5UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc6x5SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc6x6UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc6x6SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x5UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x5SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x6UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x6SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x8UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc8x8SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x5UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x5SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x6UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x6SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x8UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x8SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x10UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc10x10SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc12x10UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc12x10SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc12x12UnormBlock: return BlockParams{0, 0, 0};
		case vk::Format::eAstc12x12SrgbBlock: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc12BppUnormBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc14BppUnormBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc22BppUnormBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc24BppUnormBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc12BppSrgbBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc14BppSrgbBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc22BppSrgbBlockIMG: return BlockParams{0, 0, 0};
		case vk::Format::ePvrtc24BppSrgbBlockIMG: return BlockParams{0, 0, 0};
		}
		return BlockParams{0, 0, 0};
	}

	/// Factory for renderpasses.
	/// example:
	///     RenderPassMaker rpm;
	///     rpm.subpassBegin(vk::PipelineBindPoint::eGraphics);
	///     rpm.subpassColorAttachment(vk::ImageLayout::eColorAttachmentOptimal);
	///
	///     rpm.attachmentDescription(attachmentDesc);
	///     rpm.subpassDependency(dependency);
	///     s.renderPass_ = rpm.createUnique(device);
	class RenderPassMaker
	{
	public:
		RenderPassMaker()
		{
		}

		/// Begin an attachment description.
		/// After this you can call attachment* many times
		RenderPassMaker& attachmentBegin(vk::Format format)
		{
			vk::AttachmentDescription desc{{}, format};
			s.attachmentDescriptions.push_back(desc);
			return *this;
		}

		RenderPassMaker& attachmentFlags(vk::AttachmentDescriptionFlags value)
		{
			s.attachmentDescriptions.back().flags = value;
			return *this;
		};

		RenderPassMaker& attachmentFormat(vk::Format value)
		{
			s.attachmentDescriptions.back().format = value;
			return *this;
		};

		RenderPassMaker& attachmentSamples(vk::SampleCountFlagBits value)
		{
			s.attachmentDescriptions.back().samples = value;
			return *this;
		};

		RenderPassMaker& attachmentLoadOp(vk::AttachmentLoadOp value)
		{
			s.attachmentDescriptions.back().loadOp = value;
			return *this;
		};

		RenderPassMaker& attachmentStoreOp(vk::AttachmentStoreOp value)
		{
			s.attachmentDescriptions.back().storeOp = value;
			return *this;
		};

		RenderPassMaker& attachmentStencilLoadOp(vk::AttachmentLoadOp value)
		{
			s.attachmentDescriptions.back().stencilLoadOp = value;
			return *this;
		};

		RenderPassMaker& attachmentStencilStoreOp(vk::AttachmentStoreOp value)
		{
			s.attachmentDescriptions.back().stencilStoreOp = value;
			return *this;
		};

		RenderPassMaker& attachmentInitialLayout(vk::ImageLayout value)
		{
			s.attachmentDescriptions.back().initialLayout = value;
			return *this;
		};

		RenderPassMaker& attachmentFinalLayout(vk::ImageLayout value)
		{
			s.attachmentDescriptions.back().finalLayout = value;
			return *this;
		};

		/// Start a subpass description.
		/// After this you can can call subpassColorAttachment many times
		/// and subpassDepthStencilAttachment once.
		RenderPassMaker& subpassBegin(vk::PipelineBindPoint bp)
		{
			vk::SubpassDescription desc{};
			desc.pipelineBindPoint = bp;
			s.subpassDescriptions.push_back(desc);
			return *this;
		}

		RenderPassMaker& subpassColorAttachment(vk::ImageLayout layout, uint32_t attachment)
		{
			vk::SubpassDescription& subpass = s.subpassDescriptions.back();
			auto* p = getAttachmentReference();
			p->layout = layout;
			p->attachment = attachment;
			if (subpass.colorAttachmentCount == 0)
			{
				subpass.pColorAttachments = p;
			}
			subpass.colorAttachmentCount++;
			return *this;
		}

		RenderPassMaker& subpassDepthStencilAttachment(vk::ImageLayout layout, uint32_t attachment)
		{
			vk::SubpassDescription& subpass = s.subpassDescriptions.back();
			auto* p = getAttachmentReference();
			p->layout = layout;
			p->attachment = attachment;
			subpass.pDepthStencilAttachment = p;
			return *this;
		}

		vk::UniqueRenderPass createUnique(const vk::Device& device) const
		{
			vk::RenderPassCreateInfo renderPassInfo{};
			renderPassInfo.attachmentCount = (uint32_t)s.attachmentDescriptions.size();
			renderPassInfo.pAttachments = s.attachmentDescriptions.data();
			renderPassInfo.subpassCount = (uint32_t)s.subpassDescriptions.size();
			renderPassInfo.pSubpasses = s.subpassDescriptions.data();
			renderPassInfo.dependencyCount = (uint32_t)s.subpassDependencies.size();
			renderPassInfo.pDependencies = s.subpassDependencies.data();
			return device.createRenderPassUnique(renderPassInfo);
		}

		vk::UniqueRenderPass createUnique(const vk::Device& device, const vk::RenderPassMultiviewCreateInfo& I) const
		{
			vk::RenderPassCreateInfo renderPassInfo{};
			renderPassInfo.attachmentCount = (uint32_t)s.attachmentDescriptions.size();
			renderPassInfo.pAttachments = s.attachmentDescriptions.data();
			renderPassInfo.subpassCount = (uint32_t)s.subpassDescriptions.size();
			renderPassInfo.pSubpasses = s.subpassDescriptions.data();
			renderPassInfo.dependencyCount = (uint32_t)s.subpassDependencies.size();
			renderPassInfo.pDependencies = s.subpassDependencies.data();
			renderPassInfo.pNext = &I;
			// identical to createUnique(const vk::Device &device) except set pNext to use multi-view &I
			return device.createRenderPassUnique(renderPassInfo);
		}

		RenderPassMaker& dependencyBegin(uint32_t srcSubpass, uint32_t dstSubpass)
		{
			vk::SubpassDependency desc{};
			desc.srcSubpass = srcSubpass;
			desc.dstSubpass = dstSubpass;
			s.subpassDependencies.push_back(desc);
			return *this;
		}

		RenderPassMaker& dependencySrcSubpass(uint32_t value)
		{
			s.subpassDependencies.back().srcSubpass = value;
			return *this;
		};

		RenderPassMaker& dependencyDstSubpass(uint32_t value)
		{
			s.subpassDependencies.back().dstSubpass = value;
			return *this;
		};

		RenderPassMaker& dependencySrcStageMask(vk::PipelineStageFlags value)
		{
			s.subpassDependencies.back().srcStageMask = value;
			return *this;
		};

		RenderPassMaker& dependencyDstStageMask(vk::PipelineStageFlags value)
		{
			s.subpassDependencies.back().dstStageMask = value;
			return *this;
		};

		RenderPassMaker& dependencySrcAccessMask(vk::AccessFlags value)
		{
			s.subpassDependencies.back().srcAccessMask = value;
			return *this;
		};

		RenderPassMaker& dependencyDstAccessMask(vk::AccessFlags value)
		{
			s.subpassDependencies.back().dstAccessMask = value;
			return *this;
		};

		RenderPassMaker& dependencyDependencyFlags(vk::DependencyFlags value)
		{
			s.subpassDependencies.back().dependencyFlags = value;
			return *this;
		};
	private:
		constexpr static int max_refs = 64;

		vk::AttachmentReference* getAttachmentReference()
		{
			return (s.num_refs < max_refs) ? &s.attachmentReferences[s.num_refs++] : nullptr;
		}

		struct State
		{
			std::vector<vk::AttachmentDescription> attachmentDescriptions;
			std::vector<vk::SubpassDescription> subpassDescriptions;
			std::vector<vk::SubpassDependency> subpassDependencies;
			std::array<vk::AttachmentReference, max_refs> attachmentReferences;
			int num_refs = 0;
			bool ok_ = false;
		};

		State s;
	};


	/// A class for building pipeline layouts.
	/// Pipeline layouts describe the descriptor sets and push constants used by the shaders.
	class PipelineLayoutMaker
	{
	public:
		PipelineLayoutMaker()
		{
		}

		/// Create a self-deleting pipeline layout object.
		vk::UniquePipelineLayout createUnique(const vk::Device& device) const
		{
			vk::PipelineLayoutCreateInfo pipelineLayoutInfo{
				{}, (uint32_t)setLayouts_.size(),
				setLayouts_.data(), (uint32_t)pushConstantRanges_.size(),
				pushConstantRanges_.data()
			};
			return device.createPipelineLayoutUnique(pipelineLayoutInfo);
		}

		/// Add a descriptor set layout to the pipeline.
		PipelineLayoutMaker& descriptorSetLayout(vk::DescriptorSetLayout layout)
		{
			setLayouts_.push_back(layout);
			return *this;
		}

		/// Add a push constant range to the pipeline.
		/// These describe the size and location of variables in the push constant area.
		PipelineLayoutMaker& pushConstantRange(vk::ShaderStageFlags stageFlags_, uint32_t offset_, uint32_t size_)
		{
			pushConstantRanges_.emplace_back(stageFlags_, offset_, size_);
			return *this;
		}

	private:
		std::vector<vk::DescriptorSetLayout> setLayouts_;
		std::vector<vk::PushConstantRange> pushConstantRanges_;
	};


	struct SpecConst
	{
		uint32_t constantID;
		std::aligned_union<4, VkBool32, uint32_t, int32_t, float, double>::type
		data;
		uint32_t alignment;
		uint32_t size;

		template <typename T>
		SpecConst(uint32_t constantID, T value)
			: constantID(constantID), alignment{alignof(T)}, size(sizeof(T))
		{
			new(&data) T{value};
		}
	};

	/// A class for building pipelines.
	/// All the state of the pipeline is exposed through individual calls.
	/// The pipeline encapsulates all the OpenGL state in a single object.
	/// This includes vertex buffer layouts, blend operations, shaders, line width etc.
	/// This class exposes all the values as individuals so a pipeline can be customised.
	/// The default is to generate a working pipeline.
	class PipelineMaker
	{
	public:
		struct SpecData
		{
			vk::SpecializationInfo specializationInfo_;
			std::vector<vk::SpecializationMapEntry> specializationMapEntries_;
			std::unique_ptr<char []> data_;
			size_t data_size_;

			SpecData()
			{
			}

			template <typename iterator, typename sentinel>
			SpecData(iterator b, sentinel e);

			template <typename SCList>
			SpecData(const SCList& specConstants);

			SpecData(std::initializer_list<SpecConst> list)
				: SpecData(list.begin(), list.end())
			{
			}

			SpecData(SpecData&&) = default;
			SpecData(const SpecData&) = delete;
			SpecData& operator=(SpecData&&) = default;
			SpecData& operator=(const SpecData&) = delete;
		};

	public:
		PipelineMaker()
		{
			// copied from `PipelineMaker(uint32_t width, uint32_t height)`
			inputAssemblyState_.topology = vk::PrimitiveTopology::eTriangleList;
			viewport_ = vk::Viewport{0.0f, 0.0f, .0f, .0f, 0.0f, 1.0f};
			scissor_ = vk::Rect2D{{0, 0}, {0, 0}};
			rasterizationState_.lineWidth = 1.0f;

			// Set up depth test, but do not enable it.
			depthStencilState_.depthTestEnable = VK_FALSE;
			depthStencilState_.depthWriteEnable = VK_TRUE;
			depthStencilState_.depthCompareOp = vk::CompareOp::eLessOrEqual;
			depthStencilState_.depthBoundsTestEnable = VK_FALSE;
			depthStencilState_.back.failOp = vk::StencilOp::eKeep;
			depthStencilState_.back.passOp = vk::StencilOp::eKeep;
			depthStencilState_.back.compareOp = vk::CompareOp::eAlways;
			depthStencilState_.stencilTestEnable = VK_FALSE;
			depthStencilState_.front = depthStencilState_.back;
		}

		PipelineMaker(uint32_t width, uint32_t height)
		{
			inputAssemblyState_.topology = vk::PrimitiveTopology::eTriangleList;
			viewport_ = vk::Viewport{0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f};
			scissor_ = vk::Rect2D{{0, 0}, {width, height}};
			rasterizationState_.lineWidth = 1.0f;

			// Set up depth test, but do not enable it.
			depthStencilState_.depthTestEnable = VK_FALSE;
			depthStencilState_.depthWriteEnable = VK_TRUE;
			depthStencilState_.depthCompareOp = vk::CompareOp::eLessOrEqual;
			depthStencilState_.depthBoundsTestEnable = VK_FALSE;
			depthStencilState_.back.failOp = vk::StencilOp::eKeep;
			depthStencilState_.back.passOp = vk::StencilOp::eKeep;
			depthStencilState_.back.compareOp = vk::CompareOp::eAlways;
			depthStencilState_.stencilTestEnable = VK_FALSE;
			depthStencilState_.front = depthStencilState_.back;
		}

		vk::UniquePipeline createUnique(const vk::Device& device,
		                                const vk::PipelineCache& pipelineCache,
		                                const vk::PipelineLayout& pipelineLayout,
		                                const vk::RenderPass& renderPass, bool defaultBlend = true)
		{
			// Add default colour blend attachment if necessary.
			// Warning: createUnique with dynamic rendering copy from this function
			if (colorBlendAttachments_.empty() && defaultBlend)
			{
				vk::PipelineColorBlendAttachmentState blend{};
				blend.blendEnable = 0;
				blend.srcColorBlendFactor = vk::BlendFactor::eOne;
				blend.dstColorBlendFactor = vk::BlendFactor::eZero;
				blend.colorBlendOp = vk::BlendOp::eAdd;
				blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
				blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
				blend.alphaBlendOp = vk::BlendOp::eAdd;
				typedef vk::ColorComponentFlagBits ccbf;
				blend.colorWriteMask = ccbf::eR | ccbf::eG | ccbf::eB | ccbf::eA;
				colorBlendAttachments_.push_back(blend);
			}

			auto count = (uint32_t)colorBlendAttachments_.size();
			colorBlendState_.attachmentCount = count;
			colorBlendState_.pAttachments = count ? colorBlendAttachments_.data() : nullptr;

			vk::PipelineViewportStateCreateInfo viewportState{
				{}, 1, &viewport_, 1, &scissor_
			};

			vk::PipelineVertexInputStateCreateInfo vertexInputState;
			vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions_.size();
			vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions_.data();
			vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions_.size();
			vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions_.data();

			vk::PipelineDynamicStateCreateInfo dynState{{}, (uint32_t)dynamicState_.size(), dynamicState_.data()};

			vk::GraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.pVertexInputState = &vertexInputState;
			pipelineInfo.stageCount = (uint32_t)modules_.size();
			pipelineInfo.pStages = modules_.data();
			pipelineInfo.pInputAssemblyState = &inputAssemblyState_;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizationState_;
			pipelineInfo.pMultisampleState = &multisampleState_;
			pipelineInfo.pColorBlendState = &colorBlendState_;
			pipelineInfo.pDepthStencilState = &depthStencilState_;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = renderPass;
			pipelineInfo.pDynamicState = dynamicState_.empty() ? nullptr : &dynState;
			pipelineInfo.subpass = subpass_;
			pipelineInfo.pTessellationState = &tessellationState_;

			return device.createGraphicsPipelineUnique(pipelineCache, pipelineInfo).value;
		}

		// Dynamic rendering api
		vk::UniquePipeline createUnique(const vk::Device& device,
		                                const vk::PipelineCache& pipelineCache,
		                                const vk::PipelineLayout& pipelineLayout,
		                                const vk::PipelineRenderingCreateInfo& pipelineRenderingCreateInfo,
		                                bool defaultBlend = true)
		{
			if (colorBlendAttachments_.empty() && defaultBlend)
			{
				vk::PipelineColorBlendAttachmentState blend{};
				blend.blendEnable = 0;
				blend.srcColorBlendFactor = vk::BlendFactor::eOne;
				blend.dstColorBlendFactor = vk::BlendFactor::eZero;
				blend.colorBlendOp = vk::BlendOp::eAdd;
				blend.srcAlphaBlendFactor = vk::BlendFactor::eOne;
				blend.dstAlphaBlendFactor = vk::BlendFactor::eZero;
				blend.alphaBlendOp = vk::BlendOp::eAdd;
				typedef vk::ColorComponentFlagBits ccbf;
				blend.colorWriteMask = ccbf::eR | ccbf::eG | ccbf::eB | ccbf::eA;
				colorBlendAttachments_.push_back(blend);
			}

			auto count = (uint32_t)colorBlendAttachments_.size();
			colorBlendState_.attachmentCount = count;
			colorBlendState_.pAttachments = count ? colorBlendAttachments_.data() : nullptr;

			vk::PipelineViewportStateCreateInfo viewportState{
				{}, 1, &viewport_, 1, &scissor_
			};

			vk::PipelineVertexInputStateCreateInfo vertexInputState;
			vertexInputState.vertexAttributeDescriptionCount = (uint32_t)vertexAttributeDescriptions_.size();
			vertexInputState.pVertexAttributeDescriptions = vertexAttributeDescriptions_.data();
			vertexInputState.vertexBindingDescriptionCount = (uint32_t)vertexBindingDescriptions_.size();
			vertexInputState.pVertexBindingDescriptions = vertexBindingDescriptions_.data();

			vk::PipelineDynamicStateCreateInfo dynState{{}, (uint32_t)dynamicState_.size(), dynamicState_.data()};

			vk::GraphicsPipelineCreateInfo pipelineInfo{};
			pipelineInfo.pNext = &pipelineRenderingCreateInfo;
			pipelineInfo.pVertexInputState = &vertexInputState;
			pipelineInfo.stageCount = (uint32_t)modules_.size();
			pipelineInfo.pStages = modules_.data();
			pipelineInfo.pInputAssemblyState = &inputAssemblyState_;
			pipelineInfo.pViewportState = &viewportState;
			pipelineInfo.pRasterizationState = &rasterizationState_;
			pipelineInfo.pMultisampleState = &multisampleState_;
			pipelineInfo.pColorBlendState = &colorBlendState_;
			pipelineInfo.pDepthStencilState = &depthStencilState_;
			pipelineInfo.layout = pipelineLayout;
			pipelineInfo.renderPass = VK_NULL_HANDLE;
			pipelineInfo.pDynamicState = dynamicState_.empty() ? nullptr : &dynState;
			pipelineInfo.pTessellationState = &tessellationState_;

			return device.createGraphicsPipelineUnique(pipelineCache, pipelineInfo).value;
		}

		/// Add a shader module to the pipeline.
		PipelineMaker& shader(vk::ShaderStageFlagBits stage, const vk::ShaderModule& shader,
		                      const char* entryPoint = "main")
		{
			vk::PipelineShaderStageCreateInfo info{};
			info.module = shader;
			info.pName = entryPoint;
			info.stage = stage;
			modules_.emplace_back(info);
			return *this;
		}

		/// Add a shader module with specialized constants to the pipeline.
		PipelineMaker& shader(vk::ShaderStageFlagBits stage, vk::ShaderModule& shader,
		                      SpecData specConstants,
		                      const char* entryPoint = "main")
		{
			auto data = std::unique_ptr<SpecData>{new SpecData{std::move(specConstants)}};
			vk::PipelineShaderStageCreateInfo info{};
			info.module = shader;
			info.pName = entryPoint;
			info.stage = stage;
			info.pSpecializationInfo = &data->specializationInfo_;
			modules_.emplace_back(info);
			moduleSpecializations_.emplace_back(std::move(data));
			return *this;
		}

		/// Add a blend state to the pipeline for one colour attachment.
		/// If you don't do this, a default is used.
		PipelineMaker& colorBlend(const vk::PipelineColorBlendAttachmentState& state)
		{
			colorBlendAttachments_.push_back(state);
			return *this;
		}

		PipelineMaker& subPass(uint32_t subpass)
		{
			subpass_ = subpass;
			return *this;
		}

		/// Begin setting colour blend value
		/// If you don't do this, a default is used.
		/// Follow this with blendEnable() blendSrcColorBlendFactor() etc.
		/// Default is a regular alpha blend.
		PipelineMaker& blendBegin(vk::Bool32 enable)
		{
			colorBlendAttachments_.emplace_back();
			auto& blend = colorBlendAttachments_.back();
			blend.blendEnable = enable;
			blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
			blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			blend.colorBlendOp = vk::BlendOp::eAdd;
			blend.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
			blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
			blend.alphaBlendOp = vk::BlendOp::eAdd;
			typedef vk::ColorComponentFlagBits ccbf;
			blend.colorWriteMask = ccbf::eR | ccbf::eG | ccbf::eB | ccbf::eA;
			return *this;
		}

		/// Enable or disable blending (called after blendBegin())
		PipelineMaker& blendEnable(vk::Bool32 value)
		{
			return blendBegin(value);
		}

		/// Source colour blend factor (called after blendBegin())
		PipelineMaker& blendSrcColorBlendFactor(vk::BlendFactor value)
		{
			colorBlendAttachments_.back().srcColorBlendFactor = value;
			return *this;
		}

		/// Destination colour blend factor (called after blendBegin())
		PipelineMaker& blendDstColorBlendFactor(vk::BlendFactor value)
		{
			colorBlendAttachments_.back().dstColorBlendFactor = value;
			return *this;
		}

		/// Blend operation (called after blendBegin())
		PipelineMaker& blendColorBlendOp(vk::BlendOp value)
		{
			colorBlendAttachments_.back().colorBlendOp = value;
			return *this;
		}

		/// Source alpha blend factor (called after blendBegin())
		PipelineMaker& blendSrcAlphaBlendFactor(vk::BlendFactor value)
		{
			colorBlendAttachments_.back().srcAlphaBlendFactor = value;
			return *this;
		}

		/// Destination alpha blend factor (called after blendBegin())
		PipelineMaker& blendDstAlphaBlendFactor(vk::BlendFactor value)
		{
			colorBlendAttachments_.back().dstAlphaBlendFactor = value;
			return *this;
		}

		/// Alpha operation (called after blendBegin())
		PipelineMaker& blendAlphaBlendOp(vk::BlendOp value)
		{
			colorBlendAttachments_.back().alphaBlendOp = value;
			return *this;
		}

		/// Colour write mask (called after blendBegin())
		PipelineMaker& blendColorWriteMask(vk::ColorComponentFlags value)
		{
			colorBlendAttachments_.back().colorWriteMask = value;
			return *this;
		}

		/// Add a vertex attribute to the pipeline.
		PipelineMaker& vertexAttribute(uint32_t location_, uint32_t binding_, vk::Format format_, uint32_t offset_)
		{
			vertexAttributeDescriptions_.push_back({location_, binding_, format_, offset_});
			return *this;
		}

		/// Add a vertex attribute to the pipeline.
		PipelineMaker& vertexAttribute(const vk::VertexInputAttributeDescription& desc)
		{
			vertexAttributeDescriptions_.push_back(desc);
			return *this;
		}

		/// Add a vertex binding to the pipeline.
		/// Usually only one of these is needed to specify the stride.
		/// Vertices can also be delivered one per instance.
		PipelineMaker& vertexBinding(uint32_t binding_, uint32_t stride_,
		                             vk::VertexInputRate inputRate_ = vk::VertexInputRate::eVertex)
		{
			vertexBindingDescriptions_.push_back({binding_, stride_, inputRate_});
			return *this;
		}

		/// Add a vertex binding to the pipeline.
		/// Usually only one of these is needed to specify the stride.
		/// Vertices can also be delivered one per instance.
		PipelineMaker& vertexBinding(const vk::VertexInputBindingDescription& desc)
		{
			vertexBindingDescriptions_.push_back(desc);
			return *this;
		}

		/// Specify the topology of the pipeline.
		/// Usually this is a triangle list, but points and lines are possible too.
		PipelineMaker& topology(vk::PrimitiveTopology topology)
		{
			inputAssemblyState_.topology = topology;
			return *this;
		}

		/// Specify patch count.
		/// Applies when (inputAssemblyState_.topology == vk::PrimitiveTopology::ePatchList).
		PipelineMaker& setPatchControlPoints(uint32_t patchControlPoints)
		{
			tessellationState_.setPatchControlPoints(patchControlPoints);
			return *this;
		}

		/// Enable or disable primitive restart.
		/// If using triangle strips, for example, this allows a special index value (0xffff or 0xffffffff) to start a new strip.
		PipelineMaker& primitiveRestartEnable(vk::Bool32 primitiveRestartEnable)
		{
			inputAssemblyState_.primitiveRestartEnable = primitiveRestartEnable;
			return *this;
		}

		/// Set a whole new input assembly state.
		/// Note you can set individual values with their own call
		PipelineMaker& inputAssemblyState(const vk::PipelineInputAssemblyStateCreateInfo& value)
		{
			inputAssemblyState_ = value;
			return *this;
		}

		/// Set the viewport value.
		/// Usually there is only one viewport, but you can have multiple viewports active for rendering cubemaps or VR stereo pair
		PipelineMaker& viewport(const vk::Viewport& value)
		{
			viewport_ = value;
			return *this;
		}

		/// Set the scissor value.
		/// This defines the area that the fragment shaders can write to. For example, if you are rendering a portal or a mirror.
		PipelineMaker& scissor(const vk::Rect2D& value)
		{
			scissor_ = value;
			return *this;
		}

		/// Set a whole rasterization state.
		/// Note you can set individual values with their own call
		PipelineMaker& rasterizationState(const vk::PipelineRasterizationStateCreateInfo& value)
		{
			rasterizationState_ = value;
			return *this;
		}

		PipelineMaker& depthClampEnable(vk::Bool32 value)
		{
			rasterizationState_.depthClampEnable = value;
			return *this;
		}

		PipelineMaker& rasterizerDiscardEnable(vk::Bool32 value)
		{
			rasterizationState_.rasterizerDiscardEnable = value;
			return *this;
		}

		PipelineMaker& polygonMode(vk::PolygonMode value)
		{
			rasterizationState_.polygonMode = value;
			return *this;
		}

		PipelineMaker& cullMode(vk::CullModeFlags value)
		{
			rasterizationState_.cullMode = value;
			return *this;
		}

		PipelineMaker& frontFace(vk::FrontFace value)
		{
			rasterizationState_.frontFace = value;
			return *this;
		}

		PipelineMaker& depthBiasEnable(vk::Bool32 value)
		{
			rasterizationState_.depthBiasEnable = value;
			return *this;
		}

		PipelineMaker& depthBiasConstantFactor(float value)
		{
			rasterizationState_.depthBiasConstantFactor = value;
			return *this;
		}

		PipelineMaker& depthBiasClamp(float value)
		{
			rasterizationState_.depthBiasClamp = value;
			return *this;
		}

		PipelineMaker& depthBiasSlopeFactor(float value)
		{
			rasterizationState_.depthBiasSlopeFactor = value;
			return *this;
		}

		PipelineMaker& lineWidth(float value)
		{
			rasterizationState_.lineWidth = value;
			return *this;
		}


		/// Set a whole multi sample state.
		/// Note you can set individual values with their own call
		PipelineMaker& multisampleState(const vk::PipelineMultisampleStateCreateInfo& value)
		{
			multisampleState_ = value;
			return *this;
		}

		PipelineMaker& rasterizationSamples(vk::SampleCountFlagBits value)
		{
			multisampleState_.rasterizationSamples = value;
			return *this;
		}

		PipelineMaker& sampleShadingEnable(vk::Bool32 value)
		{
			multisampleState_.sampleShadingEnable = value;
			return *this;
		}

		PipelineMaker& minSampleShading(float value)
		{
			multisampleState_.minSampleShading = value;
			return *this;
		}

		PipelineMaker& pSampleMask(const vk::SampleMask* value)
		{
			multisampleState_.pSampleMask = value;
			return *this;
		}

		PipelineMaker& alphaToCoverageEnable(vk::Bool32 value)
		{
			multisampleState_.alphaToCoverageEnable = value;
			return *this;
		}

		PipelineMaker& alphaToOneEnable(vk::Bool32 value)
		{
			multisampleState_.alphaToOneEnable = value;
			return *this;
		}

		/// Set a whole depth stencil state.
		/// Note you can set individual values with their own call
		PipelineMaker& depthStencilState(const vk::PipelineDepthStencilStateCreateInfo& value)
		{
			depthStencilState_ = value;
			return *this;
		}

		PipelineMaker& depthTestEnable(vk::Bool32 value)
		{
			depthStencilState_.depthTestEnable = value;
			return *this;
		}

		PipelineMaker& depthWriteEnable(vk::Bool32 value)
		{
			depthStencilState_.depthWriteEnable = value;
			return *this;
		}

		PipelineMaker& depthCompareOp(vk::CompareOp value)
		{
			depthStencilState_.depthCompareOp = value;
			return *this;
		}

		PipelineMaker& depthBoundsTestEnable(vk::Bool32 value)
		{
			depthStencilState_.depthBoundsTestEnable = value;
			return *this;
		}

		PipelineMaker& stencilTestEnable(vk::Bool32 value)
		{
			depthStencilState_.stencilTestEnable = value;
			return *this;
		}

		PipelineMaker& front(vk::StencilOpState value)
		{
			depthStencilState_.front = value;
			return *this;
		}

		PipelineMaker& back(vk::StencilOpState value)
		{
			depthStencilState_.back = value;
			return *this;
		}

		PipelineMaker& minDepthBounds(float value)
		{
			depthStencilState_.minDepthBounds = value;
			return *this;
		}

		PipelineMaker& maxDepthBounds(float value)
		{
			depthStencilState_.maxDepthBounds = value;
			return *this;
		}

		/// Set a whole colour blend state.
		/// Note you can set individual values with their own call
		PipelineMaker& colorBlendState(const vk::PipelineColorBlendStateCreateInfo& value)
		{
			colorBlendState_ = value;
			return *this;
		}

		PipelineMaker& logicOpEnable(vk::Bool32 value)
		{
			colorBlendState_.logicOpEnable = value;
			return *this;
		}

		PipelineMaker& logicOp(vk::LogicOp value)
		{
			colorBlendState_.logicOp = value;
			return *this;
		}

		PipelineMaker& blendConstants(float r, float g, float b, float a)
		{
			float* bc = colorBlendState_.blendConstants;
			bc[0] = r;
			bc[1] = g;
			bc[2] = b;
			bc[3] = a;
			return *this;
		}

		PipelineMaker& dynamicState(vk::DynamicState value)
		{
			dynamicState_.push_back(value);
			return *this;
		}

	private:
		vk::PipelineInputAssemblyStateCreateInfo inputAssemblyState_;
		vk::Viewport viewport_;
		vk::Rect2D scissor_;
		vk::PipelineRasterizationStateCreateInfo rasterizationState_;
		vk::PipelineMultisampleStateCreateInfo multisampleState_;
		vk::PipelineDepthStencilStateCreateInfo depthStencilState_;
		vk::PipelineColorBlendStateCreateInfo colorBlendState_;
		vk::PipelineTessellationStateCreateInfo tessellationState_;
		std::vector<vk::PipelineColorBlendAttachmentState> colorBlendAttachments_;
		std::vector<vk::PipelineShaderStageCreateInfo> modules_;
		std::vector<std::unique_ptr<SpecData>> moduleSpecializations_;
		std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions_;
		std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions_;
		std::vector<vk::DynamicState> dynamicState_;
		uint32_t subpass_ = 0;
	};

	template <typename iterator, typename sentinel>
	PipelineMaker::SpecData::SpecData(iterator b, sentinel e)
	{
		auto round_offset = [](uint32_t offset, uint32_t alignment)
		{
			uint32_t unaligned = offset & (alignment - 1);
			return unaligned == 0 ? offset : offset + alignment - unaligned;
		};
		uint32_t offset = 0;
		for (auto it = b; it != e; ++it)
		{
			auto& entry = *it;
			offset = round_offset(offset, entry.alignment) + entry.size;
		}
		data_size_ = offset;
		// We rely on the fact that new allocates with the maximum basic type alignment.
		data_ = std::unique_ptr<char[]>(new char[data_size_]);
		offset = 0;
		int specCount = 0;
		for (auto it = b; it != e; ++it)
		{
			auto& entry = *it;
			offset = round_offset(offset, entry.alignment);
			specializationMapEntries_.emplace_back(
				entry.constantID, offset, entry.size);
			const char* src = reinterpret_cast<const char*>(&entry.data);
			std::copy(src, src + entry.size, data_.get() + offset);
			offset += entry.size;
			++specCount;
		}
		specializationInfo_.mapEntryCount = specCount;
		specializationInfo_.pMapEntries = specializationMapEntries_.data();
		specializationInfo_.dataSize = data_size_;
		specializationInfo_.pData = data_.get();
	}

	template <typename SCList>
	PipelineMaker::SpecData::SpecData(const SCList& specConstants)
	{
		auto round_offset = [](uint32_t offset, uint32_t alignment)
		{
			uint32_t unaligned = offset & (alignment - 1);
			return unaligned == 0 ? offset : offset + alignment - unaligned;
		};
		uint32_t offset = 0;
		for (auto& entry : specConstants)
		{
			offset = round_offset(offset, entry.alignment) + entry.size;
		}
		data_size_ = offset;
		// We rely on the fact that new allocates with the maximum basic type alignment.
		data_ = std::unique_ptr<char[]>(new char[data_size_]);
		offset = 0;
		for (auto& entry : specConstants)
		{
			offset = round_offset(offset, entry.alignment);
			specializationMapEntries_.emplace_back(
				entry.constantID, offset, entry.size);
			const char* src = reinterpret_cast<const char*>(&entry.data);
			std::copy(src, src + entry.size, data_.get() + offset);
			offset += entry.size;
		}
		specializationInfo_.mapEntryCount = static_cast<uint32_t>(specConstants.size());
		specializationInfo_.pMapEntries = specializationMapEntries_.data();
		specializationInfo_.dataSize = data_size_;
		specializationInfo_.pData = data_.get();
	}

	/// A class for building compute pipelines.
	class ComputePipelineMaker
	{
	public:
		ComputePipelineMaker()
		{
		}

		/// Add a shader module to the pipeline.
		ComputePipelineMaker& shader(vk::ShaderStageFlagBits stage, const vk::ShaderModule& shader,
		                             const char* entryPoint = "main")
		{
			stage_.module = shader;
			stage_.pName = entryPoint;
			stage_.stage = stage;
			return *this;
		}

		/// Set the compute shader module.
		ComputePipelineMaker& module(const vk::PipelineShaderStageCreateInfo& value)
		{
			stage_ = value;
			return *this;
		}

		/// Create a managed handle to a compute shader.
		vk::UniquePipeline createUnique(vk::Device device, const vk::PipelineCache& pipelineCache,
		                                const vk::PipelineLayout& pipelineLayout) const
		{
			vk::ComputePipelineCreateInfo pipelineInfo{};

			pipelineInfo.stage = stage_;
			pipelineInfo.layout = pipelineLayout;

			return device.createComputePipelineUnique(pipelineCache, pipelineInfo).value;
		}

	private:
		vk::PipelineShaderStageCreateInfo stage_;
	};

	/// Convenience class for updating descriptor sets (uniforms)
	class DescriptorSetUpdater
	{
	public:
		DescriptorSetUpdater(int maxBuffers = 10, int maxImages = 10, int maxBufferViews = 0)
		{
			// we must pre-size these buffers as we take pointers to their members.
			bufferInfo_.resize(maxBuffers);
			imageInfo_.resize(maxImages);
			bufferViews_.resize(maxBufferViews);
		}

		/// Call this to begin a new descriptor set.
		DescriptorSetUpdater& beginDescriptorSet(vk::DescriptorSet dstSet)
		{
			dstSet_ = dstSet;
			return *this;
		}

		/// Call this to begin a new set of images.
		DescriptorSetUpdater& beginImages(uint32_t dstBinding, uint32_t dstArrayElement,
		                                  vk::DescriptorType descriptorType)
		{
			vk::WriteDescriptorSet wdesc{};
			wdesc.dstSet = dstSet_;
			wdesc.dstBinding = dstBinding;
			wdesc.dstArrayElement = dstArrayElement;
			wdesc.descriptorCount = 0;
			wdesc.descriptorType = descriptorType;
			wdesc.pImageInfo = imageInfo_.data() + numImages_;
			descriptorWrites_.push_back(wdesc);
			return *this;
		}

		/// Call this to add a combined image sampler.
		DescriptorSetUpdater& image(vk::Sampler sampler, vk::ImageView imageView, vk::ImageLayout imageLayout)
		{
			if (!descriptorWrites_.empty() && numImages_ != imageInfo_.size() && descriptorWrites_.back().pImageInfo)
			{
				descriptorWrites_.back().descriptorCount++;
				imageInfo_[numImages_++] = vk::DescriptorImageInfo{sampler, imageView, imageLayout};
			}
			else
			{
				ok_ = false;
			}
			return *this;
		}

		/// Call this to start defining buffers.
		DescriptorSetUpdater& beginBuffers(uint32_t dstBinding, uint32_t dstArrayElement,
		                                   vk::DescriptorType descriptorType)
		{
			vk::WriteDescriptorSet wdesc{};
			wdesc.dstSet = dstSet_;
			wdesc.dstBinding = dstBinding;
			wdesc.dstArrayElement = dstArrayElement;
			wdesc.descriptorCount = 0;
			wdesc.descriptorType = descriptorType;
			wdesc.pBufferInfo = bufferInfo_.data() + numBuffers_;
			descriptorWrites_.push_back(wdesc);
			return *this;
		}

		/// Call this to add a new buffer.
		DescriptorSetUpdater& buffer(vk::Buffer buffer, vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE)
		{
			if (!descriptorWrites_.empty() && numBuffers_ != bufferInfo_.size() && descriptorWrites_.back().pBufferInfo)
			{
				descriptorWrites_.back().descriptorCount++;
				bufferInfo_[numBuffers_++] = vk::DescriptorBufferInfo{buffer, offset, range};
			}
			else
			{
				ok_ = false;
			}
			return *this;
		}

		/// Call this to start adding buffer views. (for example, writable images).
		void beginBufferViews(uint32_t dstBinding, uint32_t dstArrayElement, vk::DescriptorType descriptorType)
		{
			vk::WriteDescriptorSet wdesc{};
			wdesc.dstSet = dstSet_;
			wdesc.dstBinding = dstBinding;
			wdesc.dstArrayElement = dstArrayElement;
			wdesc.descriptorCount = 0;
			wdesc.descriptorType = descriptorType;
			wdesc.pTexelBufferView = bufferViews_.data() + numBufferViews_;
			descriptorWrites_.push_back(wdesc);
		}

		/// Call this to add a buffer view. (Texel images)
		void bufferView(vk::BufferView view)
		{
			if (!descriptorWrites_.empty() && numBufferViews_ != bufferViews_.size() && descriptorWrites_.back().
				pImageInfo)
			{
				descriptorWrites_.back().descriptorCount++;
				bufferViews_[numBufferViews_++] = view;
			}
			else
			{
				ok_ = false;
			}
		}

		/// Copy an existing descriptor.
		void copy(vk::DescriptorSet srcSet, uint32_t srcBinding, uint32_t srcArrayElement, vk::DescriptorSet dstSet,
		          uint32_t dstBinding, uint32_t dstArrayElement, uint32_t descriptorCount)
		{
			descriptorCopies_.emplace_back(srcSet, srcBinding, srcArrayElement, dstSet, dstBinding, dstArrayElement,
			                               descriptorCount);
		}

		/// Call this to update the descriptor sets with their pointers (but not data).
		void update(const vk::Device& device) const
		{
			device.updateDescriptorSets(descriptorWrites_, descriptorCopies_);
		}

		/// Returns true if the updater is error free.
		bool ok() const { return ok_; }
	private:
		std::vector<vk::DescriptorBufferInfo> bufferInfo_;
		std::vector<vk::DescriptorImageInfo> imageInfo_;
		std::vector<vk::WriteDescriptorSet> descriptorWrites_;
		std::vector<vk::CopyDescriptorSet> descriptorCopies_;
		std::vector<vk::BufferView> bufferViews_;
		vk::DescriptorSet dstSet_;
		int numBuffers_ = 0;
		int numImages_ = 0;
		int numBufferViews_ = 0;
		bool ok_ = true;
	};

	/// A factory class for descriptor set layouts. (An interface to the shaders)
	class DescriptorSetLayoutMaker
	{
	public:
		DescriptorSetLayoutMaker()
		{
		}

		DescriptorSetLayoutMaker& buffer(uint32_t binding, vk::DescriptorType descriptorType,
		                                 vk::ShaderStageFlags stageFlags, uint32_t descriptorCount)
		{
			s.bindings.emplace_back(binding, descriptorType, descriptorCount, stageFlags, nullptr);
			return *this;
		}

		DescriptorSetLayoutMaker& image(uint32_t binding, vk::DescriptorType descriptorType,
		                                vk::ShaderStageFlags stageFlags, uint32_t descriptorCount)
		{
			s.bindings.emplace_back(binding, descriptorType, descriptorCount, stageFlags, nullptr);
			return *this;
		}

		DescriptorSetLayoutMaker& samplers(uint32_t binding, vk::DescriptorType descriptorType,
		                                   vk::ShaderStageFlags stageFlags,
		                                   const std::vector<vk::Sampler> immutableSamplers)
		{
			s.samplers.push_back(immutableSamplers);
			auto pImmutableSamplers = s.samplers.back().data();
			s.bindings.emplace_back(binding, descriptorType, (uint32_t)immutableSamplers.size(), stageFlags,
			                        pImmutableSamplers);
			return *this;
		}

		DescriptorSetLayoutMaker& bufferView(uint32_t binding, vk::DescriptorType descriptorType,
		                                     vk::ShaderStageFlags stageFlags, uint32_t descriptorCount)
		{
			s.bindings.emplace_back(binding, descriptorType, descriptorCount, stageFlags, nullptr);
			return *this;
		}

		/// Create a self-deleting descriptor set object.
		vk::UniqueDescriptorSetLayout createUnique(vk::Device device) const
		{
			vk::DescriptorSetLayoutCreateInfo dsci{};
			dsci.bindingCount = (uint32_t)s.bindings.size();
			dsci.pBindings = s.bindings.data();
			return device.createDescriptorSetLayoutUnique(dsci);
		}

	private:
		struct State
		{
			std::vector<vk::DescriptorSetLayoutBinding> bindings;
			std::vector<std::vector<vk::Sampler>> samplers;
			int numSamplers = 0;
		};

		State s;
	};

	/// A factory class for descriptor sets (A set of uniform bindings)
	class DescriptorSetMaker
	{
	public:
		// Construct a new, empty DescriptorSetMaker.
		DescriptorSetMaker()
		{
		}

		/// Add another layout describing a descriptor set.
		DescriptorSetMaker& layout(vk::DescriptorSetLayout layout)
		{
			s.layouts.push_back(layout);
			return *this;
		}

		/// Allocate a vector of non-self-deleting descriptor sets
		/// Note: descriptor sets get freed with the pool, so this is the better choice.
		std::vector<vk::DescriptorSet> create(vk::Device device, vk::DescriptorPool descriptorPool) const
		{
			vk::DescriptorSetAllocateInfo dsai{};
			dsai.descriptorPool = descriptorPool;
			dsai.descriptorSetCount = (uint32_t)s.layouts.size();
			dsai.pSetLayouts = s.layouts.data();
			return device.allocateDescriptorSets(dsai);
		}

		/// Allocate a vector of self-deleting descriptor sets.
		std::vector<vk::UniqueDescriptorSet> createUnique(vk::Device device, vk::DescriptorPool descriptorPool) const
		{
			vk::DescriptorSetAllocateInfo dsai{};
			dsai.descriptorPool = descriptorPool;
			dsai.descriptorSetCount = (uint32_t)s.layouts.size();
			dsai.pSetLayouts = s.layouts.data();
			return device.allocateDescriptorSetsUnique(dsai);
		}

	private:
		struct State
		{
			std::vector<vk::DescriptorSetLayout> layouts;
		};

		State s;
	};

	/// A class to help build samplers.
	/// Samplers tell the shader stages how to sample an image.
	/// They are used in combination with an image to make a combined image sampler
	/// used by texture() calls in shaders.
	/// They can also be passed to shaders directly for use on multiple image sources.
	class SamplerMaker
	{
	public:
		/// Default to a very basic sampler.
		SamplerMaker()
		{
			s.info.magFilter = vk::Filter::eNearest;
			s.info.minFilter = vk::Filter::eNearest;
			s.info.mipmapMode = vk::SamplerMipmapMode::eNearest;
			s.info.addressModeU = vk::SamplerAddressMode::eRepeat;
			s.info.addressModeV = vk::SamplerAddressMode::eRepeat;
			s.info.addressModeW = vk::SamplerAddressMode::eRepeat;
			s.info.mipLodBias = 0.0f;
			s.info.anisotropyEnable = 0;
			s.info.maxAnisotropy = 0.0f;
			s.info.compareEnable = 0;
			s.info.compareOp = vk::CompareOp::eNever;
			s.info.minLod = 0;
			s.info.maxLod = 0;
			s.info.borderColor = vk::BorderColor{};
			s.info.unnormalizedCoordinates = 0;
		}

		////////////////////
		//
		// Setters
		//
		SamplerMaker& flags(vk::SamplerCreateFlags value)
		{
			s.info.flags = value;
			return *this;
		}

		/// Set the magnify filter value. (for close textures)
		/// Options are: vk::Filter::eLinear and vk::Filter::eNearest
		SamplerMaker& magFilter(vk::Filter value)
		{
			s.info.magFilter = value;
			return *this;
		}

		/// Set the minnify filter value. (for far away textures)
		/// Options are: vk::Filter::eLinear and vk::Filter::eNearest
		SamplerMaker& minFilter(vk::Filter value)
		{
			s.info.minFilter = value;
			return *this;
		}

		/// Set the minnify filter value. (for far away textures)
		/// Options are: vk::SamplerMipmapMode::eLinear and vk::SamplerMipmapMode::eNearest
		SamplerMaker& mipmapMode(vk::SamplerMipmapMode value)
		{
			s.info.mipmapMode = value;
			return *this;
		}

		SamplerMaker& addressModeU(vk::SamplerAddressMode value)
		{
			s.info.addressModeU = value;
			return *this;
		}

		SamplerMaker& addressModeV(vk::SamplerAddressMode value)
		{
			s.info.addressModeV = value;
			return *this;
		}

		SamplerMaker& addressModeW(vk::SamplerAddressMode value)
		{
			s.info.addressModeW = value;
			return *this;
		}

		SamplerMaker& mipLodBias(float value)
		{
			s.info.mipLodBias = value;
			return *this;
		}

		SamplerMaker& anisotropyEnable(vk::Bool32 value)
		{
			s.info.anisotropyEnable = value;
			return *this;
		}

		SamplerMaker& maxAnisotropy(float value)
		{
			s.info.maxAnisotropy = value;
			return *this;
		}

		SamplerMaker& compareEnable(vk::Bool32 value)
		{
			s.info.compareEnable = value;
			return *this;
		}

		SamplerMaker& compareOp(vk::CompareOp value)
		{
			s.info.compareOp = value;
			return *this;
		}

		SamplerMaker& minLod(float value)
		{
			s.info.minLod = value;
			return *this;
		}

		SamplerMaker& maxLod(float value)
		{
			s.info.maxLod = value;
			return *this;
		}

		SamplerMaker& borderColor(vk::BorderColor value)
		{
			s.info.borderColor = value;
			return *this;
		}

		SamplerMaker& unnormalizedCoordinates(vk::Bool32 value)
		{
			s.info.unnormalizedCoordinates = value;
			return *this;
		}

		/// Allocate a self-deleting image.
		vk::UniqueSampler createUnique(vk::Device device) const
		{
			return device.createSamplerUnique(s.info);
		}

		/// Allocate a non self-deleting Sampler.
		vk::Sampler create(vk::Device device) const
		{
			return device.createSampler(s.info);
		}

	private:
		struct State
		{
			vk::SamplerCreateInfo info;
		};

		State s;
	};

	/// KTX files use OpenGL format values. This converts some common ones to Vulkan equivalents.
	inline vk::Format GLtoVKFormat(uint32_t glFormat)
	{
		switch (glFormat)
		{
		case 0x1907: return vk::Format::eR8G8B8Unorm; // GL_RGB
		case 0x1908: return vk::Format::eR8G8B8A8Unorm; // GL_RGBA
		case 0x83F0: return vk::Format::eBc1RgbUnormBlock; // GL_COMPRESSED_RGB_S3TC_DXT1_EXT
		case 0x83F1: return vk::Format::eBc1RgbaUnormBlock; // GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
		case 0x83F2: return vk::Format::eBc3UnormBlock; // GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
		case 0x83F3: return vk::Format::eBc5UnormBlock; // GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
		}
		return vk::Format::eUndefined;
	}
} // namespace vku

#endif // VKU_HPP
