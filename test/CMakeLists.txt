enable_testing()

message(STATUS "Configuring unit tests")

include(CMakeParseArguments)
include_directories(include)
include_directories(../src)

add_library(testlib STATIC test_main.cpp)

set(ALL_TESTS "")

# Otherwise GCC throws a lot of warnings for REQUIRE(...) from Catch v.1.2.1
if(CMAKE_COMPILER_IS_GNUCXX)
    add_definitions(-Wno-parentheses)
endif()


add_executable(test_role_order_check t/test_role_order_check.cpp ../src/ptv2_checker.cpp ../src/route_writer.cpp ../src/ogr_output_base.cpp)
target_compile_options(test_role_order_check PUBLIC "-DTEST_NO_ERROR_WRITING")
target_link_libraries(test_role_order_check testlib ${Boost_LIBRARIES} ${GDAL_LIBRARY} ${PROJ_LIBRARY})
add_test(NAME test_role_order_check
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_role_order_check)

add_executable(test_gap_detection t/test_gap_detection.cpp ../src/ptv2_checker.cpp ../src/route_writer.cpp ../src/ogr_output_base.cpp)
target_compile_options(test_gap_detection PUBLIC "-DTEST_NO_ERROR_WRITING")
target_link_libraries(test_gap_detection testlib ${Boost_LIBRARIES} ${GDAL_LIBRARY} ${PROJ_LIBRARY})
add_test(NAME test_gap_detection
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND test_gap_detection)
