/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef __sipi_io_tiff_h
#define __sipi_io_tiff_h

#include <string>

#include "tiff.h"
#include "tiffio.h"

#include "../IIIFImage.h"
#include "../IIIFIO.h"

namespace cserve {

    class IIIFImage;

    std::unique_ptr<unsigned char[]> read_watermark(const std::string &wmfile, int &nx, int &ny, int &nc);

    /*! Class which implements the TIFF-reader/writer */
    class IIIFIOTiff : public IIIFIO {
    private:
        /*!
         * Read the EXIF data from the TIFF file and create an Exiv2::Exif object
         * \param img Pointer to SipiImage instance
         * \param[in] tif Pointer to TIFF file handle
         * \param[in] exif_offset Offset of EXIF directory in TIFF file
         */
        void readExif(IIIFImage &img, TIFF *tif, toff_t exif_offset);

        /*!
         * Write the EXIF data to the TIFF file
          * \param img Pointer to SipiImage instance
          * \param[in] tif Pointer to TIFF file handle
         */
        void writeExif(const IIIFImage &img, TIFF *tif);


    public:
        virtual ~IIIFIOTiff() {};

        [[maybe_unused]]
        static void initLibrary(void);

        IIIFImage read(const std::string &filepath,
                       int pagenum, std::shared_ptr<IIIFRegion> region,
                       std::shared_ptr<IIIFSize> size,
                       bool force_bps_8,
                       ScalingQuality scaling_quality) override;

        IIIFImgInfo getDim(const std::string &filepath, int pagenum = 0) override;

        /*!
         * Write a TIFF image to a file, stdout or to a memory buffer
         *
         * If the filepath is "-", the TIFF file is built in an internal memory buffer
         * and after finished transfered to stdout. This is necessary because libtiff
         * makes extensive use of "lseek" which is not available on stdout!
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Name of the image file to be written. Please note that
         * - "-" means to write the image data to stdout
         * - "HTTP" means to write the image data to the HTTP-server output
         */
        void write(IIIFImage &img, const std::string &filepath, const IIIFCompressionParams &params) override;
    };
}

#endif
