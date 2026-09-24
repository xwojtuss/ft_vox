FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG   fd79eadb5bc1760e7cbae12fd45b0d0040d1bb73 # v3.16.0
)

FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
include(Catch)

file(GLOB_RECURSE SCOP_TEST_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(scop_tests ${SCOP_TEST_SOURCES})

target_include_directories(scop_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(scop_tests PRIVATE
  scop_core
  Catch2::Catch2WithMain
)

catch_discover_tests(scop_tests DISCOVERY_MODE PRE_TEST)

set(SCOP_COVERAGE_DIR "${CMAKE_BINARY_DIR}/coverage")

if(NOT SCOP_COVERAGE)
  add_custom_target(coverage
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage is disabled in this build. Use: cmake --workflow --preset coverage"
    COMMAND ${CMAKE_COMMAND} -E false
    VERBATIM
  )
  return()
endif()

find_program(GCOVR_EXECUTABLE gcovr)
if(GCOVR_EXECUTABLE)
  set(GCOVR_COMMAND "${GCOVR_EXECUTABLE}")
else()
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_FOUND)
    execute_process(COMMAND "${Python3_EXECUTABLE}" -m gcovr --version
      RESULT_VARIABLE GCOVR_MODULE_RESULT OUTPUT_QUIET ERROR_QUIET)
    if(GCOVR_MODULE_RESULT EQUAL 0)
      set(GCOVR_COMMAND "${Python3_EXECUTABLE}" -m gcovr)
    endif()
  endif()
endif()

if(NOT GCOVR_COMMAND)
  message(FATAL_ERROR "SCOP_COVERAGE is ON but gcovr was not found. Install it with: python3 -m pip install --user gcovr")
endif()

string(REGEX MATCH "^[0-9]+" SCOP_COMPILER_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  find_program(SCOP_GCOV NAMES gcov-${SCOP_COMPILER_MAJOR} gcov REQUIRED)
  set(SCOP_GCOV_EXECUTABLE "${SCOP_GCOV}")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  set(SCOP_GCOV_EXECUTABLE "xcrun llvm-cov gcov")
else()
  find_program(SCOP_LLVM_COV NAMES llvm-cov-${SCOP_COMPILER_MAJOR} llvm-cov REQUIRED)
  set(SCOP_GCOV_EXECUTABLE "${SCOP_LLVM_COV} gcov")
endif()

string(REPLACE ";" "|" SCOP_GCOVR_COMMAND "${GCOVR_COMMAND}")
set(SCOP_GCOVR_ARGS
  --config "${PROJECT_SOURCE_DIR}/gcovr.cfg"
  --root "${PROJECT_SOURCE_DIR}"
  --gcov-executable "${SCOP_GCOV_EXECUTABLE}"
  --txt
  --html-details "${SCOP_COVERAGE_DIR}/index.html"
  "${CMAKE_BINARY_DIR}"
)
string(REPLACE ";" "|" SCOP_GCOVR_ARGS "${SCOP_GCOVR_ARGS}")

add_custom_target(coverage
  COMMAND ${CMAKE_COMMAND}
    -DBUILD_DIR=${CMAKE_BINARY_DIR}
    -DREPORT_DIR=${SCOP_COVERAGE_DIR}
    -DCTEST=${CMAKE_CTEST_COMMAND}
    "-DGCOVR=${SCOP_GCOVR_COMMAND}"
    "-DGCOVR_ARGS=${SCOP_GCOVR_ARGS}"
    -P "${PROJECT_SOURCE_DIR}/cmake/RunCoverage.cmake"
  DEPENDS scop_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running tests and measuring coverage"
  VERBATIM
  USES_TERMINAL
)
