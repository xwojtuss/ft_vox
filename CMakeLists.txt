cmake_minimum_required(VERSION 3.25)

project(ft_vox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
else()
  message(STATUS "ccache not found, compiling without compiler cache")
endif()

find_program(MOLD_PROGRAM mold)
if(MOLD_PROGRAM)
  add_link_options(-fuse-ld=mold)
  message(STATUS "Using mold linker: ${MOLD_PROGRAM}")
else()
  message(STATUS "mold not found, using default linker")
endif()

option(FT_VOX_BUILD_TESTS "Build the Catch2 unit tests" ON)
option(FT_VOX_COVERAGE "Instrument the tests and the code they run for coverage (gcovr)" OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(FT_VOX_VALIDATION_DEFAULT OFF)
else()
  set(FT_VOX_VALIDATION_DEFAULT ON)
endif()
option(FT_VOX_VALIDATION "Enable the Vulkan validation layers (on by default, except in Release builds)" ${FT_VOX_VALIDATION_DEFAULT})
option(FT_VOX_LTO "Link-time optimization in Release builds" ON)
option(FT_VOX_PROFILER "Build with the Tracy frame profiler; every profiling macro compiles to nothing when OFF" OFF)
option(FT_VOX_CLANG_TIDY "Run clang-tidy on our sources while compiling; any finding fails the build" OFF)

find_program(PRE_COMMIT_PROGRAM pre-commit)
if(PRE_COMMIT_PROGRAM AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  execute_process(COMMAND "${PRE_COMMIT_PROGRAM}" install --overwrite
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" OUTPUT_QUIET)
  message(STATUS "pre-commit hook installed (formats staged C++ files on commit)")
else()
  message(STATUS "pre-commit not found, commits are not formatted automatically (install: pipx install pre-commit)")
endif()

set(FT_VOX_CLANG_TIDY_MAJOR 21)
if(FT_VOX_CLANG_TIDY)
  find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy-${FT_VOX_CLANG_TIDY_MAJOR} clang-tidy REQUIRED)
  execute_process(COMMAND "${CLANG_TIDY_PROGRAM}" --version OUTPUT_VARIABLE CLANG_TIDY_VERSION)
  if(NOT CLANG_TIDY_VERSION MATCHES "version ${FT_VOX_CLANG_TIDY_MAJOR}\\.")
    message(FATAL_ERROR "${CLANG_TIDY_PROGRAM} is not clang-tidy ${FT_VOX_CLANG_TIDY_MAJOR}, which CI uses. "
      "Install it with: pipx install clang-tidy==${FT_VOX_CLANG_TIDY_MAJOR}.1.6")
  endif()
  set(FT_VOX_CLANG_TIDY_COMMAND "${CLANG_TIDY_PROGRAM}" --quiet --warnings-as-errors=*)
  if(APPLE)
    execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE MACOS_SDK OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND xcrun clang -print-resource-dir OUTPUT_VARIABLE CLANG_RESOURCE_DIR
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND FT_VOX_CLANG_TIDY_COMMAND --extra-arg=-isysroot${MACOS_SDK}
      --extra-arg=-resource-dir=${CLANG_RESOURCE_DIR} --extra-arg=-isystem/usr/local/include)
  endif()
  message(STATUS "clang-tidy enabled: ${CLANG_TIDY_PROGRAM}")
endif()

function(ft_vox_enable_clang_tidy target)
  if(FT_VOX_CLANG_TIDY)
    set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${FT_VOX_CLANG_TIDY_COMMAND}")
  endif()
endfunction()

file(GLOB_RECURSE FT_VOX_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)
list(REMOVE_ITEM FT_VOX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")

include(FetchContent)

find_package(Vulkan REQUIRED)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG   7b6aead9fb88b3623e3b3725ebb42670cbe4c579
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG   0af55ccecd98d4e5a8d1fad7de25ba429d60e863
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG   6ded5230d043aa32c755e65c910c2af5002fb9f9
)

FetchContent_Declare(
  magic_enum
  GIT_REPOSITORY https://github.com/Neargye/magic_enum.git
  GIT_TAG   e046b69a3736d314fad813e159b1c192eaef92cd
)

FetchContent_MakeAvailable(glfw)
FetchContent_MakeAvailable(glm)
FetchContent_MakeAvailable(imgui)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG   79524ddd08a4ec981b7fea76afd08ee05f83755d # v1.17.0
)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG   55f93686c01528224f448c19128836e7df245f72 # v3.12.0
)

FetchContent_Declare(
  EnTT
  GIT_REPOSITORY https://github.com/skypjack/entt.git
  GIT_TAG   b4e58bdd364ad72246c123a0c28538eab3252672 # v3.16.0
)

FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG   776893306c5d3b22b6185b5d4a258b81d94572bf # vulkan-sdk-1.4.357.0
  SYSTEM
)

FetchContent_Declare(
  VulkanMemoryAllocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG   3aa921224c154a0d2c43912bc88e1c42ce1f7607 # v3.4.0
  SYSTEM
)

FetchContent_Declare(
  libassert
  GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert.git
  GIT_TAG   bd33ba116f209bf71761c58dccc2f3bf277e0824 # v2.2.1
  SYSTEM
)

if (FT_VOX_PROFILER)
  FetchContent_Declare(
          Tracy
          GIT_REPOSITORY https://github.com/wolfpld/tracy.git
          GIT_TAG v0.13.1
          GIT_SHALLOW TRUE
          SYSTEM
  )
  set(TRACY_ENABLE ON CACHE BOOL "" FORCE)
  set(TRACY_ON_DEMAND OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(Tracy)
  message(STATUS "Tracy profiler enabled (the viewer must be v0.13.1)")
endif ()

FetchContent_Declare(
  bs_thread_pool
  GIT_REPOSITORY https://github.com/bshoshany/thread-pool.git
  GIT_TAG   bd4533f1f70c2b975cbd5769a60d8eaaea1d2233 # v5.1.0
  SYSTEM
)

FetchContent_Declare(
  FastNoise2
  GIT_REPOSITORY https://github.com/Auburn/FastNoise2.git
  GIT_TAG   903c1f2d2f9d53ddce94cd223f32727d9ab3aeaa # v1.1.1
  SYSTEM
)

set(FASTNOISE2_TOOLS OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_TESTS OFF CACHE BOOL "" FORCE)
set(FASTNOISE2_UTILITY OFF CACHE BOOL "" FORCE)
set(FT_VOX_CXX_FLAGS_DEBUG_BACKUP "${CMAKE_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O2")
FetchContent_MakeAvailable(FastNoise2)
set(CMAKE_CXX_FLAGS_DEBUG "${FT_VOX_CXX_FLAGS_DEBUG_BACKUP}")
FetchContent_MakeAvailable(bs_thread_pool)
FetchContent_MakeAvailable(magic_enum)
FetchContent_MakeAvailable(libassert)
FetchContent_MakeAvailable(volk)
FetchContent_MakeAvailable(VulkanMemoryAllocator)
FetchContent_MakeAvailable(EnTT)
set(SPDLOG_USE_STD_FORMAT ON CACHE BOOL "Use std::format instead of the bundled fmt" FORCE)
FetchContent_MakeAvailable(spdlog)
FetchContent_MakeAvailable(nlohmann_json)

set(IMGUI_SOURCES
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
)
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES SKIP_LINTING ON)

function(ft_vox_add_core target optimization)
  add_library(${target} STATIC ${FT_VOX_SOURCES} ${IMGUI_SOURCES})

  if(FT_VOX_VALIDATION)
    target_compile_definitions(${target} PUBLIC FT_VOX_VALIDATION_LAYERS)
  endif()

  target_compile_definitions(${target} PUBLIC
    GLM_FORCE_CTOR_INIT
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    GLFW_INCLUDE_NONE
    VK_NO_PROTOTYPES
    VMA_STATIC_VULKAN_FUNCTIONS=0
    VMA_DYNAMIC_VULKAN_FUNCTIONS=1
    IMGUI_IMPL_VULKAN_USE_VOLK
    PROJECT_ROOT_DIR="${PROJECT_SOURCE_DIR}"
  )

  if (FT_VOX_PROFILER)
    target_compile_definitions(${target} PUBLIC FT_VOX_PROFILER)
    target_link_libraries(${target} PUBLIC Tracy::TracyClient)
  endif ()

  target_compile_options(${target} PUBLIC -Wall -Wextra -Werror ${optimization} $<$<NOT:$<CONFIG:Release>>:-g>)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(${target} PUBLIC -Wno-error=maybe-uninitialized)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${target} PUBLIC -Wno-unknown-warning-option -Wmissing-designated-field-initializers)
  endif()

  target_include_directories(${target} PUBLIC
    "${PROJECT_SOURCE_DIR}/srcs"
    "${imgui_SOURCE_DIR}"
    "${imgui_SOURCE_DIR}/backends"
    "${magic_enum_SOURCE_DIR}/include"
  )
  target_include_directories(${target} SYSTEM PRIVATE "${bs_thread_pool_SOURCE_DIR}/include")

  target_link_libraries(${target} PUBLIC
    Vulkan::Headers
    volk::volk
    GPUOpen::VulkanMemoryAllocator
    glfw
    glm::glm
    magic_enum::magic_enum
    EnTT::EnTT
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    libassert::assert
    FastNoise2
  )
endfunction()

file(GLOB_RECURSE FT_VOX_CODE_FILES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.hpp" "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.tpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.hpp"
)

function(ft_vox_restrict_include library include_regex allowed_folder hint)
  foreach(FILE IN LISTS FT_VOX_CODE_FILES)
    if(FILE MATCHES "/${allowed_folder}/")
      continue()
    endif()
    file(STRINGS "${FILE}" FORBIDDEN_INCLUDES REGEX "${include_regex}")
    if(FORBIDDEN_INCLUDES)
      message(FATAL_ERROR "${FILE} includes ${library} directly; only ${allowed_folder} may, ${hint}")
    endif()
  endforeach()
endfunction()

ft_vox_restrict_include("EnTT" "#include [<\"]entt/" "srcs/ecs" "everything else uses the ecs API")
ft_vox_restrict_include("FastNoise2" "#include [<\"]FastNoise/" "srcs/noise" "everything else uses noise::INoise2D")
ft_vox_restrict_include("BS::thread_pool" "#include [<\"]BS_thread_pool" "srcs/concurrency"
  "everything else uses concurrency::IThreadPool")

ft_vox_add_core(ft_vox_core $<IF:$<CONFIG:Release>,-O3,-O2>)
ft_vox_enable_clang_tidy(ft_vox_core)

add_executable(ft_vox "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")
target_link_libraries(ft_vox PRIVATE ft_vox_core)

include(CheckIPOSupported)
check_ipo_supported(RESULT FT_VOX_IPO_SUPPORTED OUTPUT FT_VOX_IPO_MESSAGE LANGUAGES CXX)
if(FT_VOX_LTO AND FT_VOX_IPO_SUPPORTED)
  set_target_properties(ft_vox_core ft_vox PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
else()
  message(STATUS "Link-time optimization is not available for Release builds: ${FT_VOX_IPO_MESSAGE}")
endif()
ft_vox_enable_clang_tidy(ft_vox)

if(FT_VOX_BUILD_TESTS)
  ft_vox_add_core(ft_vox_core_tests -O0)

  if(FT_VOX_COVERAGE)
    target_compile_options(ft_vox_core_tests PUBLIC --coverage)
    target_link_options(ft_vox_core_tests PUBLIC --coverage)
  endif()

  enable_testing()
  add_subdirectory(tests)
endif()

find_program(GLSLANG_VALIDATOR glslangValidator)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found. Install it to compile shaders.")
endif()

file(GLOB FRAG_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")
file(GLOB VERT_SHADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert")
set(SHADER_SOURCES ${FRAG_SHADERS} ${VERT_SHADERS})
set(SHADER_OUTPUTS)

foreach(SHADER_FILE IN LISTS SHADER_SOURCES)
  set(SPV_FILE "${SHADER_FILE}.spv")
  add_custom_command(
    OUTPUT "${SPV_FILE}"
    COMMAND "${GLSLANG_VALIDATOR}" -V -s "${SHADER_FILE}" -o "${SPV_FILE}"
    DEPENDS "${SHADER_FILE}"
    COMMENT "Compiling shader ${SHADER_FILE}"
    VERBATIM
  )
  list(APPEND SHADER_OUTPUTS "${SPV_FILE}")
endforeach()

add_custom_target(shaders ALL DEPENDS ${SHADER_OUTPUTS})
add_dependencies(ft_vox shaders)

set(DOCS_INDEX_HTML "${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/html/index.html")

if(WIN32)
  set(OPEN_DOCS_COMMAND cmd /c start "" "${DOCS_INDEX_HTML}")
elseif(APPLE)
  set(OPEN_DOCS_COMMAND open "${DOCS_INDEX_HTML}")
else()
  set(OPEN_DOCS_COMMAND xdg-open "${DOCS_INDEX_HTML}")
endif()

add_custom_target(generate_docs
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/docs/doxygen/generateDoxygen.sh"
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/docs/plantuml/generatePlantuml.sh"
  COMMAND ${OPEN_DOCS_COMMAND}
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  COMMENT "Generating documentation"
)
