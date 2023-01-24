cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(DARWIN "Darwin")
set(LINUX "Linux")
math(EXPR BITS "8*${CMAKE_SIZEOF_VOID_P}")
if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    message(STATUS "Compiling for Darwin...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    SET(CMAKE_MACOSX_RPATH 1)
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        SET(KDU_MAKE "Makefile-MAC-x86-${BITS}-gcc")
        SET(KDU_ARCH "Mac-x86-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Mac-x86-64-gcc")
    else()
        SET(KDU_MAKE "Makefile-MAC-arm-${BITS}-gcc")
        SET(KDU_ARCH "Mac-arm-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Mac-arm-64-gcc")
    endif()
    SET(LUA_ARCH "macosx")
elseif(CMAKE_SYSTEM_NAME STREQUAL LINUX)
    message(STATUS "Compiling for Linux64...")
    message(STATUS ${CMAKE_SYSTEM_NAME})
    execute_process(COMMAND /usr/bin/lsb_release -si COMMAND tr -d '\n' OUTPUT_VARIABLE CMAKE_DISTRO_NAME)
    MESSAGE(STATUS "Distro: " ${CMAKE_DISTRO_NAME})
    if (${CMAKE_HOST_SYSTEM_PROCESSOR} MATCHES "x86_64")
        SET(KDU_MAKE "Makefile-Linux-x86-${BITS}-gcc")
        SET(KDU_ARCH "Linux-x86-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Linux-x86-64-gcc")
    else()
        SET(KDU_MAKE "Makefile-Linux-arm-${BITS}-gcc")
        SET(KDU_ARCH "Linux-arm-${BITS}-gcc")
        SET(KDU_EXEC_PLATFORM "Linux-arm-64-gcc")

    endif()
    SET(LUA_ARCH "linux")
endif()

include_directories(
        ${CMAKE_SOURCE_DIR}/lib
        ${COMMON_LOCAL}/include
        /usr/local/include
)

add_executable (iiifparser_tests test_iiifparser.cpp
        ../IIIFError.cpp ../IIIFError.h
        ../iiifparser/IIIFRegion.cpp ../iiifparser/IIIFRegion.h
        ../iiifparser/IIIFSize.cpp ../iiifparser/IIIFSize.h
        ../iiifparser/IIIFRotation.cpp ../iiifparser/IIIFRotation.h
        ../iiifparser/IIIFQualityFormat.cpp ../iiifparser/IIIFQualityFormat.h
        ../iiifparser/IIIFIdentifier.cpp ../iiifparser/IIIFIdentifier.h)

target_link_libraries(iiifparser_tests PRIVATE
        cserve
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        webp
        lerc
        jbigkit
        png
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        sharpyuv
        deflate
        Threads::Threads
        ${CMAKE_DL_LIBS})

add_test(NAME iiifparser_tests COMMAND iiifparser_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

add_executable (metadata_tests test_metadata.cpp
        ../IIIFError.cpp ../IIIFError.h
        ../metadata/IIIFEssentials.cpp ../metadata/IIIFEssentials.h
        ../metadata/IIIFExif.cpp ../metadata/IIIFExif.h
        ../metadata/IIIFIptc.cpp ../metadata/IIIFIptc.h
        ../metadata/IIIFIcc.cpp ../metadata/IIIFIcc.h
        ../metadata/IIIFXmp.cpp ../metadata/IIIFXmp.h)

target_link_libraries(metadata_tests PRIVATE
        cserve
        #Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        sharpyuv
        deflate
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(metadata_tests PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(metadata_tests lcms2 rt)
endif()

add_test(NAME metadata_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} COMMAND metadata_tests)


add_library(iiiflib STATIC
        ../IIIFError.cpp ../IIIFError.h
        ../IIIFIO.h
        ../IIIFImage.cpp ../IIIFImage.h
        ../IIIFImgTools.cpp ../IIIFImgTools.h
        ../iiifparser/IIIFIdentifier.cpp ../iiifparser/IIIFIdentifier.h
        ../iiifparser/IIIFQualityFormat.cpp ../iiifparser/IIIFQualityFormat.h
        ../iiifparser/IIIFRegion.cpp ../iiifparser/IIIFRegion.h
        ../iiifparser/IIIFRotation.cpp ../iiifparser/IIIFRotation.h
        ../iiifparser/IIIFSize.cpp ../iiifparser/IIIFSize.h
        ../metadata/IIIFEssentials.cpp ../metadata/IIIFEssentials.h
        ../metadata/IIIFExif.cpp ../metadata/IIIFExif.h
        ../metadata/IIIFIcc.cpp ../metadata/IIIFIcc.h
        ../metadata/IIIFIptc.cpp ../metadata/IIIFIptc.h
        ../metadata/IIIFXmp.cpp ../metadata/IIIFXmp.h
        ../imgformats/IIIFIOTiff.cpp ../imgformats/IIIFIOTiff.h
        ../imgformats/IIIFIOPng.cpp ../imgformats/IIIFIOPng.h
        ../imgformats/IIIFIOJpeg.cpp ../imgformats/IIIFIOJpeg.h
        ../imgformats/IIIFIOJ2k.cpp ../imgformats/IIIFIOJ2k.h
        ../IIIFPhotometricInterpretation.h ../IIIFGetCanonicalUrl.cpp)

add_executable (tiff_tests test_tiff_format.cpp)

target_link_libraries(tiff_tests PRIVATE
        cserve
        iiiflib
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        sharpyuv
        deflate
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(tiff_tests PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(tiff_tests lcms2 rt)
endif()

add_test(NAME tiff_tests COMMAND tiff_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#------------------------------------------------------------

add_executable (png_tests test_png_format.cpp
)

target_link_libraries(png_tests PRIVATE
        cserve
        iiiflib
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        sharpyuv
        deflate
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(png_tests PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(png_tests lcms2 rt)
endif()

add_test(NAME png_tests COMMAND png_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#------------------------------------------------------------

add_executable (jpeg_tests test_jpeg_format.cpp)

target_link_libraries(jpeg_tests PRIVATE
        cserve
        iiiflib
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        deflate
        sharpyuv
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(jpeg_tests PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(jpeg_tests lcms2 rt)
endif()

add_test(NAME jpeg_tests COMMAND jpeg_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

#------------------------------------------------------------

add_executable (j2k_tests test_j2k_format.cpp)

target_link_libraries(j2k_tests PRIVATE
        cserve
        iiiflib
        Catch2Main
        Catch2
        fmt
        magic
        lua
        sqlite3
        jwtcpp
        spdlog
        curl
        ssl
        crypto
        zlib
        xz
        bzip2
        exiv2
        tiff
        turbojpeg
        png
        webp
        lerc
        jbigkit
        kdu
        kdu_aux
        expat
        lcms2
        iconv
        gettext_intl
        zlib
        zstd
        deflate
        sharpyuv
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(j2k_tests PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(j2k_tests lcms2 rt)
endif()

add_test(NAME j2k_tests COMMAND j2k_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

enable_testing()

