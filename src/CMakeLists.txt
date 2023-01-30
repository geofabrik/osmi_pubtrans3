#-----------------------------------------------------------------------------
#
#  CMake Config
#
#-----------------------------------------------------------------------------

add_executable(osmi_pubtrans3 osmi_pubtrans3.cpp ogr_writer.cpp ogr_output_base.cpp railway_handler_pass1.cpp railway_handler_pass2.cpp turn_restriction_handler.cpp route_manager.cpp route_writer.cpp ptv2_checker.cpp)
target_link_libraries(osmi_pubtrans3 ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS osmi_pubtrans3 DESTINATION bin)

add_executable(osmi_pubtrans3_merc osmi_pubtrans3.cpp ogr_writer.cpp ogr_output_base.cpp railway_handler_pass1.cpp railway_handler_pass2.cpp turn_restriction_handler.cpp route_manager.cpp route_writer.cpp ptv2_checker.cpp)
target_compile_options(osmi_pubtrans3_merc PUBLIC "-DMERCATOR_OUTPUT")
target_link_libraries(osmi_pubtrans3_merc ${OSMIUM_LIBRARIES} ${Boost_LIBRARIES})
install(TARGETS osmi_pubtrans3_merc DESTINATION bin)
