# Removes .gcda files so every coverage run starts from zero
# Usage: cmake -DDIR=<build dir> -P CleanCoverageData.cmake
file(GLOB_RECURSE SCOP_GCDA_FILES "${DIR}/*.gcda")
if(SCOP_GCDA_FILES)
  file(REMOVE ${SCOP_GCDA_FILES})
endif()
