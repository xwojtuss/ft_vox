string(REPLACE "|" ";" GCOVR "${GCOVR}")
string(REPLACE "|" ";" GCOVR_ARGS "${GCOVR_ARGS}")

file(GLOB_RECURSE FT_VOX_GCDA_FILES "${BUILD_DIR}/*.gcda")
if(FT_VOX_GCDA_FILES)
  file(REMOVE ${FT_VOX_GCDA_FILES})
endif()

cmake_host_system_information(RESULT FT_VOX_CORES QUERY NUMBER_OF_LOGICAL_CORES)

execute_process(
  COMMAND "${CTEST}" --test-dir "${BUILD_DIR}" --output-on-failure --parallel ${FT_VOX_CORES}
  RESULT_VARIABLE FT_VOX_TESTS_RESULT
)

file(MAKE_DIRECTORY "${REPORT_DIR}")
message("HTML report: ${REPORT_DIR}/index.html")

execute_process(
  COMMAND ${GCOVR} ${GCOVR_ARGS}
  WORKING_DIRECTORY "${BUILD_DIR}"
  RESULT_VARIABLE FT_VOX_COVERAGE_RESULT
)

if(NOT FT_VOX_TESTS_RESULT EQUAL 0 AND NOT FT_VOX_COVERAGE_RESULT EQUAL 0)
  message(FATAL_ERROR "Some tests failed and coverage is below the threshold")
elseif(NOT FT_VOX_TESTS_RESULT EQUAL 0)
  message(FATAL_ERROR "Some tests failed (coverage threshold met)")
elseif(NOT FT_VOX_COVERAGE_RESULT EQUAL 0)
  message(FATAL_ERROR "Coverage is below the threshold")
endif()
