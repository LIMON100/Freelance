# - Try to find FFTW3
# Once done this will define
#
#  FFTW3_FOUND - system has FFTW3
#  FFTW3_INCLUDE_DIRS - the FFTW3 include directory
#  FFTW3_LIBRARIES - Link these to use FFTW3
#
#  And for the float version (fftw3f):
#  FFTW3f_LIBRARIES - Link these to use the float version of FFTW3
#
# Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021, 2022
# Laurent de Soras <laurent.desoras@club-fr>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS

if (FFTW3_INCLUDE_DIRS AND FFTW3_LIBRARIES)
  set(FFTW3_FIND_QUIETLY TRUE)
endif ()

find_path(FFTW3_INCLUDE_DIR fftw3.h)

find_library(FFTW3_LIBRARY NAMES fftw3)
find_library(FFTW3f_LIBRARY NAMES fftw3f)

set(FFTW3_INCLUDE_DIRS ${FFTW3_INCLUDE_DIR})
set(FFTW3_LIBRARIES ${FFTW3_LIBRARY})
set(FFTW3f_LIBRARIES ${FFTW3f_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFTW3 DEFAULT_MSG FFTW3_LIBRARIES FFTW3_INCLUDE_DIRS)

mark_as_advanced(FFTW3_INCLUDE_DIRS FFTW3_LIBRARIES FFTW3f_LIBRARIES)