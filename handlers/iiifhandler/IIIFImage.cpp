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

#include <climits>

#include "lcms2.h"
#include "fmt/format.h"
#include "Global.h"
#include "Hash.h"
//#include "IIIFImage.h"
#include "Parsing.h"
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


    IIIFImage::IIIFImage() : nx(0), ny(0), nc(0), bps(0), orientation(TOPLEFT), bpixels(nullptr), wpixels(nullptr),
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
                bpixels = std::make_unique<byte[]>(nx * ny * nc);
                std::memcpy(bpixels.get(), img_p.bpixels.get(), nx * ny * nc * sizeof(byte));
                break;
            }
            case 16: {
                wpixels = std::make_unique<word[]>(nx * ny * nc);
                std::memcpy(wpixels.get(), img_p.wpixels.get(), nx * ny * nc * sizeof(word));
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", bps));
            }
        }

    }
    //============================================================================

    IIIFImage::IIIFImage(IIIFImage &&other) noexcept: nx(0), ny(0), nc(0), bps(0), orientation(TOPLEFT), es({}),
                                                      photo(INVALID), bpixels(nullptr), wpixels(nullptr),
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
    IIIFImage::IIIFImage(size_t nx_p, size_t ny_p, size_t nc_p, size_t bps_p,
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
            case 0: {
                bpixels = nullptr;
                wpixels = nullptr;
                break;
            }
            case 8: {
                bpixels = std::make_unique<byte[]>(nx * ny * nc);
                break;
            }
            case 16: {
                wpixels = std::make_unique<word[]>(nx * ny * nc);
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", bps));
            }
        }
    }

    [[maybe_unused]] void IIIFImage::setPixel(unsigned int x, unsigned int y, unsigned int c, int val) {
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
                    bpixels = std::make_unique<byte[]>(nx*ny*nc);
                    memcpy(bpixels.get(), img_p.bpixels.get(), nx*ny*nc*sizeof(byte));
                    break;
                }
                case 16: {
                    wpixels = std::make_unique<word[]>(nx*ny*nc);
                    memcpy(wpixels.get(), img_p.wpixels.get(), nx*ny*nc*sizeof(word));
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
            bpixels.reset();
            wpixels.reset();
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
                               int pagenum,
                               const std::shared_ptr<IIIFRegion>& region,
                               const std::shared_ptr<IIIFSize>& size,
                               bool force_bps_8,
                               ScalingQuality scaling_quality) {
        size_t pos = filepath.find_last_of('.');
        std::string fext = filepath.substr(pos + 1);
        std::string _fext;

        _fext.resize(fext.size());
        std::transform(fext.begin(), fext.end(), _fext.begin(), ::tolower);

        try {
            if ((_fext == "tif") || (_fext == "tiff")) {
                return io[std::string("tif")]->read(filepath, pagenum, region, size, force_bps_8, scaling_quality);
            } else if ((_fext == "jpg") || (_fext == "jpeg")) {
                return io[std::string("jpg")]->read(filepath, pagenum, region, size, force_bps_8, scaling_quality);
            } else if (_fext == "png") {
                return io[std::string("png")]->read(filepath, pagenum, region, size, force_bps_8, scaling_quality);
            } else if ((_fext == "jp2") || (_fext == "jpx") || (_fext == "j2k")) {
                return io[std::string("jpx")]->read(filepath, pagenum, region, size, force_bps_8, scaling_quality);
            }
        }
        catch (const IIIFImageError &err) {
            for (auto const &iterator : io) {
                try {
                    return iterator.second->read(filepath, pagenum, region, size, force_bps_8, scaling_quality);
                }
                catch (const IIIFImageError &err) { }
            }
            throw IIIFImageError(file_, __LINE__, "Could not read file " + filepath);
         }
         throw IIIFImageError(file_, __LINE__, "Could not read file " + filepath);
    }
    //============================================================================

    IIIFImage IIIFImage::readOriginal(const std::string &filepath,
                                      int pagenum,
                                      const std::shared_ptr<IIIFRegion>& region,
                                      const std::shared_ptr<IIIFSize>& size,
                                      const std::string &origname,
                                      HashType htype) {
        IIIFImage img = IIIFImage::read(filepath, pagenum, region, size, false);

        if (!img.emdata.is_set()) {
            Hash internal_hash(htype);
            switch (img.bps) {
                case 0: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Image with invalid bps (bps = {}).", img.bps));
                }
                case 8: {
                    internal_hash.add_data(img.bpixels.get(), img.nx * img.ny * img.nc * sizeof(byte));
                    break;
                }
                case 16: {
                    internal_hash.add_data(img.wpixels.get(), img.nx * img.ny * img.nc * sizeof(word));
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
                    internal_hash.add_data(img.bpixels.get(), img.nx * img.ny * img.nc * sizeof(byte));
                    break;
                }
                case 16: {
                    internal_hash.add_data(img.wpixels.get(), img.nx * img.ny * img.nc * sizeof(word));
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
                                      int pagenum,
                                      const std::shared_ptr<IIIFRegion>& region,
                                      const std::shared_ptr<IIIFSize>& size,
                                      HashType htype) {
        std::string origname = getFileName(filepath);
        return IIIFImage::readOriginal(filepath, pagenum, region, size, origname, htype);
    }
    //============================================================================


    [[maybe_unused]]
    IIIFImgInfo IIIFImage::getDim(const std::string &filepath, int pagenum) {
        size_t pos = filepath.find_last_of('.');
        std::string fext = filepath.substr(pos + 1);
        std::string _fext;

        _fext.resize(fext.size());
        std::transform(fext.begin(), fext.end(), _fext.begin(), ::tolower);

        IIIFImgInfo info;
        std::string mimetype = Parsing::getFileMimetype(filepath).first;

        if ((mimetype == "image/tiff") || (mimetype == "image/x-tiff")) {
            info = io[std::string("tif")]->getDim(filepath, pagenum);
        } else if ((mimetype == "image/jpeg") || (mimetype == "image/pjpeg")) {
            info = io[std::string("jpg")]->getDim(filepath, pagenum);
        } else if (mimetype == "image/png") {
            info = io[std::string("png")]->getDim(filepath, pagenum);
        } else if ((mimetype == "image/jp2") || (mimetype == "image/jpx")) {
            info = io[std::string("jpx")]->getDim(filepath, pagenum);
        } else if (mimetype == "application/pdf") {
            info = io[std::string("pdf")]->getDim(filepath, pagenum);
        }
        info.internalmimetype = mimetype;

        if (info.success == IIIFImgInfo::FAILURE) {
            for (auto const &iterator : io) {
                info = iterator.second->getDim(filepath, pagenum);
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
    void IIIFImage::getDim(size_t &width, size_t &height) const {
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
            auto outbuf = std::make_unique<byte[]>(nc*nx*ny);

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
            size_t nnc = nc - 1;
            auto outbuf = std::make_unique<word[]>(nc*nx*ny);

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

    void IIIFImage::convertToIcc(const IIIFIcc &target_icc_p, int new_bps) {
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
            case 0: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
                break;
            }
            case 8: {
                inbuf = bpixels.get();
                break;
            }
            case 16: {
                inbuf = wpixels.get();
                break;
            }
            default: {
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not supported for operation (bps={})", bps));
            }
        }

        switch (new_bps) {
            case 8: {
                auto boutbuf = std::make_unique<byte[]>(nx*ny*nnc*sizeof(byte));
                cmsDoTransform(hTransform, inbuf, boutbuf.get(), nx * ny);
                wpixels.reset();
                bpixels = std::move(boutbuf);
                break;
            }
            case 16: {
                auto woutbuf = std::make_unique<word[]>(nx*ny*nnc*sizeof(word));
                cmsDoTransform(hTransform, inbuf, woutbuf.get(), nx * ny);
                bpixels.reset();
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
    void IIIFImage::removeChan(unsigned int chan) {
        if ((nc == 1) || (chan >= nc)) {
            std::string msg = "Cannot remove component: nc=" + std::to_string(nc) + " chan=" + std::to_string(chan);
            throw IIIFImageError(file_, __LINE__, msg);
        }

        if (!es.empty()) {
            if (nc < 3) {
                es.clear(); // no more alpha channel
            } else if (nc > 3) { // it's probably an alpha channel
                if ((nc == 4) && (photo == SEPARATED)) {  // oh no – 4 channes, but CMYK
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
            auto boutbuf = std::make_unique<byte[]>(nnc*nx*ny);

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
            auto woutbuf = std::make_unique<word[]>(nnc*nx*ny);

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
    void IIIFImage::crop(int x, int y, size_t width, size_t height) {
        if (x < 0) {
            width += x;
            x = 0;
        } else if (x >= (long) nx) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cropping outside image: nx={}, x={}", nx, x));
        }

        if (y < 0) {
            height += y;
            y = 0;
        } else if (y >= (long) ny) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cropping outside image: ny={}, y={}", ny, y));
        }

        if (width == 0) {
            width = nx - x;
        } else if ((x + width) > nx) {
            width = nx - x;
        }

        if (height == 0) {
            height = ny - y;
        } else if ((y + height) > ny) {
            height = ny - y;
        }

        if ((x == 0) && (y == 0) && (width == nx) && (height == ny)) return; //we do not have to crop!!

        if (bps == 8) {
            auto boutbuf = std::make_unique<byte[]>(width * height * nc);
            for (size_t j = 0; j < height; j++) {
                for (size_t i = 0; i < width; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        boutbuf[nc * (j * width + i) + k] = bpixels[nc * ((j + y) * nx + (i + x)) + k];
                    }
                }
            }

            bpixels = std::move(boutbuf);
        } else if (bps == 16) {
            auto woutbuf = std::make_unique<word[]>(width * height * nc);

            for (size_t j = 0; j < height; j++) {
                for (size_t i = 0; i < width; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        woutbuf[nc * (j * width + i) + k] = wpixels[nc * ((j + y) * nx + (i + x)) + k];
                    }
                }
            }

            wpixels = std::move(woutbuf);
        } else {
            // clean up and throw exception
        }
        nx = width;
        ny = height;
    }



    [[maybe_unused]]
    bool IIIFImage::crop(const std::shared_ptr<IIIFRegion> &region) {
        int x, y;
        size_t width, height;
        if (region->getType() == IIIFRegion::FULL) return true; // we do not have to crop;
        region->crop_coords(nx, ny, x, y, width, height);

        if (bps == 8) {
            auto boutbuf = std::make_unique<byte[]>(width * height * nc);

            for (size_t j = 0; j < height; j++) {
                for (size_t i = 0; i < width; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        boutbuf[nc * (j * width + i) + k] = bpixels[nc * ((j + y) * nx + (i + x)) + k];
                    }
                }
            }

            bpixels = std::move(boutbuf);
        } else if (bps == 16) {
            auto woutbuf = std::make_unique<word[]>(width * height * nc);

            for (size_t j = 0; j < height; j++) {
                for (size_t i = 0; i < width; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        woutbuf[nc * (j * width + i) + k] = wpixels[nc * ((j + y) * nx + (i + x)) + k];
                    }
                }
            }

            wpixels = std::move(woutbuf);
        } else {
            // clean up and throw exception
        }

        nx = width;
        ny = height;
        return true;
    }


    /****************************************************************************/
#define POSITION(x, y, c, n) ((n)*((y)*nx + (x)) + c)

    byte IIIFImage::bilinn(const byte buf[], int nx, double x, double y, int c, int n) {
        int ix, iy;
        double rx, ry;
        ix = (int) x;
        iy = (int) y;
        rx = x - (double) ix;
        ry = y - (double) iy;

        if ((rx < 1.0e-2) && (ry < 1.0e-2)) {
            return (buf[POSITION(ix, iy, c, n)]);
        } else if (rx < 1.0e-2) {
            return ((byte) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry))));
        } else if (ry < 1.0e-2) {
            return ((byte) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                    (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry))));
        } else {
            return ((byte) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                    (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry) +
                                    (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry) +
                                    (double) buf[POSITION((ix + 1), (iy + 1), c, n)] * rx * ry)));
        }
    }

    /*==========================================================================*/

    word IIIFImage::bilinn(const word buf[], int nx, double x, double y, int c, int n) {
        int ix, iy;
        double rx, ry;
        ix = (int) x;
        iy = (int) y;
        rx = x - (double) ix;
        ry = y - (double) iy;

        if ((rx < 1.0e-2) && (ry < 1.0e-2)) {
            return (buf[POSITION(ix, iy, c, n)]);
        } else if (rx < 1.0e-2) {
            return ((word) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry))));
        } else if (ry < 1.0e-2) {
            return ((word) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry))));
        } else {
            return ((word) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry) +
                                   (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry) +
                                   (double) buf[POSITION((ix + 1), (iy + 1), c, n)] * rx * ry)));
        }
    }
    /*==========================================================================*/

#undef POSITION

    bool IIIFImage::scaleFast(size_t nnx, size_t nny) {
        auto xlut = std::make_unique<size_t[]>(nnx);
        auto ylut = std::make_unique<size_t[]>(nny);

        for (size_t i = 0; i < nnx; i++) {
            xlut[i] = (size_t) lround(i * (nx - 1) / (double) (nnx - 1));
        }
        for (size_t i = 0; i < nny; i++) {
            ylut[i] = (size_t) lround(i * (ny - 1) / (double) (nny - 1 ));
        }

        if (bps == 8) {
            auto boutbuf = std::make_unique<byte[]>(nnx * nny * nc);
            for (size_t y = 0; y < nny; y++) {
                for (size_t x = 0; x < nnx; x++) {
                    for (size_t k = 0; k < nc; k++) {
                        boutbuf[nc * (y * nnx + x) + k] = bpixels[nc * (ylut[y] * nx + xlut[x]) + k];
                    }
                }
            }
            bpixels = std::move(boutbuf) ;
        } else if (bps == 16) {
            auto woutbuf = std::make_unique<word[]>(nnx * nny * nc);
            for (size_t y = 0; y < nny; y++) {
                for (size_t x = 0; x < nnx; x++) {
                    for (size_t k = 0; k < nc; k++) {
                        woutbuf[nc * (y * nnx + x) + k] = wpixels[nc * (ylut[y] * nx + xlut[x]) + k];
                    }
                }
            }
            wpixels = std::move(woutbuf);
        } else {
            return false;
        }

        nx = nnx;
        ny = nny;
        return true;
    }

    bool IIIFImage::scaleMedium(size_t nnx, size_t nny) {
        auto xlut = std::make_unique<double[]>(nnx);
        auto ylut = std::make_unique<double[]>(nny);

        for (size_t i = 0; i < nnx; i++) {
            xlut[i] = (double) (i * (nx - 1)) / (double) (nnx - 1);
        }
        for (size_t j = 0; j < nny; j++) {
            ylut[j] = (double) (j * (ny - 1)) / (double) (nny - 1);
        }

        if (bps == 8) {
            auto boutbuf = std::make_unique<byte[]>(nnx * nny * nc);
            double rx, ry;
            auto raw_input = bpixels.get();
            for (size_t j = 0; j < nny; j++) {
                ry = ylut[j];
                for (size_t i = 0; i < nnx; i++) {
                    rx = xlut[i];
                    for (size_t k = 0; k < nc; k++) {
                        boutbuf[nc * (j * nnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                    }
                }
            }

            bpixels = std::move(boutbuf) ;
        } else if (bps == 16) {
            auto woutbuf = std::make_unique<word[]>(nnx * nny * nc);
            double rx, ry;

            auto raw_input = wpixels.get();
            for (size_t j = 0; j < nny; j++) {
                ry = ylut[j];
                for (size_t i = 0; i < nnx; i++) {
                    rx = xlut[i];
                    for (size_t k = 0; k < nc; k++) {
                        woutbuf[nc * (j * nnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                    }
                }
            }

            wpixels = std::move(woutbuf);
        } else {
            return false;
        }

        nx = nnx;
        ny = nny;
        return true;
    }
    /*==========================================================================*/


    bool IIIFImage::scale(size_t nnx, size_t nny) {
        size_t iix = 1, iiy = 1;
        size_t nnnx, nnny;

        //
        // if the scaling is less than 1 (that is, the image gets smaller), we first
        // expand it to a integer multiple of the desired size, and then we just
        // avarage the number of pixels. This is the "proper" way of downscale an
        // image...
        //
        if (nnx < nx) {
            while (nnx * iix < nx) iix++;
            nnnx = nnx * iix;
        } else {
            nnnx = nnx;
        }

        if (nny < ny) {
            while (nny * iiy < ny) iiy++;
            nnny = nny * iiy;
        } else {
            nnny = nny;
        }

        auto xlut = std::make_unique<double[]>(nnnx);
        auto ylut = std::make_unique<double[]>(nnny);

        for (size_t i = 0; i < nnnx; i++) {
            xlut[i] = (double) (i * (nx - 1)) / (double) (nnnx - 1);
        }
        for (size_t j = 0; j < nnny; j++) {
            ylut[j] = (double) (j * (ny - 1)) / (double) (nnny - 1);
        }

        if (bps == 8) {
            auto boutbuf = std::make_unique<byte[]>(nnnx * nnny * nc);
            double rx, ry;

            auto raw_input = bpixels.get();
            for (size_t j = 0; j < nnny; j++) {
                ry = ylut[j];
                for (size_t i = 0; i < nnnx; i++) {
                    rx = xlut[i];
                    for (size_t k = 0; k < nc; k++) {
                        boutbuf[nc * (j * nnnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                    }
                }
            }
            bpixels = std::move(boutbuf);
        } else if (bps == 16) {
            auto woutbuf = std::make_unique<word[]>(nnnx * nnny * nc);
            double rx, ry;

            auto raw_input = wpixels.get();
            for (size_t j = 0; j < nnny; j++) {
                ry = ylut[j];
                for (size_t i = 0; i < nnnx; i++) {
                    rx = xlut[i];
                    for (size_t k = 0; k < nc; k++) {
                        woutbuf[nc * (j * nnnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                    }
                }
            }
            wpixels = std::move(woutbuf);
        } else {
            return false;
            // clean up and throw exception
        }

        //
        // now we have to check if we have to average the pixels
        //
        if ((iix > 1) || (iiy > 1)) {
            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nnx * nny * nc);
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            unsigned int accu = 0;

                            for (size_t jj = 0; jj < iiy; jj++) {
                                for (size_t ii = 0; ii < iix; ii++) {
                                    accu += bpixels[nc * ((iiy * j + jj) * nnnx + (iix * i + ii)) + k];
                                }
                            }

                            boutbuf[nc * (j * nnx + i) + k] = accu / (iix * iiy);
                        }
                    }
                }
                bpixels = std::move(boutbuf);
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nnx * nny * nc);

                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            unsigned int accu = 0;

                            for (size_t jj = 0; jj < iiy; jj++) {
                                for (size_t ii = 0; ii < iix; ii++) {
                                    accu += wpixels[nc * ((iiy * j + jj) * nnnx + (iix * i + ii)) + k];
                                }
                            }

                            woutbuf[nc * (j * nnx + i) + k] = accu / (iix * iiy);
                        }
                    }
                }
                wpixels = std::move(woutbuf);
            }
        }
        nx = nnx;
        ny = nny;
        return true;
    }

    bool IIIFImage::rotate(float angle, bool mirror) {
        if (mirror) {
            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nx * ny * nc);
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            boutbuf[nc * (j * nx + i) + k] = bpixels[nc * (j * nx + (nx - i - 1)) + k];
                        }
                    }
                }

                bpixels = std::move(boutbuf);
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nx * ny * nc);

                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            woutbuf[nc * (j * nx + i) + k] = wpixels[nc * (j * nx + (nx - i - 1)) + k];
                        }
                    }
                }

                wpixels = std::move(woutbuf);
            } else {
                return false;
                // clean up and throw exception
            }
        }

        while (angle < 0.) angle += 360.;
        while (angle >= 360.) angle -= 360.;

        if (angle == 0.) {
            return true;
        }

        if (angle == 90.) {
            //
            // abcdef     mga
            // ghijkl ==> nhb
            // mnopqr     oic
            //            pjd
            //            qke
            //            rlf
            //
            size_t nnx = ny;
            size_t nny = nx;

            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nx * ny * nc);

                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            boutbuf[nc * (j * nnx + i) + k] = bpixels[nc * ((ny - i - 1) * nx + j) + k];
                        }
                    }
                }

                bpixels = std::move(boutbuf);
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nx * ny * nc);

                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            woutbuf[nc * (j * nnx + i) + k] = wpixels[nc * ((ny - i - 1) * nx + j) + k];
                        }
                    }
                }

                wpixels = std::move(woutbuf);
            }
            nx = nnx;
            ny = nny;
        } else if (angle == 180.) {
            //
            // abcdef     rqponm
            // ghijkl ==> lkjihg
            // mnopqr     fedcba
            //
            size_t nnx = nx;
            size_t nny = ny;
            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nx * ny * nc);
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            boutbuf[nc * (j * nnx + i) + k] = bpixels[nc * ((ny - j - 1) * nx + (nx - i - 1)) + k];
                        }
                    }
                }

                bpixels = std::move(boutbuf);
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nx * ny * nc);
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            woutbuf[nc * (j * nnx + i) + k] = wpixels[nc * ((ny - j - 1) * nx + (nx - i - 1)) + k];
                        }
                    }
                }

                wpixels = std::move(woutbuf);
            }
            nx = nnx;
            ny = nny;
        } else if (angle == 270.) {
            //
            // abcdef     flr
            // ghijkl ==> ekq
            // mnopqr     djp
            //            cio
            //            bhn
            //            agm
            //
            size_t nnx = ny;
            size_t nny = nx;

            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nx * ny * nc);
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            boutbuf[nc * (j * nnx + i) + k] = bpixels[nc * (i * nx + (nx - j - 1)) + k];
                        }
                    }
                }

                bpixels = std::move(boutbuf) ;
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nx * ny * nc);
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            woutbuf[nc * (j * nnx + i) + k] = wpixels[nc * (i * nx + (nx - j - 1)) + k];
                        }
                    }
                }
                wpixels = std::move(woutbuf);
            }
            nx = nnx;
            ny = nny;
        } else { // all other angles
            double phi = M_PI * angle / 180.0;
            double ptx = static_cast<double>(nx) / 2. - .5;
            double pty = static_cast<double>(ny) / 2. - .5;

            double si = sin(-phi);
            double co = cos(-phi);

            size_t nnx;
            size_t nny;

            if ((angle > 0.) && (angle < 90.)) {
                nnx = floor((double) nx * cos(phi) + (double) ny * sin(phi) + .5);
                nny = floor((double) nx * sin(phi) + (double) ny * cos(phi) + .5);
            } else if ((angle > 90.) && (angle < 180.)) {
                nnx = floor(-((double) nx) * cos(phi) + (double) ny * sin(phi) + .5);
                nny = floor((double) nx * sin(phi) - (double) ny * cosf(phi) + .5);
            } else if ((angle > 180.) && (angle < 270.)) {
                nnx = floor(-((double) nx) * cos(phi) - (double) ny * sin(phi) + .5);
                nny = floor(-((double) nx) * sinf(phi) - (double) ny * cosf(phi) + .5);
            } else {
                nnx = floor((double) nx * cos(phi) - (double) ny * sin(phi) + .5);
                nny = floor(-((double) nx) * sin(phi) + (double) ny * cos(phi) + .5);
            }

            double pptx = ptx * (double) nnx / (double) nx;
            double ppty = pty * (double) nny / (double) ny;

            if (bps == 8) {
                auto boutbuf = std::make_unique<byte[]>(nnx * nny * nc);
                auto raw_input = bpixels.get();
                byte bg = 0;
                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        double rx = ((double) i - pptx) * co - ((double) j - ppty) * si + ptx;
                        double ry = ((double) i - pptx) * si + ((double) j - ppty) * co + pty;

                        if ((rx < 0.0) || (rx >= (double) (nx - 1)) || (ry < 0.0) || (ry >= (double) (ny - 1))) {
                            for (size_t k = 0; k < nc; k++) {
                                boutbuf[nc * (j * nnx + i) + k] = bg;
                            }
                        } else {
                            for (size_t k = 0; k < nc; k++) {
                                boutbuf[nc * (j * nnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                            }
                        }
                    }
                }

                bpixels = std::move(boutbuf);
            } else if (bps == 16) {
                auto woutbuf = std::make_unique<word[]>(nnx * nny * nc);
                auto raw_input = wpixels.get();
                word bg = 0;

                for (size_t j = 0; j < nny; j++) {
                    for (size_t i = 0; i < nnx; i++) {
                        double rx = ((double) i - pptx) * co - ((double) j - ppty) * si + ptx;
                        double ry = ((double) i - pptx) * si + ((double) j - ppty) * co + pty;

                        if ((rx < 0.0) || (rx >= (double) (nx - 1)) || (ry < 0.0) || (ry >= (double) (ny - 1))) {
                            for (size_t k = 0; k < nc; k++) {
                                woutbuf[nc * (j * nnx + i) + k] = bg;
                            }
                        } else {
                            for (size_t k = 0; k < nc; k++) {
                                woutbuf[nc * (j * nnx + i) + k] = bilinn(raw_input, nx, rx, ry, k, nc);
                            }
                        }
                    }
                }

                wpixels = std::move(woutbuf) ;
            }
            nx = nnx;
            ny = nny;
        }
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
            auto boutbuf = std::make_unique<byte[]>(nc * nx * ny);
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        // divide pixel values by 256 using ">> 8"
                        boutbuf[nc * (j * nx + i) + k] = (wpixels[nc * (j * nx + i) + k] >> 8);
                    }
                }
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
            if (!doit && (bpixels[i] != 0) && (bpixels[i] != 255)) {
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
        int wm_nx, wm_ny, wm_nc;
        std::unique_ptr<unsigned char[]> wmbuf = read_watermark(wmfilename, wm_nx, wm_ny, wm_nc);
        if (wmbuf == nullptr) {
            throw IIIFImageError(file_, __LINE__, "Cannot read watermark file " + wmfilename);
        }

        auto xlut = std::make_unique<double[]>(nx);
        auto ylut = std::make_unique<double[]>(ny);

        for (size_t i = 0; i < nx; i++) {
            xlut[i] = (double) (wm_nx * i) / (double) nx;
        }
        for (size_t j = 0; j < ny; j++) {
            ylut[j] = (double) (wm_ny * j) / (double) ny;
        }

        auto raw_wmbuf = wmbuf.get();
        if (bps == 8) {
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    byte val = IIIFImage::bilinn(raw_wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);

                    for (size_t k = 0; k < nc; k++) {
                        double nval = (bpixels[nc * (j * nx + i) + k] / 255.) * (1.0 + val / 2550.0) + val / 2550.0;
                        bpixels[nc * (j * nx + i) + k] = (nval > 1.0) ? 255 : (unsigned char) floorl(nval * 255. + .5);
                    }
                }
            }
        } else if (bps == 16) {
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    for (size_t k = 0; k < nc; k++) {
                        byte val = bilinn(raw_wmbuf, wm_nx, xlut[i], ylut[j], 0, wm_nc);
                        double nval =
                                (wpixels[nc * (j * nx + i) + k] / 65535.0) * (1.0 + val / 655350.0) + val / 352500.;
                        wpixels[nc * (j * nx + i) + k] = (nval > 1.0) ? (word) 65535 : (word) floorl(nval * 65535. + .5);
                    }
                }
            }
        }
        return true;
    }

    /*==========================================================================*/


    IIIFImage &IIIFImage::operator-=(const IIIFImage &rhs) {
        IIIFImage *new_rhs = nullptr;

        if ((nc != rhs.nc) || (bps != rhs.bps) || (photo != rhs.photo)) {
            std::stringstream ss;
            ss << "Image op: images not compatible" << std::endl;
            ss << "Image 1:  nc: " << nc << " bps: " << bps << " photo: " << as_integer(photo) << std::endl;
            ss << "Image 2:  nc: " << rhs.nc << " bps: " << rhs.bps << " photo: " << as_integer(rhs.photo)
               << std::endl;
            throw IIIFImageError(file_, __LINE__, ss.str());
        }

        if ((nx != rhs.nx) || (ny != rhs.ny)) {
            new_rhs = new IIIFImage(rhs);
            new_rhs->scale(nx, ny);
        }

        auto diffbuf = std::make_unique<int[]>(nx * ny * nc);

        switch (bps) {
            case 8: {
                byte *rtmp = (new_rhs == nullptr) ? rhs.bpixels.get() : new_rhs->bpixels.get();
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (bpixels[nc * (j * nx + i) + k] != rtmp[nc * (j * nx + i) + k]) {
                                diffbuf[nc * (j * nx + i) + k] =
                                        bpixels[nc * (j * nx + i) + k] - rtmp[nc * (j * nx + i) + k];
                            }
                        }
                    }
                }
                break;
            }
            case 16: {
                word *rtmp = (new_rhs == nullptr) ? (word *) rhs.wpixels.get() : (word *) new_rhs->wpixels.get();
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (wpixels[nc * (j * nx + i) + k] != rtmp[nc * (j * nx + i) + k]) {
                                diffbuf[nc * (j * nx + i) + k] =
                                        wpixels[nc * (j * nx + i) + k] - rtmp[nc * (j * nx + i) + k];
                            }
                        }
                    }
                }
                break;
            }
            default: {
                delete new_rhs;
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        int min = INT_MAX;
        int max = INT_MIN;

        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {
                for (size_t k = 0; k < nc; k++) {
                    if (diffbuf[nc * (j * nx + i) + k] > max) max = diffbuf[nc * (j * nx + i) + k];
                    if (diffbuf[nc * (j * nx + i) + k] < min) min = diffbuf[nc * (j * nx + i) + k];
                }
            }
        }
        int maxmax = abs(min) > abs(max) ? abs(min) : abs(max);

        switch (bps) {
            case 8: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            bpixels[nc * (j * nx + i) + k] = (byte) ((diffbuf[nc * (j * nx + i) + k] + maxmax) *
                                                                  UCHAR_MAX / (2 * maxmax));
                        }
                    }
                }
                break;
            }
            case 16: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            wpixels[nc * (j * nx + i) + k] = (word) ((diffbuf[nc * (j * nx + i) + k] + maxmax) *
                                                                  USHRT_MAX / (2 * maxmax));
                        }
                    }
                }
                break;
            }

            default: {
                delete new_rhs;
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        delete new_rhs;
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
        IIIFImage *new_rhs = nullptr;

        if ((nc != rhs.nc) || (bps != rhs.bps) || (photo != rhs.photo)) {
            std::stringstream ss;
            ss << "Image op: images not compatible" << std::endl;
            ss << "Image 1:  nc: " << nc << " bps: " << bps << " photo: " << as_integer(photo) << std::endl;
            ss << "Image 2:  nc: " << rhs.nc << " bps: " << rhs.bps << " photo: " << as_integer(rhs.photo)
               << std::endl;
            throw IIIFImageError(file_, __LINE__, ss.str());
        }

        if ((nx != rhs.nx) || (ny != rhs.ny)) {
            new_rhs = new IIIFImage(rhs);
            new_rhs->scale(nx, ny);
        }

        auto diffbuf = std::make_unique<int[]>(nx * ny * nc);

        switch (bps) {
            case 8: {
                byte *rtmp = (new_rhs == nullptr) ? rhs.bpixels.get() : new_rhs->bpixels.get();
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (bpixels[nc * (j * nx + i) + k] != rtmp[nc * (j * nx + i) + k]) {
                                diffbuf[nc * (j * nx + i) + k] =
                                        bpixels[nc * (j * nx + i) + k] + rtmp[nc * (j * nx + i) + k];
                            }
                        }
                    }
                }
                break;
            }
            case 16: {
                word *rtmp = (new_rhs == nullptr) ? (word *) rhs.wpixels.get() : (word *) new_rhs->wpixels.get();

                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (wpixels[nc * (j * nx + i) + k] != rtmp[nc * (j * nx + i) + k]) {
                                diffbuf[nc * (j * nx + i) + k] =
                                        wpixels[nc * (j * nx + i) + k] - rtmp[nc * (j * nx + i) + k];
                            }
                        }
                    }
                }

                break;
            }

            default: {
                delete new_rhs;
                throw IIIFImageError(file_, __LINE__, fmt::format("Bits per pixels not supported (bps={})", bps));
            }
        }

        int max = INT_MIN;

        for (size_t j = 0; j < ny; j++) {
            for (size_t i = 0; i < nx; i++) {
                for (size_t k = 0; k < nc; k++) {
                    if (diffbuf[nc * (j * nx + i) + k] > max) max = diffbuf[nc * (j * nx + i) + k];
                }
            }
        }

        switch (bps) {
            case 8: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            bpixels[nc * (j * nx + i) + k] = (byte) (diffbuf[nc * (j * nx + i) + k] * UCHAR_MAX / max);
                        }
                    }
                }
                break;
            }
            case 16: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            wpixels[nc * (j * nx + i) + k] = (word) (diffbuf[nc * (j * nx + i) + k] * USHRT_MAX / max);
                        }
                    }
                }
                break;
            }
            default: {
                delete new_rhs;
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

        long long n_differences = 0;
        switch (bps) {
            case 8: {
                for (size_t j = 0; j < ny; j++) {
                    for (size_t i = 0; i < nx; i++) {
                        for (size_t k = 0; k < nc; k++) {
                            if (bpixels[nc * (j * nx + i) + k] != rhs.bpixels[nc * (j * nx + i) + k]) {
                                n_differences++;
                            }
                        }
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

        return n_differences <= 0;
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
