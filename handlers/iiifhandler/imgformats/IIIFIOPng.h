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
#ifndef __io_png_h
#define __io_png_h

#include <string>

#include "../IIIFImage.h"
#include "../IIIFIO.h"

namespace cserve {

    class IIIFIOPng : public IIIFIO {
    public:
        ~IIIFIOPng() override = default;

        /*!
         * Method used to read an image file
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Image file path
         * \param reduce Reducing factor. Not used reading TIFF files
         */
        IIIFImage read(const std::string &filepath,
                       int pagenum, std::shared_ptr<IIIFRegion> region,
                       std::shared_ptr<IIIFSize> size,
                       bool force_bps_8,
                       ScalingQuality scaling_quality) override;

        /*!
         * Get the dimension of the image
         *
         * \param[in] filepath Pathname of the image file
         * \param[out] width Width of the image in pixels
         * \param[out] height Height of the image in pixels
         */
        IIIFImgInfo getDim(const std::string &filepath, int pagenum) override;


        /*!
         * Write a PNG image to a file, stdout or to a memory buffer
         *
         * If the filepath is "-", the PNG file is built in an internal memory buffer
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
