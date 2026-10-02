#include "render/vulkan/VulkanVertexUtils.hpp"
#include "render/ChunkVertex.hpp"
#include "render/GpuTypes.hpp"

namespace render::vulkan {
	VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding   = 0;
		bindingDescription.stride    = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding  = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset   = offsetof(render::Vertex, pos);

		attributeDescriptions[1].binding  = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset   = offsetof(render::Vertex, color);

		attributeDescriptions[2].binding  = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset   = offsetof(render::Vertex, texCoord);

		return attributeDescriptions;
	}

	VkVertexInputBindingDescription getChunkBindingDescription() {
		return {.binding = 0, .stride = sizeof(ChunkVertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};
	}

	VkVertexInputAttributeDescription getChunkAttributeDescription() {
		constexpr std::array formats = {VK_FORMAT_R32_UINT, VK_FORMAT_R32G32_UINT, VK_FORMAT_R32G32B32_UINT,
										VK_FORMAT_R32G32B32A32_UINT};
		static_assert(chunkvertex::wordCount >= 1 && chunkvertex::wordCount <= formats.size(),
					  "a chunk vertex is read as one vertex attribute of at most 4 words");

		return {.location = 0, .binding = 0, .format = formats[chunkvertex::wordCount - 1], .offset = 0};
	}

	ChunkVertexSpecialization::ChunkVertexSpecialization() {
		using chunkvertex::FieldId;
		constexpr std::array shaderFields = {FieldId::PositionX, FieldId::PositionY, FieldId::PositionZ, FieldId::UvU,
											 FieldId::UvV};

		const auto add = [this](const std::uint32_t value) {
			const auto id = static_cast<std::uint32_t>(m_data.size());
			m_entries.push_back({.constantID = id,
								 .offset     = static_cast<std::uint32_t>(m_data.size() * sizeof(std::uint32_t)),
								 .size       = sizeof(std::uint32_t)});
			m_data.push_back(value);
		};

		for (const FieldId id: shaderFields) {
			const chunkvertex::Field& field = chunkvertex::field(id);
			add(field.word);
			add(field.shift);
			add(field.bits);
		}
		add(chunkvertex::positionStepsPerBlock);
		add(chunkvertex::uvStepsPerTexture);

		m_info = {.mapEntryCount = static_cast<std::uint32_t>(m_entries.size()),
				  .pMapEntries   = m_entries.data(),
				  .dataSize      = m_data.size() * sizeof(std::uint32_t),
				  .pData         = m_data.data()};
	}

	const VkSpecializationInfo* ChunkVertexSpecialization::info() const {
		return &m_info;
	}
}
