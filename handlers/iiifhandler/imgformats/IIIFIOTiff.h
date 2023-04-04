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
#ifndef sipi_io_tiff_h
#define sipi_io_tiff_h

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
        static void readExif(IIIFImage &img, TIFF *tif, toff_t exif_offset);

        /*!
         * Write the EXIF data to the TIFF file
          * \param img Pointer to SipiImage instance
          * \param[in] tif Pointer to TIFF file handle
         */
        static void writeExif(const IIIFImage &img, TIFF *tif);

        void write_basic_tags(const IIIFImage &img,
                              TIFF *tif,
                              uint32_t nx, uint32_t ny,
                              bool its_1_bit,
                              const std::string &compression
        );

        void write_subfile(const IIIFImage &img,
                      TIFF *tif,
                      uint32_t level,
                      uint32_t tile_width = 0,
                      uint32_t tile_height = 0,
                      const std::string &compression = "");
    public:
        ~IIIFIOTiff() override {};

        [[maybe_unused]]
        static void initLibrary();

        IIIFImage read(const std::string &filepath,
                       int pagenum, std::shared_ptr<IIIFRegion> region,
                       std::shared_ptr<IIIFSize> size,
                       bool force_bps_8,
                       ScalingQuality scaling_quality) override;

        IIIFImgInfo getDim(const std::string &filepath, int pagenum) override;

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
