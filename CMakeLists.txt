CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)
PROJECT(vtkansys)

SET (LIBRARY_OUTPUT_PATH ${vtkansys_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${vtkansys_BINARY_DIR}/bin CACHE INTERNAL
  "Single output directory for building all executables.")
MARK_AS_ADVANCED(LIBRARY_OUTPUT_PATH EXECUTABLE_OUTPUT_PATH)

FIND_PACKAGE(VTK REQUIRED)
INCLUDE(${VTK_USE_FILE})

SET(vtkansys_MAJOR_VERSION 0)
SET(vtkansys_MINOR_VERSION 1)
SET(vtkansys_BUILD_VERSION 0)

IF(EXISTS ${vtkansys_SOURCE_DIR}/.git)
  FIND_PACKAGE(Git)
  IF(GIT_FOUND)
    EXECUTE_PROCESS(
      COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
      WORKING_DIRECTORY "${vtkansys_SOURCE_DIR}"
      OUTPUT_VARIABLE "vtkansys_BUILD_VERSION"
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    MESSAGE( STATUS "Git version: ${vtkansys_BUILD_VERSION}" )
  ENDIF(GIT_FOUND)
ENDIF(EXISTS ${vtkansys_SOURCE_DIR}/.git)

SET(vtkansys_VERSION "${vtkansys_MAJOR_VERSION}.${vtkansys_MINOR_VERSION}.${vtkansys_BUILD_VERSION}")

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/vtkansysVersion.h.in ${CMAKE_CURRENT_BINARY_DIR}/vtkansysVersion.h @ONLY)

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
)

SET(vtkansysSrcs
  vtkvscAnsysCdbWriter.cxx
)

SET(vtkansysHdrs
  vtkvscAnsysCdbWriter.h
  vtkansysExport.h
  ${CMAKE_CURRENT_BINARY_DIR}/vtkansysVersion.h
)

SET (vtkansysLibs
  vtkIO
  vtkFiltering
)

ADD_LIBRARY(vtkansys SHARED ${vtkansysSrcs} ${vtkansysHdrs})
TARGET_LINK_LIBRARIES(vtkansys ${vtkansysLibs})

SET_TARGET_PROPERTIES(vtkansys PROPERTIES
  VERSION "${vtkansys_MAJOR_VERSION}.${vtkansys_MINOR_VERSION}"
  SOVERSION "${vtkansys_MAJOR_VERSION}.${vtkansys_MINOR_VERSION}"
)
