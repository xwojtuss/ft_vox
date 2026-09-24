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

target_link_libraries(scop_tests PRIVATE
  scop_core
  Catch2::Catch2WithMain
)

# Every SCENARIO becomes its own CTest test
catch_discover_tests(scop_tests DISCOVERY_MODE PRE_TEST)

# ---------------------------------------------------------------------------
# Coverage: `cmake --build --preset coverage` runs every test, prints a
# per-file report, writes HTML and fails if line coverage is below the
# threshold in gcovr.cfg
# ---------------------------------------------------------------------------
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
  # `pip install --user gcovr` does not always put gcovr on PATH
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

# gcovr needs the gcov flavour that matches the compiler
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

add_custom_target(coverage
  COMMAND ${CMAKE_COMMAND} -DDIR=${CMAKE_BINARY_DIR} -P "${PROJECT_SOURCE_DIR}/cmake/CleanCoverageData.cmake"
  COMMAND ${CMAKE_CTEST_COMMAND} --test-dir "${CMAKE_BINARY_DIR}" --output-on-failure
  COMMAND ${CMAKE_COMMAND} -E make_directory "${SCOP_COVERAGE_DIR}"
  COMMAND ${CMAKE_COMMAND} -E echo "HTML report: ${SCOP_COVERAGE_DIR}/index.html"
  COMMAND ${GCOVR_COMMAND}
    --config "${PROJECT_SOURCE_DIR}/gcovr.cfg"
    --root "${PROJECT_SOURCE_DIR}"
    --gcov-executable "${SCOP_GCOV_EXECUTABLE}"
    --txt
    --html-details "${SCOP_COVERAGE_DIR}/index.html"
    "${CMAKE_BINARY_DIR}"
  DEPENDS scop_tests
  WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
  COMMENT "Running tests and measuring coverage"
  VERBATIM
  USES_TERMINAL
)
