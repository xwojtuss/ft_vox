cmake_minimum_required(VERSION 3.16)

project(scop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

option(SCOP_BUILD_TESTS "Build the Catch2 unit tests" ON)
option(SCOP_COVERAGE "Instrument scop_core and the tests for coverage (gcovr)" OFF)

file(GLOB_RECURSE SCOP_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*.cpp"
)
list(REMOVE_ITEM SCOP_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")

add_library(scop_core STATIC ${SCOP_SOURCES})

add_executable(scop "${CMAKE_CURRENT_SOURCE_DIR}/srcs/main.cpp")

target_compile_definitions(scop_core PUBLIC GLM_FORCE_CTOR_INIT NDEBUG
  PROJECT_ROOT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_options(scop_core PUBLIC -Wall -Wextra -Werror -O0 -g)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(scop_core PUBLIC -Wno-error=maybe-uninitialized)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  target_compile_options(scop_core PUBLIC -Wno-unknown-warning-option)
endif()

if(SCOP_COVERAGE)
  target_compile_options(scop_core PUBLIC --coverage)
  target_link_options(scop_core PUBLIC --coverage)
endif()

file(GLOB SRC_INCLUDE_DIR_CANDIDATES LIST_DIRECTORIES true "${CMAKE_CURRENT_SOURCE_DIR}/srcs/*")
set(SRC_INCLUDE_DIRS)
foreach(CANDIDATE IN LISTS SRC_INCLUDE_DIR_CANDIDATES)
  if(IS_DIRECTORY "${CANDIDATE}")
    list(APPEND SRC_INCLUDE_DIRS "${CANDIDATE}")
  endif()
endforeach()

target_include_directories(scop_core PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/srcs"
  ${SRC_INCLUDE_DIRS}
)

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
FetchContent_MakeAvailable(magic_enum)

target_sources(scop_core PRIVATE
  "${imgui_SOURCE_DIR}/imgui.cpp"
  "${imgui_SOURCE_DIR}/imgui_demo.cpp"
  "${imgui_SOURCE_DIR}/imgui_draw.cpp"
  "${imgui_SOURCE_DIR}/imgui_tables.cpp"
  "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp"
  "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
)

target_include_directories(scop_core PUBLIC
  "${imgui_SOURCE_DIR}"
  "${imgui_SOURCE_DIR}/backends"
  "${magic_enum_SOURCE_DIR}/include"
)

target_link_libraries(scop_core PUBLIC
  Vulkan::Vulkan
  glfw
  glm::glm
  magic_enum::magic_enum
)

target_link_libraries(scop PRIVATE scop_core)

if(SCOP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

find_program(GLSLANG_VALIDATOR glslangValidator)
if(NOT GLSLANG_VALIDATOR)
  message(FATAL_ERROR "glslangValidator not found. Install it to compile shaders.")
endif()

file(GLOB FRAG_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag")
file(GLOB VERT_SHADERS "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert")
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
add_dependencies(scop shaders)

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
