cmake_minimum_required(VERSION 3.16)
project(analyzeLumiHits_project)

find_package(EICrecon REQUIRED)
find_package(spdlog REQUIRED)

set(CMAKE_CXX_STANDARD ${EICrecon_CXX_STANDARD})

# Automatically determine source file list.
file(GLOB mysourcefiles *.cpp *.cc *.c  *.hpp *.hh *.h)
set( analyzeLumiHits_PLUGIN_SOURCES ${mysourcefiles} )

add_library(analyzeLumiHits_plugin SHARED ${analyzeLumiHits_PLUGIN_SOURCES})
target_link_libraries(analyzeLumiHits_plugin EICrecon::rootfile_plugin)
target_link_libraries(analyzeLumiHits_plugin ${JANA_LIB} ${ROOT_LIBRARIES} spdlog::spdlog)
set_target_properties(analyzeLumiHits_plugin PROPERTIES PREFIX "" OUTPUT_NAME "analyzeLumiHits" SUFFIX ".so")

install(TARGETS analyzeLumiHits_plugin DESTINATION ${USER_PLUGIN_OUTPUT_DIRECTORY})
