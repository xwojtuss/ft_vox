# Runs every test, then always writes the coverage report, then fails if
# either a test failed or coverage is below the threshold in gcovr.cfg.
# Usage: cmake -DBUILD_DIR=... -DREPORT_DIR=... -DCTEST=... -DGCOVR="cmd|args" -DGCOVR_ARGS="a|b" -P RunCoverage.cmake

string(REPLACE "|" ";" GCOVR "${GCOVR}")
string(REPLACE "|" ";" GCOVR_ARGS "${GCOVR_ARGS}")

file(GLOB_RECURSE SCOP_GCDA_FILES "${BUILD_DIR}/*.gcda")
if(SCOP_GCDA_FILES)
  file(REMOVE ${SCOP_GCDA_FILES})
endif()

execute_process(
  COMMAND "${CTEST}" --test-dir "${BUILD_DIR}" --output-on-failure
  RESULT_VARIABLE SCOP_TESTS_RESULT
)

file(MAKE_DIRECTORY "${REPORT_DIR}")
message("HTML report: ${REPORT_DIR}/index.html")

execute_process(
  COMMAND ${GCOVR} ${GCOVR_ARGS}
  WORKING_DIRECTORY "${BUILD_DIR}"
  RESULT_VARIABLE SCOP_COVERAGE_RESULT
)

if(NOT SCOP_TESTS_RESULT EQUAL 0 AND NOT SCOP_COVERAGE_RESULT EQUAL 0)
  message(FATAL_ERROR "Some tests failed and coverage is below the threshold")
elseif(NOT SCOP_TESTS_RESULT EQUAL 0)
  message(FATAL_ERROR "Some tests failed (coverage threshold met)")
elseif(NOT SCOP_COVERAGE_RESULT EQUAL 0)
  message(FATAL_ERROR "Coverage is below the threshold")
endif()
