#pragma once

#include <array>
#include <cstdint> // for std::uint32_t
#include <vector>
#include <volk.h>

namespace render::vulkan {
	[[nodiscard]] VkVertexInputBindingDescription                  getBindingDescription();
	[[nodiscard]] std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	[[nodiscard]] VkVertexInputBindingDescription   getChunkBindingDescription();
	[[nodiscard]] VkVertexInputAttributeDescription getChunkAttributeDescription();

	/** Hands the chunk vertex layout (ChunkVertex.hpp) to the chunk vertex shader as specialization constants. */
	class ChunkVertexSpecialization {
	private:
		std::vector<std::uint32_t>            m_data;
		std::vector<VkSpecializationMapEntry> m_entries;
		VkSpecializationInfo                  m_info{};

	public:
		ChunkVertexSpecialization();
		ChunkVertexSpecialization(const ChunkVertexSpecialization&)            = delete;
		ChunkVertexSpecialization& operator=(const ChunkVertexSpecialization&) = delete;
		ChunkVertexSpecialization(ChunkVertexSpecialization&&)                 = delete;
		ChunkVertexSpecialization& operator=(ChunkVertexSpecialization&&)      = delete;
		~ChunkVertexSpecialization()                                           = default;

		[[nodiscard]] const VkSpecializationInfo* info() const;
	};
}
