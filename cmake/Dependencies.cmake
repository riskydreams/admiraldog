# All third-party dependencies are resolved here, and only here.
# Every FetchContent_Declare pins an exact tag: no branch names.
include(FetchContent)

if(BUILD_TESTING)
  FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.5.3
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(doctest)
  # Provides doctest_discover_tests().
  include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
endif()
