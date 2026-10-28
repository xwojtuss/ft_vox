export SDKROOT=$(xcrun --show-sdk-path)
export DYLD_FALLBACK_LIBRARY_PATH=$(brew --prefix)/lib
export VK_ICD_FILENAMES=$(brew --prefix)/etc/vulkan/icd.d/MoltenVK_icd.json
export VK_LAYER_PATH=$(brew --prefix)/etc/vulkan/explicit_layer.d