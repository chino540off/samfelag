add_executable(samfelag-tests
  tests_net.cc
)

#add_dependencies(samfelag-tests googletest)

target_include_directories(samfelag-tests
  PUBLIC
    ${GTEST_INCLUDE_DIRS}
    ${SAMFELAG_INCLUDE_DIRS}
    ${BOKASAFN_INCLUDE_DIRS}
)

set_target_properties(samfelag-tests
  PROPERTIES
    LINK_FLAGS "-static"
)

target_link_libraries(samfelag-tests
  ${GTEST_LIBS}
  ${CMAKE_THREAD_LIBS_INIT}
)

add_test(NAME samfelag-unit-tests COMMAND ./samfelag-tests)

#if(SAMFELAG_BUILD_COVERAGE)
#set(COVERAGE_LCOV_EXTRACT
#  '*/samfelag/include/*'
#)
#
#setup_TARGET_FOR_COVERAGE_LCOV(
#    NAME coverage
#    EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/samfelag-tests
#    DEPENDENCIES samfelag-tests
#)
#endif()
