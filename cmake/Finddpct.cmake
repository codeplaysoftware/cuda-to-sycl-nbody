if (WIN32)
    find_path( dpct_INCLUDE_DIR
        NAMES
            dpct/dpct.hpp
        PATHS
            ${dpct_LOCATION}/include
            $ENV{dpct_LOCATION}/include
            $ENV{DPCT_BUNDLE_ROOT}/include
            $ENV{ONEAPI_ROOT}/dpcpp-ct/latest/include
            $ENV{PROGRAMFILES}/include
            NO_DEFAULT_PATH
            DOC "The directory where dpct/dpct.hpp resides"
    )
else()
    find_path( dpct_INCLUDE_DIR
        NAMES
            dpct/dpct.hpp
        PATHS
            ${dpct_LOCATION}/include
            $ENV{dpct_LOCATION}/include
            $ENV{DPCT_BUNDLE_ROOT}/include
            $ENV{ONEAPI_ROOT}/dpcpp-ct/latest/include
            /opt/intel/oneapi/dpcpp-ct/latest/include
            /usr/include
            /usr/local/include
            /sw/include
            /opt/local/include
            NO_DEFAULT_PATH
            DOC "The directory where dpct/dpct.hpp resides"
    )
endif ()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args( dpct REQUIRED_VARS dpct_INCLUDE_DIR )

