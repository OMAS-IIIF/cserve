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
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
#include <cstring>
#include <cctype>

#include <climits>

#include "lcms2.h"
#include "fmt/format.h"
#include "Global.h"
#include "Hash.h"
#include "IIIFImage.h"
#include "Parsing.h"
#include "IIIFImgTools.h"
#include "imgformats/IIIFIOTiff.h"
#include "imgformats/IIIFIOJ2k.h"
#include "imgformats/IIIFIOJpeg.h"
#include "imgformats/IIIFIOPng.h"
#include "IIIFPhotometricInterpretation.h"


static const char file_[] = __FILE__;


namespace cserve {

    std::unordered_map<std::string, std::shared_ptr<IIIFIO>> IIIFImage::io = {{"tif", std::make_shared<IIIFIOTiff>()},
                                                                              {"jpx", std::make_shared<IIIFIOJ2k>()},
                                                                              {"jpg", std::make_shared<IIIFIOJpeg>()},
                                                                              {"png", std::make_shared<IIIFIOPng>()}};


    IIIFImage::IIIFImage() : nx(0), ny(0), nc(0), bps(0), orientation(TOPLEFT),
                             xmp(nullptr), icc(nullptr), iptc(nullptr), exif(nullptr), photo(INVALID),
                             skip_metadata(SKIP_NONE), conobj(nullptr) { };

    IIIFImage::IIIFImage(const IIIFImage &img_p)
            : nx(img_p.nx), ny(img_p.ny), nc(img_p.nc), bps(img_p.bps), orientation(img_p.orientation), es(img_p.es),
              photo(img_p.photo), emdata(img_p.emdata), skip_metadata(img_p.skip_metadata), conobj(img_p.conobj),
              xmp(img_p.xmp), icc(img_p.icc), iptc(img_p.iptc), exif(img_p.exif) {
        switch (bps) {
            case 0: {
                break;
            }
            case 8: {
                bpixels = img_p.bpixels;
                break;
            }
            case 16: {
                wpixels = img_p.wpixels;
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", bps));
            }
        }

    }
    //============================================================================

    IIIFImage::IIIFImage(IIIFImage &&other) noexcept: nx(0), ny(0), nc(0), bps(0), orientation(TOPLEFT), es({}),
                                                      photo(INVALID),
                                                      xmp(nullptr), icc(nullptr), iptc(nullptr), exif(nullptr),
                                                      emdata(), conobj(nullptr), skip_metadata(SKIP_NONE) {
        nx = other.nx;
        ny = other.ny;
        nc = other.nc;
        bps = other.bps;
        orientation = other.orientation;
        es = other.es;
        photo = other.photo;
        bpixels = std::move(other.bpixels);
        wpixels = std::move(other.wpixels);
        xmp = other.xmp;
        icc = other.icc;
        iptc = other.iptc;
        exif = other.exif;
        emdata = other.emdata;
        conobj = other.conobj;
        skip_metadata = other.skip_metadata;

        other.nx = 0;
        other.ny = 0;
        other.nc = 0;
        other.bps = 0;
        other.orientation = TOPLEFT;
        other.es = {};
        other.photo = INVALID;
        other.xmp = nullptr;
        other.icc = nullptr;
        other.exif = nullptr;
        other.emdata.clear();
        other.conobj = nullptr;
        other.skip_metadata = SKIP_NONE;
    }

    [[maybe_unused]]
    IIIFImage::IIIFImage(uint32_t nx_p, uint32_t ny_p, uint32_t nc_p, uint32_t bps_p,
                         PhotometricInterpretation photo_p)
            : nx(nx_p), ny(ny_p), nc(nc_p), bps(bps_p), photo(photo_p), xmp(nullptr), icc(nullptr), iptc(nullptr),
              exif(nullptr), skip_metadata(SKIP_NONE), conobj(nullptr) {
        orientation = TOPLEFT; // assuming default...
        if (((photo == MINISWHITE) || (photo == MINISBLACK)) && !((nc == 1) || (nc == 2))) {
            throw IIIFImageError(file_, __LINE__, "Mismatch in Photometric interpretation and number of channels");
        }
        if ((photo == RGB) && !((nc == 3) || (nc == 4))) {
            throw IIIFImageError(file_, __LINE__, "Mismatch in Photometric interpretation and number of channels");
        }
        if ((bps != 8) && (bps != 16)) {
            throw IIIFImageError(file_, __LINE__, "Bits per samples not supported by Sipi");
        }
        if (nx*ny*nc == 0) {
            throw IIIFImageError(file_, __LINE__, "Image with no content");
        }

        switch (bps) {
            case 8: {
                bpixels = std::vector<uint8_t>(nx * ny * nc);
                break;
            }
            case 16: {
                wpixels = std::vector<uint16_t>(nx * ny * nc);
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", bps));
            }
        }
    }

    [[maybe_unused]] void IIIFImage::setPixel(uint32_t x, uint32_t y, uint32_t c, int val) {
        if (x >= nx) throw IIIFImageError(file_, __LINE__, "Error in setPixel: x >= nx");
        if (y >= ny) throw IIIFImageError(file_, __LINE__, "Error in setPixel: y >= ny");
        if (c >= nc) throw IIIFImageError(file_, __LINE__, "Error in setPixel: c >= nc");

        switch (bps) {
            case 8: {
                if (val > 0xff) throw IIIFImageError(file_, __LINE__, "Error in setPixel: val > 0xff");
                bpixels[nc * (x * nx + y) + c] = static_cast<byte>(val);
                break;
            }
            case 16: {
                if (val > 0xffff) throw IIIFImageError(file_, __LINE__, "Error in setPixel: val > 0xffff");
                wpixels[nc * (x * nx + y) + c] = static_cast<word>(val);
                break;
            }
            default: {
                if (val > 0xffff) throw IIIFImageError(file_, __LINE__, "Error in setPixel: val > 0xffff");
            }
        }
    }

    IIIFImage &IIIFImage::operator=(const IIIFImage &img_p) {
        if (this != &img_p) {
            nx = img_p.nx;
            ny = img_p.ny;
            nc = img_p.nc;
            bps = img_p.bps;
            orientation = img_p.orientation;
            es = img_p.es;

            switch (bps) {
                case 0: {
                    break;
                }
                case 8: {
                    bpixels = img_p.bpixels;
                    break;
                }
                case 16: {
                    wpixels = img_p.wpixels;
                    break;
                }
                default: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", bps));
                }
            }
            xmp = img_p.xmp;
            icc = img_p.icc;
            iptc = img_p.iptc;
            exif = img_p.exif;
            skip_metadata = img_p.skip_metadata;
            conobj = img_p.conobj;
        }
        return *this;
    }

    IIIFImage &IIIFImage::operator=(IIIFImage &&other) noexcept {
        if (this != &other) {
            es = {};
            orientation = TOPLEFT;
            bpixels.clear();
            wpixels.clear();
            xmp.reset();
            icc.reset();
            iptc.reset();
            exif.reset();
            emdata.clear();
            delete conobj;

            nx = other.nx;
            ny = other.ny;
            nc = other.nc;
            bps = other.bps;
            orientation = other.orientation;
            es = other.es;
            photo = other.photo;
            bpixels = std::move(other.bpixels);
            wpixels = std::move(other.wpixels);
            xmp = other.xmp;
            icc = other.icc;
            iptc = other.iptc;
            exif = other.exif;
            emdata = other.emdata;
            conobj = other.conobj;
            skip_metadata = other.skip_metadata;

            other.nx = 0;
            other.ny = 0;
            other.nc = 0;
            other.bps = 0;
            other.orientation = TOPLEFT;
            other.es = {};
            other.photo = INVALID;
            other.xmp.reset();
            other.icc.reset();
            other.exif.reset();
            other.emdata.clear();
            other.conobj = nullptr;
            other.skip_metadata = SKIP_NONE;
        }
        return *this;
    }

    /*!
     * If this image has no SipiExif, creates an empty one.
     */
    void IIIFImage::ensure_exif() {
        if (exif == nullptr) exif = std::make_shared<IIIFExif>();
    }

    //============================================================================

     IIIFImage IIIFImage::read(const std::string& filepath,
                               const std::shared_ptr<IIIFRegion>& region,
                               const std::shared_ptr<IIIFSize>& size,
                               bool force_bps_8,
                               ScalingQuality scaling_quality) {
        std::filesystem::path fpath(filepath);
        std::string fext(fpath.extension().string());

        std::string _fext;
        std::transform(fext.begin(), fext.end(), _fext.begin(), [](unsigned char c) { return std::toupper(c); });

        try {
            if (_fext.empty()) {
                for (auto const &iterator : io) {
                    try {
                        return iterator.second->read(fpath.string(), region, size, force_bps_8, scaling_quality);
                    }
                    catch (const IIIFImageError &err) { }
                }
            } else if ((_fext == "tif") || (_fext == "tiff")) {
                return io["tif"]->read(fpath.string(), region, size, force_bps_8, scaling_quality);
            } else if ((_fext == "jpg") || (_fext == "jpeg")) {
                return io["jpg"]->read(fpath.string(), region, size, force_bps_8, scaling_quality);
            } else if (_fext == "png") {
                return io["png"]->read(fpath.string(), region, size, force_bps_8, scaling_quality);
            } else if ((_fext == "jp2") || (_fext == "jpx") || (_fext == "j2k")) {
                return io["jpx"]->read(fpath.string(), region, size, force_bps_8, scaling_quality);
            }
            // file seems to have the wrong extension, let's try all image formats we support
            for (auto const &iterator : io) {
                try {
                    return iterator.second->read(fpath.string(), region, size, force_bps_8, scaling_quality);
                }
                catch (const IIIFImageError &err) { }
            }
        }
        catch (const IIIFImageError &err) {
            for (auto const &iterator : io) {
                try {
                    return iterator.second->read(fpath.string(), region, size, force_bps_8, scaling_quality);
                }
                catch (const IIIFImageError &err) { }
            }
            throw IIIFImageError(file_, __LINE__, fmt::format("Could not read file '{}'", fpath.string()));
         }
         throw IIIFImageError(file_, __LINE__, fmt::format("Could not read file '{}'", fpath.string()));
    }
    //============================================================================

    IIIFImage IIIFImage::readOriginal(const std::string &filepath,
                                      const std::shared_ptr<IIIFRegion>& region,
                                      const std::shared_ptr<IIIFSize>& size,
                                      const std::string &origname,
                                      HashType htype) {
        IIIFImage img = IIIFImage::read(filepath, region, size, false);

        if (!img.emdata.is_set()) {
            Hash internal_hash(htype);
            switch (img.bps) {
                case 0: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", img.bps));
                }
                case 8: {
                    internal_hash.add_data(img.bpixels.data(), img.nx * img.ny * img.nc * sizeof(uint8_t));
                    break;
                }
                case 16: {
                    internal_hash.add_data(img.wpixels.data(), img.nx * img.ny * img.nc * sizeof(uint16_t));
                    break;
                }
                default: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", img.bps));
                }
            }
            std::string checksum = internal_hash.hash();
            std::string mimetype = Parsing::getFileMimetype(filepath).first;
            IIIFEssentials emdata(origname, mimetype, HashType::sha256, checksum);
            img.essential_metadata(emdata);
        } else {
            Hash internal_hash(img.emdata.hash_type());
            switch (img.bps) {
                case 0: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", img.bps));
                }
                case 8: {
                    internal_hash.add_data(img.bpixels.data(), img.nx * img.ny * img.nc * sizeof(uint8_t));
                    break;
                }
                case 16: {
                    internal_hash.add_data(img.wpixels.data(), img.nx * img.ny * img.nc * sizeof(uint16_t));
                    break;
                }
                default: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", img.bps));
                }
            }
            std::string checksum = internal_hash.hash();
            if (checksum != img.emdata.data_chksum()) {
                throw IIIFImageError(file_, __LINE__, "Checksum of data differs from expected value.");
            }
        }

        return img;
    }
    //============================================================================

    [[maybe_unused]]
    IIIFImage IIIFImage::readOriginal(const std::string &filepath,
                                      const std::shared_ptr<IIIFRegion>& region,
                                      const std::shared_ptr<IIIFSize>& size,
                                      HashType htype) {
        std::string origname = getFileName(filepath);
        return IIIFImage::readOriginal(filepath, region, size, origname, htype);
    }
    //============================================================================


    [[maybe_unused]]
    IIIFImgInfo IIIFImage::getDim(const std::string &filepath) {
        size_t pos = filepath.find_last_of('.');
        std::string fext = filepath.substr(pos + 1);
        std::string _fext;

        _fext.resize(fext.size());
        std::transform(fext.begin(), fext.end(), _fext.begin(), ::tolower);

        IIIFImgInfo info;
        std::string mimetype = Parsing::getFileMimetype(filepath).first;

        if ((mimetype == "image/tiff") || (mimetype == "image/x-tiff")) {
            info = io[std::string("tif")]->getDim(filepath);
        } else if ((mimetype == "image/jpeg") || (mimetype == "image/pjpeg")) {
            info = io[std::string("jpg")]->getDim(filepath);
        } else if (mimetype == "image/png") {
            info = io[std::string("png")]->getDim(filepath);
        } else if ((mimetype == "image/jp2") || (mimetype == "image/jpx")) {
            info = io[std::string("jpx")]->getDim(filepath);
        } else if (mimetype == "application/pdf") {
            info = io[std::string("pdf")]->getDim(filepath);
        }
        info.internalmimetype = mimetype;

        if (info.success == IIIFImgInfo::FAILURE) {
            for (auto const &iterator : io) {
                info = iterator.second->getDim(filepath);
                if (info.success != IIIFImgInfo::FAILURE) break;
            }
        }

        if (info.success == IIIFImgInfo::FAILURE) {
            throw IIIFImageError(file_, __LINE__, "Could not read file " + filepath);
        }
        return info;
    }
    //============================================================================


    [[maybe_unused]]
    void IIIFImage::getDim(uint32_t &width, uint32_t &height) const {
        width = getNx();
        height = getNy();
    }
    //============================================================================

    void IIIFImage::write(const std::string &ftype, const std::string &filepath, const IIIFCompressionParams &params) {
        io[ftype]->write(*this, filepath, params);
   }
    //============================================================================

    [[maybe_unused]]
    void IIIFImage::convertYCC2RGB() {
        if (bps == 8) {
            auto outbuf = std::vector<uint8_t>(nc*nx*ny);

            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    auto Y = (double) bpixels[nc * (j * nx + i) + 2];
                    auto Cb = (double) bpixels[nc * (j * nx + i) + 1];
                    auto Cr = (double) bpixels[nc * (j * nx + i) + 0];

                    int r = (int) (Y + 1.40200 * (Cr - 0x80));
                    int g = (int) (Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
                    int b = (int) (Y + 1.77200 * (Cb - 0x80));

                    outbuf[nc * (j * nx + i) + 0] = std::max(0, std::min(255, r));
                    outbuf[nc * (j * nx + i) + 1] = std::max(0, std::min(255, g));
                    outbuf[nc * (j * nx + i) + 2] = std::max(0, std::min(255, b));

                    for (size_t k = 3; k < nc; k++) {
                        outbuf[nc * (j * nx + i) + k] = bpixels[nc * (j * nx + i) + k];
                    }
                }
            }
            bpixels = std::move(outbuf);
        } else if (bps == 16) {
            auto outbuf = std::vector<uint16_t>(nc*nx*ny);

            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    auto Y = (double) wpixels[nc * (j * nx + i) + 2];
                    auto Cb = (double) wpixels[nc * (j * nx + i) + 1];
                    auto Cr = (double) wpixels[nc * (j * nx + i) + 0];

                    int r = (int) (Y + 1.40200 * (Cr - 0x80));
                    int g = (int) (Y - 0.34414 * (Cb - 0x80) - 0.71414 * (Cr - 0x80));
                    int b = (int) (Y + 1.77200 * (Cb - 0x80));

                    outbuf[nc * (j * nx + i) + 0] = std::max(0, std::min(65535, r));
                    outbuf[nc * (j * nx + i) + 1] = std::max(0, std::min(65535, g));
                    outbuf[nc * (j * nx + i) + 2] = std::max(0, std::min(65535, b));

                    for (size_t k = 3; k < nc; k++) {
                        outbuf[nc * (j * nx + i) + k] = wpixels[nc * (j * nx + i) + k];
                    }
                }
            }
            wpixels = std::move(outbuf);
        } else {
            throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
        }
    }
    //============================================================================

    void IIIFImage::convertToIcc(const IIIFIcc &target_icc_p, uint32_t new_bps) {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsUInt32Number in_formatter, out_formatter;

        if (icc == nullptr) {
            switch (nc) {
                case 1: {
                    icc = std::make_shared<IIIFIcc>(icc_GRAY_D50); // assume gray value image with D50
                    break;
                }
                case 3: {
                    icc = std::make_shared<IIIFIcc>(icc_sRGB); // assume sRGB
                    break;
                }
                case 4: {
                    icc = std::make_shared<IIIFIcc>(icc_CYMK_standard); // assume CYMK
                    break;
                }
                default: {
                    throw IIIFImageError(file_, __LINE__,
                                         "Cannot assign ICC profile to image with nc=" + std::to_string(nc));
                }
            }
        }
        unsigned int nnc = cmsChannelsOf(cmsGetColorSpace(target_icc_p.getIccProfile()));

        if (!((new_bps == 8) || (new_bps == 16))) {
            throw IIIFImageError(file_, __LINE__, "Unsupported bits/sample (" + std::to_string(bps) + ")");
        }

        cmsHTRANSFORM hTransform;
        in_formatter = icc->iccFormatter(nc, bps, photo);
        out_formatter = target_icc_p.iccFormatter(new_bps);

        hTransform = cmsCreateTransform(icc->getIccProfile(), in_formatter, target_icc_p.getIccProfile(), out_formatter,
                                        INTENT_PERCEPTUAL, 0);

        if (hTransform == nullptr) {
            throw IIIFImageError(file_, __LINE__, "Couldn't create color transform");
        }

        void *inbuf;
        switch (bps) {
            case 8: {
                inbuf = bpixels.data();
                break;
            }
            case 16: {
                inbuf = wpixels.data();
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
            }
        }

        switch (new_bps) {
            case 8: {
                auto boutbuf = std::vector<uint8_t>(nx*ny*nnc);
                cmsDoTransform(hTransform, inbuf, boutbuf.data(), nx * ny);
                wpixels.clear();
                bpixels = std::move(boutbuf);
                break;
            }
            case 16: {
                auto woutbuf = std::vector<uint16_t>(nx*ny*nnc);
                cmsDoTransform(hTransform, inbuf, woutbuf.data(), nx * ny);
                bpixels.clear();
                wpixels = std::move(woutbuf);
                break;
            }
            default: { }
        }
        cmsDeleteTransform(hTransform);
        icc = std::make_shared<IIIFIcc>(target_icc_p);
        nc = nnc;
        bps = new_bps;

        PredefinedProfiles targetPT = target_icc_p.getProfileType();
        switch (targetPT) {
            case icc_GRAY_D50: {
                photo = MINISBLACK;
                break;
            }
            case icc_RGB:
            case icc_sRGB:
            case icc_AdobeRGB: {
                photo = RGB;
                break;
            }
            case icc_CYMK_standard: {
                photo = SEPARATED;
                break;
            }
            case icc_LAB: {
                photo = CIELAB;
                break;
            }
            default: { }
        }
    }


    [[maybe_unused]]
    void IIIFImage::removeChan(uint32_t chan) {
        if ((nc == 1) || (chan >= nc)) {
            std::string msg = "Cannot remove component: nc=" + std::to_string(nc) + " chan=" + std::to_string(chan);
            throw IIIFImageError(file_, __LINE__, msg);
        }

        if (!es.empty()) {
            if (nc < 3) {
                es.clear(); // no more alpha channel
            } else if (nc > 3) { // it's probably an alpha channel
                if ((nc == 4) && (photo == SEPARATED)) {  // oh no. – 4 channes, but CMYK
                    throw IIIFImageError(file_, __LINE__, fmt::format("Cannot remove component: nc={} chan={}", nc, chan));
                } else {
                    es.erase(es.begin() + (chan - ((photo == SEPARATED) ? 4 : 3)));
                }
            } else {
                std::string msg = "Cannot remove component: nc=" + std::to_string(nc) + " chan=" + std::to_string(chan);
                throw IIIFImageError(file_, __LINE__, msg);
            }
        }

        if (bps == 8) {
            size_t nnc = nc - 1;
            auto boutbuf = std::vector<uint8_t>(nnc*nx*ny);

            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        if (k == chan) continue;
                        boutbuf[nnc * (j * nx + i) + k] = bpixels[nc * (j * nx + i) + k];
                    }
                }
            }
            bpixels = std::move(boutbuf);
        } else if (bps == 16) {
            size_t nnc = nc - 1;
            auto woutbuf = std::vector<uint16_t>(nnc*nx*ny);

            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        if (k == chan) continue;
                        woutbuf[nnc * (j * nx + i) + k] = wpixels[nc * (j * nx + i) + k];
                    }
                }
            }

            wpixels = std::move(woutbuf);
        } else {
            if (bps != 8) {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
            }
        }
        nc--;
    }

    [[maybe_unused]]
    bool IIIFImage::crop(const std::shared_ptr<IIIFRegion> &region) {
        int x, y;
        uint32_t width, height;
        if (region->getType() == IIIFRegion::FULL) return true; // we do not have to crop;
        region->crop_coords(nx, ny, x, y, width, height);
        if (bps == 8) {
            bpixels = doCrop<uint8_t>(std::move(bpixels), nx, ny, nc, region);
        }
        else if (bps == 16) {
            wpixels = doCrop<uint16_t>(std::move(wpixels), nx, ny, nc, region);
        } else {
            throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
        }

        nx = width;
        ny = height;
        return true;
    }

    [[maybe_unused]]
    void IIIFImage::crop(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
        auto region = std::make_shared<IIIFRegion>(x, y, width, height);
        if (!crop(region)) {
            throw IIIFImageError(file_, __LINE__, fmt::format("crop failed£", bps));
        }
    }

    bool IIIFImage::scaleFast(uint32_t nnx, uint32_t nny) {
        if (bps == 8) {
            bpixels = doScaleFast<uint8_t>(std::move(bpixels), nx, ny, nc, nnx, nny);
        }
        else if (bps == 16) {
            wpixels = doScaleFast<uint16_t>(std::move(wpixels), nx, ny, nc, nnx, nny);
        } else {
            return false;
        }
        nx = nnx;
        ny = nny;
        return true;
    }

    bool IIIFImage::scaleMedium(uint32_t nnx, uint32_t nny) {
        if (bps == 8) {
            bpixels = doScaleMedium<uint8_t>(std::move(bpixels), nx, ny, nc, nnx, nny);
        }
        else if (bps == 16) {
            wpixels = doScaleMedium<uint16_t>(std::move(wpixels), nx, ny, nc, nnx, nny);
        } else {
            return false;
        }
        nx = nnx;
        ny = nny;
        return true;
    }
    /*==========================================================================*/

    bool IIIFImage::reduce(uint32_t reduce_p) {
        uint32_t nnx{0};
        uint32_t nny{0};
        if (bps == 8) {
            bpixels = doReduce(std::move(bpixels), reduce_p, nx, ny, nc, nnx, nny);
        }
        else if (bps == 16) {
            wpixels = doReduce(std::move(wpixels), reduce_p, nx, ny, nc, nnx, nny);
        } else {
            return false;
        }
        nx = nnx;
        ny = nny;
        return true;
    }

    bool IIIFImage::scale(uint32_t nnx, uint32_t nny) {
        if (bps == 8) {
            bpixels = doScale<uint8_t>(std::move(bpixels), nx, ny, nc, nnx, nny);
        }
        else if (bps == 16) {
            wpixels = doScale<uint16_t>(std::move(wpixels), nx, ny, nc, nnx, nny);
        } else {
            return false;
        }
        nx = nnx;
        ny = nny;
        return true;
    }

    bool IIIFImage::rotate(float angle, bool mirror) {
        uint32_t nnx, nny;
        if (bps == 8) {
             bpixels = doRotate<uint8_t>(std::move(bpixels), nx, ny, nc, nnx, nny, angle, mirror);
        }
        else if (bps == 16) {
            wpixels = doRotate<uint16_t>(std::move(wpixels), nx, ny, nc, nnx, nny, angle, mirror);
        }
        nx = nnx;
        ny = nny;
        return true;
    }
    //============================================================================

    bool IIIFImage::set_topleft() {
        switch (orientation) {
            case TOPLEFT: // 1
                return true;
            case TOPRIGHT: // 2
                rotate(0., true);
                break;
            case BOTRIGHT: // 3
                rotate(180., false);
                break;
            case BOTLEFT: // 4
                rotate(180., true);
                break;
            case LEFTTOP: // 5
                rotate(270., true);
                break;
            case RIGHTTOP: // 6
                rotate(90., false);
                break;
            case RIGHTBOT: // 7
                rotate(90., true);
                break;
            case LEFTBOT: // 8
                rotate(270., false);
                break;
            default:
                ; // nothing to do...
        }
        orientation = TOPLEFT;
        if (exif != nullptr) {
            exif->addKeyVal("Exif.Image.Orientation", static_cast<unsigned short>(TOPLEFT));
        }
        return true;
    }
    //============================================================================

    bool IIIFImage::to8bps() {
        // little-endian architecture assumed
        //
        // we just use the shift-right operater (>> 8) to devide the values by 256 (2^8)!
        // This is the most efficient and fastest way
        //
        if (bps == 16) {
            //icc = NULL;

            //byte *outbuf = new(std::nothrow) Sipi::byte[nc*nx*ny];
            auto boutbuf = std::vector<uint8_t>(nc * nx * ny);
            for (uint32_t i = 0; i < nx*ny*nc; ++i) {
                boutbuf[i] = static_cast<uint8_t>(wpixels[i] >> 8);
            }
            bpixels = std::move(boutbuf);
            bps = 8;
        }
        return true;
    }
    //============================================================================


    bool IIIFImage::toBitonal() {
        if ((photo != MINISBLACK) && (photo != MINISWHITE)) {
            convertToIcc(IIIFIcc(icc_GRAY_D50), 8);
        }

        bool doit = false; // will be set true if we find a value not equal 0 or 255
        for (size_t i = 0; i < nx * ny; i++) {
            if ((bpixels[i] != 0) && (bpixels[i] != 255)) {
                doit = true;
                break;
            }
        }

        if (!doit) return true; // we have to do nothing, it's already bitonal

        // must be signed!! Error propagation my result in values < 0 or > 255
        //auto *outbuf = new(std::nothrow) short[nx * ny];
        auto outbuf = std::make_unique<short[]>(nx * ny);

        for (size_t i = 0; i < nx * ny; i++) {
            outbuf[i] = bpixels[i];  // copy buffer
        }

        for (size_t y = 0; y < ny; y++) {
            for (size_t x = 0; x < nx; x++) {
                short oldpixel = outbuf[y * nx + x];
                outbuf[y * nx + x] = (oldpixel > 127) ? 255 : 0;
                int properr = (oldpixel - outbuf[y * nx + x]);
                if (x < (nx - 1)) outbuf[y * nx + (x + 1)] += (7 * properr) >> 4;
                if ((x > 0) && (y < (ny - 1))) outbuf[(y + 1) * nx + (x - 1)] += (3 * properr) >> 4;
                if (y < (ny - 1)) outbuf[(y + 1) * nx + x] += (5 * properr) >> 4;
                if ((x < (nx - 1)) && (y < (ny - 1))) outbuf[(y + 1) * nx + (x + 1)] += properr >> 4;
            }
        }

        for (size_t i = 0; i < nx * ny; i++) bpixels[i] = outbuf[i];
        return true;
    }
    //============================================================================


    bool IIIFImage::add_watermark(const std::string &wmfilename) {
        uint32_t wm_nx, wm_ny, wm_nc;
        std::vector<uint8_t> wmbuf = read_watermark(wmfilename, wm_nx, wm_ny, wm_nc);
        if (wmbuf.empty()) {
            throw IIIFImageError(file_, __LINE__, "Cannot read watermark file " + wmfilename);
        }

        auto xlut = std::vector<double>(nx);
        auto ylut = std::vector<double>(ny);

        for (size_t i = 0; i < nx; i++) {
            xlut[i] = (double) (wm_nx * i) / (double) nx;
        }
        for (size_t j = 0; j < ny; j++) {
            ylut[j] = (double) (wm_ny * j) / (double) ny;
        }

        if (bps == 8) {
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    auto val = bilinn<uint8_t>(wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);

                    for (size_t k = 0; k < nc; k++) {
                        double nval = (bpixels[nc * (j * nx + i) + k] / 255.) * (1.0 + val / 2550.0) + val / 2550.0;
                        bpixels[nc * (j * nx + i) + k] = (nval > 1.0) ? 255 : static_cast<uint8_t>(floorl(nval * 255. + .5));
                    }
                }
            }
        } else if (bps == 16) {
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        byte val = bilinn<uint8_t>(wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);
                        double nval =
                                (wpixels[nc * (j * nx + i) + k] / 65535.0) * (1.0 + val / 655350.0) + val / 352500.;
                        wpixels[nc * (j * nx + i) + k] = (nval > 1.0) ? static_cast<uint16_t>(65535) : static_cast<uint16_t>(floorl(nval * 65535. + .5));
                    }
                }
            }
        }
        return true;
    }

    /*==========================================================================*/


    IIIFImage &IIIFImage::operator-=(const IIIFImage &rhs) {
        if ((nx != rhs.nx) || (ny != rhs.ny) || (nc != rhs.nc) || (bps != rhs.bps) || (photo != rhs.photo)) {
            std::stringstream ss;
            ss << "Image op: images not compatible" << std::endl;
            ss << "Image 1:  nx: " << nx << " ny: " << ny << " nc: " << nc << " bps: " << bps << " photo: " << as_integer(photo) << std::endl;
            ss << "Image 2:  nx: " << rhs.nx << " ny: " << rhs.ny << " nc: " << rhs.nc << " bps: " << rhs.bps << " photo: " << as_integer(rhs.photo)
               << std::endl;
            throw IIIFImageError(file_, __LINE__, ss.str());
        }


        auto diffbuf = std::vector<int32_t>(nx * ny * nc);
        switch (bps) {
            case 8: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    diffbuf[i] = bpixels[i] - rhs.bpixels[i];
                }
                break;
            }
            case 16: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    diffbuf[i] = wpixels[i] - rhs.wpixels[i];
                }
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        int32_t min = INT_MAX;
        int32_t max = INT_MIN;
        for (uint32_t i = 0; i < nx * ny * nc; ++i) {
            if (diffbuf[i] > max) max = diffbuf[i];
            if (diffbuf[i] < min) min = diffbuf[i];
        }

        int32_t maxmax = abs(min) > abs(max) ? abs(min) : abs(max);

        switch (bps) {
            case 8: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    bpixels[i] = (uint8_t) ((diffbuf[i] + maxmax) * UCHAR_MAX / (2 * maxmax));
                }
                break;
            }
            case 16: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    wpixels[i] = (word) ((diffbuf[i] + maxmax) * USHRT_MAX / (2 * maxmax));
                }
                break;
            }

            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        return *this;
    }

    /*==========================================================================*/

    IIIFImage &IIIFImage::operator-(const IIIFImage &rhs) {
        auto *lhs = new IIIFImage(*this);
        *lhs -= rhs;
        return *lhs;
    }

    /*==========================================================================*/

    IIIFImage &IIIFImage::operator+=(const IIIFImage &rhs) {
        if ((nx != rhs.nx) || (ny != rhs.ny) || (nc != rhs.nc) || (bps != rhs.bps) || (photo != rhs.photo)) {
            std::stringstream ss;
            ss << "Image op: images not compatible" << std::endl;
            ss << "Image 1:  nx: " << nx << " ny: " << ny << " nc: " << nc << " bps: " << bps << " photo: " << as_integer(photo) << std::endl;
            ss << "Image 2:  nx: " << rhs.nx << " ny: " << rhs.ny << " nc: " << rhs.nc << " bps: " << rhs.bps << " photo: " << as_integer(rhs.photo)
               << std::endl;
            throw IIIFImageError(file_, __LINE__, ss.str());
        }

        auto diffbuf = std::vector<int32_t>(nx * ny * nc);
        switch (bps) {
            case 8: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    diffbuf[i] = bpixels[i] + rhs.bpixels[i];
                }
                break;
            }
            case 16: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    diffbuf[i] = wpixels[i] + rhs.wpixels[i];
                }
                break;
            }

            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        int max = INT_MIN;
        for (uint32_t i = 0; i < nx * ny * nc; ++i) {
            if (diffbuf[i] > max) max = diffbuf[i];
        }

        switch (bps) {
            case 8: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    bpixels[i] = static_cast<uint8_t>(diffbuf[i] * UCHAR_MAX / max);
                }
                break;
            }
            case 16: {
                for (uint32_t i = 0; i < nx * ny * nc; ++i) {
                    wpixels[i] = static_cast<uint16_t>(diffbuf[i] * USHRT_MAX / max);
                }
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }
        return *this;
    }

    /*==========================================================================*/

    IIIFImage &IIIFImage::operator+(const IIIFImage &rhs) {
        auto *lhs = new IIIFImage(*this);
        *lhs += rhs;
        return *lhs;
    }

    /*==========================================================================*/

    bool IIIFImage::operator==(const IIIFImage &rhs) {
        if ((nx != rhs.nx) || (ny != rhs.ny) || (nc != rhs.nc) || (bps != rhs.bps) || (photo != rhs.photo)) {
            return false;
        }

        uint64_t n_differences = 0;
        switch (bps) {
            case 8: {
                for (uint32_t i = 0; i < nx*ny*nc; ++i) {
                    if (bpixels[i] != rhs.bpixels[i]) {
                        n_differences++;
                    }
                }
                break;
            }
            case 16: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (wpixels[nc * (j * nx + i) + k] != rhs.wpixels[nc * (j * nx + i) + k]) {
                                n_differences++;
                            }
                        }
                    }
                }
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        return n_differences == 0;
    }

    std::ostream &operator<<(std::ostream &outstr, const IIIFImage &rhs) {
        outstr << std::endl << "SipiImage with the following parameters:" << std::endl;
        outstr << "nx    = " << std::to_string(rhs.nx) << std::endl;
        outstr << "ny    = " << std::to_string(rhs.ny) << std::endl;
        outstr << "nc    = " << std::to_string(rhs.nc) << std::endl;
        outstr << "es    = " << std::to_string(rhs.es.size()) << std::endl;
        outstr << "bps   = " << std::to_string(rhs.bps) << std::endl;
        outstr << "ori   = " << orientation_str(rhs.orientation) << std::endl;
        outstr << "photo = " << std::to_string(rhs.photo) << std::endl;

        if (rhs.xmp) {
            outstr << "XMP-Metadata: " << std::endl << *(rhs.xmp) << std::endl;
        }

        if (rhs.iptc) {
            outstr << "IPTC-Metadata: " << std::endl << *(rhs.iptc) << std::endl;
        }

        if (rhs.exif) {
            outstr << "EXIF-Metadata: " << std::endl << *(rhs.exif) << std::endl;
        }

        if (rhs.icc) {
            outstr << "ICC-Metadata: " << std::endl << *(rhs.icc) << std::endl;
        }

        return outstr;
    }
    //============================================================================

}
