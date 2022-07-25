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
#ifndef __iiif_io_jpeg_h
#define __iiif_io_jpeg_h

#include <string>

#include "../IIIFImage.h"
#include "../IIIFIO.h"

namespace cserve {

    /*! Class which implements the JPEG2000-reader/writer */
    class IIIFIOJpeg : public IIIFIO {
    private:
        static void parse_photoshop(IIIFImage &img, char *data, int length);

    public:
        ~IIIFIOJpeg() override = default;

        /*!
         * Method used to read an image file
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Image file path
         * \param reduce Reducing factor. If it is not 0, the reader
         * only reads part of the data returning an image with reduces resolution.
         * If the value is 1, only half the resolution is returned. If it is 2, only one forth etc.
         */
        IIIFImage read(const std::string &filepath,
                       int pagenum,
                       std::shared_ptr<IIIFRegion> region,
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
         * Write a JPEG image to a file, stdout or to a memory buffer
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Name of the image file to be written.
         */
        void write(IIIFImage &img, const std::string &filepath, const IIIFCompressionParams &params) override;
    };

}


#endif
