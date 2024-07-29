

include(GrPybind)

########################################################################
# Python Bindings
########################################################################
list(
    APPEND
    aobb_python_files
    dort_islem_python.cc
    file_to_vector_python.cc
    vector_to_file_python.cc
    vector_to_file_v2_python.cc
    python_bindings.cc
    )



gr_pybind_make_check_hash(aobb ../../.. gr::aobb "${aobb_python_files}")



install(
    TARGETS aobb_python
    DESTINATION ${GR_PYTHON_DIR}/gnuradio/aobb
    COMPONENT pythonapi)
