# Copyright 2012-2014,2018-2019 Free Software Foundation, Inc.
#
# This file is part of GNU Radio
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Setup compatibility checks and defines
########################################################################
include(GrMiscUtils)
gr_check_hdr_n_def(io.h HAVE_IO_H)

########################################################################
# Setup library
########################################################################
set(aobb_blocks_SOURCES
    dort_islem_impl.cc
    file_to_vector_impl.cc
    vector_to_file_impl.cc
    vector_to_file_v2_impl.cc
    )



add_library(gnuradio-aobb ${aobb_blocks_SOURCES})

target_link_libraries(gnuradio-aobb PUBLIC gnuradio-runtime Volk::volk)



if(ENABLE_COMMON_PCH)
    target_link_libraries(gnuradio-aobb PRIVATE common-precompiled-headers)
endif()

target_include_directories(
    gnuradio-aobb PUBLIC $<INSTALL_INTERFACE:include>
                           $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)



# # TODO: aobb kütüphanesinin windows'ta çalışması için .rc dosyalarının ne olduğuna bak
# #Add Windows DLL resource file if using MSVC
# if(MSVC)
#     include(${PROJECT_SOURCE_DIR}/cmake/Modules/GrVersion.cmake)

#     configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gnuradio-aobb.rc.in
#                    ${CMAKE_CURRENT_BINARY_DIR}/gnuradio-aobb.rc @ONLY)

#     target_sources(gnuradio-aobb PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/gnuradio-aobb.rc)
# endif(MSVC)

if(BUILD_SHARED_LIBS)
    gr_library_foo(gnuradio-aobb)
endif()

# TODO: aobb kütüphanesi için test ekle
########################################################################
# QA C++ Code for gr-blocks
########################################################################
# if(ENABLE_TESTING)
#     include(GrTest)

#     list(
#         APPEND
#         test_gr_blocks_sources
#         qa_block_tags.cc
#         qa_gr_block.cc
#         qa_gr_flowgraph.cc
#         qa_gr_hier_block2.cc
#         qa_gr_hier_block2_derived.cc
#         qa_gr_top_block.cc
#         qa_rotator.cc
#         qa_set_msg_handler.cc)
#     list(APPEND GR_TEST_TARGET_DEPS gnuradio-aobb)

#     foreach(qa_file ${test_gr_blocks_sources})
#         gr_add_cpp_test("blocks_${qa_file}" ${CMAKE_CURRENT_SOURCE_DIR}/${qa_file})
#         if(NOT CMAKE_CROSSCOMPILING)
#             set_tests_properties("blocks_${qa_file}" PROPERTIES TIMEOUT 30)
#         endif(NOT CMAKE_CROSSCOMPILING)
#     endforeach(qa_file)

# endif(ENABLE_TESTING)
