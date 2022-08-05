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
#ifndef __iiif_io_h
#define __iiif_io_h

#include <unordered_map>
#include <string>
#include <stdexcept>

#include "IIIFIO.h"
#include "IIIFImage.h"
#include "iiifparser/IIIFRegion.h"
#include "iiifparser/IIIFSize.h"

namespace cserve {

    /*!
     * This is the virtual base class for all classes implementing image I/O.
     */
    class IIIFIO {
    public:
        virtual ~IIIFIO() = default;

        /*!
         * Method used to read an image file
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Image file path
         * \param reduce Reducing factor. Some file formars support
         * reading a lower resolution of the file. A reducing factor of 2 indicates
         * to read only half the resolution. [default: 0]
         * \param force_bps_8 Convert the file to 8 bits/sample on reading thus enforcing an 8 bit image
         */
        virtual IIIFImage read(const std::string &filepath,
                               int pagenum, std::shared_ptr<IIIFRegion> region,
                               std::shared_ptr<IIIFSize> size,
                               bool force_bps_8,
                               ScalingQuality scaling_quality) = 0;

        IIIFImage read(const std::string &filepath) {
            return read(filepath, 0, nullptr, nullptr, false,
                        {HIGH, HIGH, HIGH, HIGH});
        }

        IIIFImage read(const std::string &filepath,
                       int pagenum) {
            return read(filepath, pagenum, nullptr, nullptr, false,
                        {HIGH, HIGH, HIGH, HIGH});
        }

        IIIFImage read(const std::string &filepath,
                       int pagenum,
                       std::shared_ptr<IIIFRegion> region) {
            return read(filepath, pagenum, region, nullptr, false,
                        {HIGH, HIGH, HIGH, HIGH});
        }

        IIIFImage read(const std::string &filepath,
                       int pagenum,
                       std::shared_ptr<IIIFRegion> region,
                       std::shared_ptr<IIIFSize> size) {
            return read(filepath, pagenum, region, size, false,
                        {HIGH, HIGH, HIGH, HIGH});
        }

        IIIFImage read(const std::string &filepath,
                       int pagenum, std::shared_ptr<IIIFRegion> region,
                       std::shared_ptr<IIIFSize> size,
                       bool force_bps_8) {
            return read(filepath, pagenum, region, size, force_bps_8,
                 {HIGH, HIGH, HIGH, HIGH});
        }

        /*!
         * Get the dimension of the image
         *
         * \param[in] filepath Pathname of the image file
         * \param[out] width Width of the image in pixels
         * \param[out] height Height of the image in pixels
         */
        virtual IIIFImgInfo getDim(const std::string &filepath, int pagenum) = 0;

        IIIFImgInfo getDim(const std::string &filepath) {
            return getDim(filepath, 0);
        }

        /*!
         * Write an image for a file using the given file format implemented by the subclass
         *
         * \param *img Pointer to SipiImage instance
         * \param filepath Name of the image file to be written. Please note that
         * - "-" means to write the image data to stdout
         * - "HTTP" means to write the image data to the HTTP-server output
         */
        virtual void write(IIIFImage &img,
                           const std::string &filepath,
                           const IIIFCompressionParams &params) = 0;

        void write(IIIFImage &img, const std::string &filepath) {
            write(img, filepath, {});
        }
    };

}

#endif
