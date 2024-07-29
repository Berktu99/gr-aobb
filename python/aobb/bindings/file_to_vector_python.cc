/*
 * Copyright 2020 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(file_to_vector.h)                                               */
/* BINDTOOL_HEADER_FILE_HASH(e76ee74a6a0c8438736fe84853abcc18)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnuradio/aobb/file_to_vector.h>


void bind_file_to_vector_template(py::module& m, const char* classname)
{
    using file_to_vector = gr::aobb::file_to_vector;

    py::class_<file_to_vector,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<file_to_vector>>(m, classname)
        .def(py::init(&file_to_vector::make),
             py::arg("itemsize"),
             py::arg("filename"),
             py::arg("repeat") = false,
             py::arg("offset") = 0,
             py::arg("len") = 0)


        .def("seek", &file_to_vector::seek, py::arg("seek_point"), py::arg("whence"))


        .def("open",
             &file_to_vector::open,
             py::arg("filename"),
             py::arg("repeat"),
             py::arg("offset") = 0,
             py::arg("len") = 0)


        .def("close", &file_to_vector::close)


        .def("set_begin_tag", &file_to_vector::set_begin_tag, py::arg("val"))

        ;
}

void bind_file_to_vector(py::module& m)
{
    bind_file_to_vector_template(m, "file_to_vector");
}
