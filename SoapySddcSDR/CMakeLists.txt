cmake_minimum_required(VERSION 3.13)

if (MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
endif (MSVC)

include_directories("." "../Core")

file(GLOB SRC "*.cpp")


########################################################################
# build the module
########################################################################
find_package(SoapySDR CONFIG)

if (NOT SoapySDR_FOUND)
    message(WARNING "SoapySDR development files not found - skipping support")
    return()
endif ()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG ")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused")

SOAPY_SDR_MODULE_UTIL(
    TARGET SoapySddcSDR
    SOURCES Registration.cpp Settings.cpp Streaming.cpp r2iqBasicControlClass.cpp
    LIBRARIES sddc ${MY_DEVICE_LIBRARIES}
)

if (MSVC)
  target_link_libraries(SoapySddcSDR PUBLIC Setupapi.lib)
else()
  target_include_directories(SoapySddcSDR PUBLIC "${LIBUSB_INCLUDE_DIR}")
  target_link_directories(SoapySddcSDR PUBLIC "${LIBUSB_LIBRARY_DIRS}")
  target_link_libraries(SoapySddcSDR PUBLIC ${LIBUSB_LIBRARIES})
endif (MSVC)

target_include_directories(SoapySddcSDR PUBLIC "${LIBFFTW_INCLUDE_DIR}")
target_link_directories(SoapySddcSDR PUBLIC "${LIBFFTW_LIBRARY_DIRS}")
target_link_libraries(SoapySddcSDR PUBLIC ${LIBFFTW_LIBRARIES})

target_link_libraries(SoapySddcSDR PRIVATE SDDC_CORE)
set_target_properties(SoapySddcSDR PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(SoapySddcSDR PROPERTIES SOVERSION 0)



set_target_properties(SoapySddcSDR PROPERTIES POSITION_INDEPENDENT_CODE True)
target_compile_definitions(SoapySddcSDR PUBLIC _CRT_SECURE_NO_WARNINGS)