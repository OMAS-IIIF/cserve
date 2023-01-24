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
    # ToDo: shouldn't this be also set on the mac through FindOpenSSL?
    #SET(OPENSSL_ROOT_DIR "/usr/local/opt/openssl" CACHE FILEPATH "openssl")
    #SET(OPENSSL_USE_STATIC_LIBS ON)
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
message(STATUS "Compiling for ${KDU_ARCH}")

add_compile_options(-fvisibility=default -DCMS_NO_REGISTER_KEYWORD=1)

set(IIIF_HANDLER_LIBDIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_SYSTEM_NAME STREQUAL DARWIN)
    find_package(Iconv REQUIRED)
endif()

set(GENERATED_ICC_HEADERS
        ${IIIF_HANDLER_LIBDIR}/AdobeRGB1998_icc.h
        ${IIIF_HANDLER_LIBDIR}/USWebCoatedSWOP_icc.h
        ${IIIF_HANDLER_LIBDIR}/YCC709_icc.h
        )

add_custom_command(
        OUTPUT ${GENERATED_ICC_HEADERS}
        COMMENT "Generating ICC profile includes"
        WORKING_DIRECTORY ${IIIF_HANDLER_LIBDIR}
        COMMAND cp ${IIIF_HANDLER_LIBDIR}/ICC-Profiles/AdobeRGB1998.icc ${IIIF_HANDLER_LIBDIR}/AdobeRGB1998.icc
        COMMAND xxd -i AdobeRGB1998.icc AdobeRGB1998_icc.h
        COMMAND rm ${IIIF_HANDLER_LIBDIR}/AdobeRGB1998.icc
        COMMAND cp ${IIIF_HANDLER_LIBDIR}/ICC-Profiles/USWebCoatedSWOP.icc ${IIIF_HANDLER_LIBDIR}/USWebCoatedSWOP.icc
        COMMAND xxd -i USWebCoatedSWOP.icc USWebCoatedSWOP_icc.h
        COMMAND rm ${IIIF_HANDLER_LIBDIR}/USWebCoatedSWOP.icc
        COMMAND cp ${IIIF_HANDLER_LIBDIR}/ICC-Profiles/YCC709.icm ${IIIF_HANDLER_LIBDIR}/YCC709.icm
        COMMAND xxd -i YCC709.icm YCC709_icc.h
        COMMAND rm ${IIIF_HANDLER_LIBDIR}/YCC709.icm
        COMMAND cp ${IIIF_HANDLER_LIBDIR}/ICC-Profiles/Rec709-Rec1886.icc ${IIIF_HANDLER_LIBDIR}/Rec709-Rec1886.icc
        COMMAND xxd -i Rec709-Rec1886.icc Rec709-Rec1886_icc.h
        COMMAND rm ${IIIF_HANDLER_LIBDIR}/Rec709-Rec1886.icc
)

add_custom_target(icc_profiles
        DEPENDS ${GENERATED_ICC_HEADERS}
        )


add_subdirectory(ext/lerc)
add_subdirectory(ext/webp)
add_subdirectory(ext/turbojpeg)
add_subdirectory(ext/zstd)
add_subdirectory(ext/deflate)
add_subdirectory(ext/jbigkit)
add_subdirectory(ext/tiff)
add_subdirectory(ext/png)
add_subdirectory(ext/expat)
add_subdirectory(ext/gettext)
add_subdirectory(ext/exiv2)
add_subdirectory(ext/lcms2)
add_subdirectory(ext/kakadu)


include_directories(
        ${CMAKE_SOURCE_DIR}/lib
        ${COMMON_LOCAL}/include
        /usr/local/include
)

add_library(iiifhandler MODULE
        IIIFHandler.cpp IIIFHandler.h
        IIIFError.cpp IIIFError.h
        IIIFSendInfo.cpp
        IIIFSendFile.cpp
        IIIFPreflight.cpp
        IIIFCheckFileAccess.cpp
        IIIFCache.cpp IIIFCache.h
        IIIFIO.h
        IIIFImage.cpp IIIFImage.h
        IIIFImgTools.cpp IIIFImgTools.h
        IIIFLua.cpp IIIFLua.h
        #AdobeRGB1998_icc.h USWebCoatedSWOP_icc.h Rec709-Rec1886_icc.h
        iiifparser/IIIFIdentifier.cpp iiifparser/IIIFIdentifier.h
        iiifparser/IIIFQualityFormat.cpp iiifparser/IIIFQualityFormat.h
        iiifparser/IIIFRegion.cpp iiifparser/IIIFRegion.h
        iiifparser/IIIFRotation.cpp iiifparser/IIIFRotation.h
        iiifparser/IIIFSize.cpp iiifparser/IIIFSize.h
        metadata/IIIFEssentials.cpp metadata/IIIFEssentials.h
        metadata/IIIFExif.cpp metadata/IIIFExif.h
        metadata/IIIFIcc.cpp metadata/IIIFIcc.h
        metadata/IIIFIptc.cpp metadata/IIIFIptc.h
        metadata/IIIFXmp.cpp metadata/IIIFXmp.h
        imgformats/IIIFIOTiff.cpp imgformats/IIIFIOTiff.h
        imgformats/IIIFIOPng.cpp imgformats/IIIFIOPng.h
        imgformats/IIIFIOJpeg.cpp imgformats/IIIFIOJpeg.h
        imgformats/IIIFIOJ2k.cpp imgformats/IIIFIOJ2k.h
        IIIFPhotometricInterpretation.h IIIFGetCanonicalUrl.cpp)

add_dependencies(iiifhandler
        icc_profiles
        turbojpeg
        tiff
        png
        expat
        gettext
        exiv2
        lcms2
        lerc
        webp
        kdu kdu_aux
        zstd
        jbigkit
        deflate
        sharpyuv)

set_target_properties(iiifhandler PROPERTIES PREFIX "")

target_link_libraries(iiifhandler PRIVATE
        cserve
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
        xz
        zlib
        zstd
        deflate
        sharpyuv
        Threads::Threads
        ${CMAKE_DL_LIBS})

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    target_link_libraries(iiifhandler PRIVATE
            ${COREFOUNDATION_FRAMEWORK}
            ${SYSTEMCONFIGURATION_FRAMEWORK})
else()
    target_link_libraries(iiifhandler lcms2 rt)
endif()


add_custom_command(TARGET iiifhandler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:iiifhandler>
        ${PROJECT_BINARY_DIR}/handler/$<TARGET_FILE_NAME:iiifhandler>)

add_custom_command(TARGET iiifhandler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:iiifhandler>
        ${CMAKE_SOURCE_DIR}/serverroot/handler/$<TARGET_FILE_NAME:iiifhandler>)


install(TARGETS iiifhandler DESTINATION ${COMMON_LOCAL}/share/cserver/)

include(CTest)

add_subdirectory(tests)

enable_testing()
