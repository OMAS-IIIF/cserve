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
#include <cstdlib>

#include <string>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <cerrno>

#include "../IIIFError.h"
#include "../IIIFImage.h"
#include "../IIIFImgTools.h"
#include "IIIFIOTiff.h"
#include "../../../lib/Cserve.h"

#include "tif_dir.h"  // libtiff internals; for _TIFFFieldArray

#include "Global.h"
#include "spdlog/fmt/bundled/format.h"
#include "../IIIFPhotometricInterpretation.h"

static const char file_[] = __FILE__;

#define TIFF_GET_FIELD(file, tag, var, default) {\
if (0 == TIFFGetField ((file), (tag), (var)))*(var) = (default); }

extern "C" {

typedef struct _memtiff {
    unsigned char *data;
    tsize_t size;
    tsize_t incsiz;
    tsize_t flen;
    toff_t fptr;
} MEMTIFF;

static MEMTIFF *memTiffOpen(tsize_t incsiz = 10240, tsize_t initsiz = 10240) {
    MEMTIFF *memtif;
    if ((memtif = (MEMTIFF *) malloc(sizeof(MEMTIFF))) == nullptr) {
        throw cserve::IIIFImageError(file_, __LINE__, "malloc failed", errno);
    }

    memtif->incsiz = incsiz;

    if (initsiz == 0) initsiz = incsiz;

    if ((memtif->data = (unsigned char *) malloc(initsiz * sizeof(unsigned char))) == nullptr) {
        free(memtif);
        throw cserve::IIIFImageError(file_, __LINE__, "malloc failed", errno);
    }

    memtif->size = initsiz;
    memtif->flen = 0;
    memtif->fptr = 0;
    return memtif;
}

static tsize_t memTiffReadProc(thandle_t handle, tdata_t buf, tsize_t size) {
    auto *memtif = (MEMTIFF *) handle;

    tsize_t n;

    if (((tsize_t) memtif->fptr + size) <= memtif->flen) {
        n = size;
    } else {
        n = memtif->flen - static_cast<tsize_t>(memtif->fptr);
    }

    memcpy(buf, memtif->data + memtif->fptr, n);
    memtif->fptr += n;

    return n;
}

static tsize_t memTiffWriteProc(thandle_t handle, tdata_t buf, tsize_t size) {
    auto *memtif = (MEMTIFF *) handle;

    if (((tsize_t) memtif->fptr + size) > memtif->size) {
        if ((memtif->data = (unsigned char *) realloc(memtif->data, memtif->fptr + memtif->incsiz + size)) == nullptr) {
            throw cserve::IIIFImageError(file_, __LINE__, "realloc failed", errno);
        }

        memtif->size = memtif->fptr + memtif->incsiz + size;
    }

    memcpy(memtif->data + memtif->fptr, buf, size);
    memtif->fptr += size;

    if (memtif->fptr > memtif->flen) memtif->flen = memtif->fptr;

    return size;
}

static toff_t memTiffSeekProc(thandle_t handle, toff_t off, int whence) {
    auto *memtif = (MEMTIFF *) handle;

    switch (whence) {
        case SEEK_SET: {
            if ((tsize_t) off > memtif->size) {
                if ((memtif->data = (unsigned char *) realloc(memtif->data, memtif->size + memtif->incsiz + off)) ==
                    nullptr) {
                    throw cserve::IIIFImageError(file_, __LINE__, "realloc failed", errno);
                }

                memtif->size = memtif->size + memtif->incsiz + off;
            }

            memtif->fptr = off;
            break;
        }
        case SEEK_CUR: {
            if ((tsize_t) (memtif->fptr + off) > memtif->size) {
                if ((memtif->data = (unsigned char *) realloc(memtif->data, memtif->fptr + memtif->incsiz + off)) ==
                    nullptr) {
                    throw cserve::IIIFImageError(file_, __LINE__, "realloc failed", errno);
                }

                memtif->size = memtif->fptr + memtif->incsiz + off;
            }

            memtif->fptr += off;
            break;
        }
        case SEEK_END: {
            if ((tsize_t) (memtif->size + off) > memtif->size) {
                if ((memtif->data = (unsigned char *) realloc(memtif->data, memtif->size + memtif->incsiz + off)) ==
                    nullptr) {
                    throw cserve::IIIFImageError(file_, __LINE__, "realloc failed", errno);
                }

                memtif->size = memtif->size + memtif->incsiz + off;
            }

            memtif->fptr = memtif->size + off;
            break;
        }
        default: {
        }
    }

    if (memtif->fptr > memtif->flen) memtif->flen = memtif->fptr;
    return memtif->fptr;
}

static int memTiffCloseProc(thandle_t handle) {
    auto *memtif = (MEMTIFF *) handle;
    memtif->fptr = 0;
    return 0;
}


static toff_t memTiffSizeProc(thandle_t handle) {
    auto *memtif = (MEMTIFF *) handle;
    return memtif->flen;
}


static int memTiffMapProc(thandle_t handle, tdata_t *base, toff_t *psize) {
    auto *memtif = (MEMTIFF *) handle;
    *base = memtif->data;
    *psize = memtif->flen;
    return (1);
}

static void memTiffUnmapProc(thandle_t handle, tdata_t base, toff_t size) {}

static void memTiffFree(MEMTIFF *memtif) {
    if (memtif->data != nullptr) free(memtif->data);
    memtif->data = nullptr;
    if (memtif != nullptr) free(memtif);
    memtif = nullptr;
}

}


//
// the 2 typedefs below are used to extract the EXIF-tags from a TIFF file. This is done
// using the normal libtiff functions...
//
typedef enum {
    EXIF_DT_UINT8 = 1,
    EXIF_DT_STRING = 2,
    EXIF_DT_UINT16 = 3,
    EXIF_DT_UINT32 = 4,
    EXIF_DT_RATIONAL = 5,
    EXIF_DT_2ST = 7,

    EXIF_DT_RATIONAL_PTR = 101,
    EXIF_DT_UINT8_PTR = 102,
    EXIF_DT_UINT16_PTR = 103,
    EXIF_DT_UINT32_PTR = 104,
    EXIF_DT_PTR = 105,
    EXIF_DT_UNDEFINED = 999

} ExifDataType_type;

typedef struct exif_tag_ {
    int tag_id;
    ExifDataType_type datatype;
    int len;
    union {
        float f_val;
        uint8_t c_val;
        uint16_t s_val;
        uint32_t i_val;
        char *str_val;
        float *f_ptr;
        uint8_t *c_ptr;
        uint16_t *s_ptr;
        uint32_t *i_ptr;
        void *ptr;
        unsigned char _4cc[4];
        unsigned short _2st[2];
    };
} ExifTag_type;

static ExifTag_type exiftag_list[] = {{EXIFTAG_EXPOSURETIME,             EXIF_DT_RATIONAL,   0L, {0L}},
                                      {EXIFTAG_FNUMBER,                  EXIF_DT_RATIONAL,   0L, {0L}},
                                      {EXIFTAG_EXPOSUREPROGRAM,          EXIF_DT_UINT16,     0L, {0L}},
                                      {EXIFTAG_SPECTRALSENSITIVITY,      EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_ISOSPEEDRATINGS,          EXIF_DT_UINT16_PTR, 0L, {0L}},
                                      {EXIFTAG_OECF,                     EXIF_DT_PTR,        0L, {0L}},
                                      {EXIFTAG_EXIFVERSION,              EXIF_DT_UNDEFINED,  0L, {0L}},
                                      {EXIFTAG_DATETIMEORIGINAL,         EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_DATETIMEDIGITIZED,        EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_COMPONENTSCONFIGURATION,  EXIF_DT_UNDEFINED,  0L, {1L}}, // !!!! would be 4cc
                                      {EXIFTAG_COMPRESSEDBITSPERPIXEL,   EXIF_DT_RATIONAL,   0L, {0L}},
                                      {EXIFTAG_SHUTTERSPEEDVALUE,        EXIF_DT_RATIONAL,   0L, {0L}},
                                      {EXIFTAG_APERTUREVALUE,            EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_BRIGHTNESSVALUE,          EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_EXPOSUREBIASVALUE,        EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_MAXAPERTUREVALUE,         EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_SUBJECTDISTANCE,          EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_METERINGMODE,             EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_LIGHTSOURCE,              EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_FLASH,                    EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_FOCALLENGTH,              EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_SUBJECTAREA,              EXIF_DT_UINT16_PTR, 0L, {0L}}, //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! ARRAY OF SHORTS
                                      {EXIFTAG_MAKERNOTE,                EXIF_DT_UNDEFINED,  0L, {0L}},
                                      {EXIFTAG_USERCOMMENT,              EXIF_DT_PTR,        0L, {0L}},
                                      {EXIFTAG_SUBSECTIME,               EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_SUBSECTIMEORIGINAL,       EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_SUBSECTIMEDIGITIZED,      EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_FLASHPIXVERSION,          EXIF_DT_UNDEFINED,  0L, {01L}}, // 2 SHORTS
                                      {EXIFTAG_COLORSPACE,               EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_PIXELXDIMENSION,          EXIF_DT_UINT32,     0L, {0l}}, // CAN ALSO BE UINT16 !!!!!!!!!!!!!!
                                      {EXIFTAG_PIXELYDIMENSION,          EXIF_DT_UINT32,     0L, {0l}}, // CAN ALSO BE UINT16 !!!!!!!!!!!!!!
                                      {EXIFTAG_RELATEDSOUNDFILE,         EXIF_DT_STRING,     0L, {0L}},
                                      {EXIFTAG_FLASHENERGY,              EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_SPATIALFREQUENCYRESPONSE, EXIF_DT_PTR,        0L, {0L}},
                                      {EXIFTAG_FOCALPLANEXRESOLUTION,    EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_FOCALPLANEYRESOLUTION,    EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_FOCALPLANERESOLUTIONUNIT, EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_SUBJECTLOCATION,          EXIF_DT_UINT32,     0L, {0l}}, // 2 SHORTS !!!!!!!!!!!!!!!!!!!!!!!!!!!
                                      {EXIFTAG_EXPOSUREINDEX,            EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_SENSINGMETHOD,            EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_FILESOURCE,               EXIF_DT_UINT8,      0L, {0L}},
                                      {EXIFTAG_SCENETYPE,                EXIF_DT_UINT8,      0L, {0L}},
                                      {EXIFTAG_CFAPATTERN,               EXIF_DT_PTR,        0L, {0L}},
                                      {EXIFTAG_CUSTOMRENDERED,           EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_EXPOSUREMODE,             EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_WHITEBALANCE,             EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_DIGITALZOOMRATIO,         EXIF_DT_RATIONAL,   0L, {0l}},
                                      {EXIFTAG_FOCALLENGTHIN35MMFILM,    EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_SCENECAPTURETYPE,         EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_GAINCONTROL,              EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_CONTRAST,                 EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_SATURATION,               EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_SHARPNESS,                EXIF_DT_UINT16,     0L, {0l}},
                                      {EXIFTAG_DEVICESETTINGDESCRIPTION, EXIF_DT_PTR,        0L, {0L}},
                                      {EXIFTAG_SUBJECTDISTANCERANGE,     EXIF_DT_UINT16,     0L, {0L}},
                                      {EXIFTAG_IMAGEUNIQUEID,            EXIF_DT_STRING,     0L, {0L}},};

static int exiftag_list_len = sizeof(exiftag_list) / sizeof(ExifTag_type);


namespace cserve {

    std::unique_ptr<unsigned char[]> read_watermark(const std::string &wmfile, int &nx, int &ny, int &nc) {
        TIFF *tif;
        int32_t sll;
        uint16_t spp, bps, pmi, pc;
        std::unique_ptr<unsigned char[]> wmbuf;
        nx = 0;
        ny = 0;

        if (nullptr == (tif = TIFFOpen(wmfile.c_str(), "r"))) {
            return nullptr;
        }

        if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &nx) == 0) {
            TIFFClose(tif);
            throw IIIFImageError(file_, __LINE__,
                                 "ERROR in read_watermark: TIFFGetField of TIFFTAG_IMAGEWIDTH failed: " + wmfile);
        }

        if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &ny) == 0) {
            TIFFClose(tif);
            throw IIIFImageError(file_, __LINE__,
                                 "ERROR in read_watermark: TIFFGetField of TIFFTAG_IMAGELENGTH failed: " +
                                 wmfile);
        }

        TIFF_GET_FIELD (tif, TIFFTAG_SAMPLESPERPIXEL, &spp, 1);

        if (spp != 1) {
            TIFFClose(tif);
            throw IIIFImageError(file_, __LINE__, "ERROR in read_watermark: ssp ≠ 1: " + wmfile);
        }

        TIFF_GET_FIELD (tif, TIFFTAG_BITSPERSAMPLE, &bps, 1);

        if (bps != 8) {
            TIFFClose(tif);
            throw IIIFImageError(file_, __LINE__, "ERROR in read_watermark: bps ≠ 8: " + wmfile);
        }

        TIFF_GET_FIELD (tif, TIFFTAG_PHOTOMETRIC, &pmi, PHOTOMETRIC_MINISBLACK);
        TIFF_GET_FIELD (tif, TIFFTAG_PLANARCONFIG, &pc, PLANARCONFIG_CONTIG);

        if (pc != PLANARCONFIG_CONTIG) {
            TIFFClose(tif);
            throw IIIFImageError(file_, __LINE__,
                                 "ERROR in read_watermark: Tag TIFFTAG_PLANARCONFIG is not PLANARCONFIG_CONTIG: " +
                                 wmfile);
        }

        sll = nx * spp * bps / 8;
        try {
            //wmbuf = new byte[ny * sll];
            wmbuf = std::make_unique<unsigned char[]>(ny * sll);
        } catch (std::bad_alloc &ba) {
            throw IIIFImageError(file_, __LINE__,
                                 "ERROR in read_watermark: Could not allocate memory: "); // + ba.what());
        }

        unsigned char *raw_wmbuf = wmbuf.get();
        for (int i = 0; i < ny; i++) {
            if (TIFFReadScanline(tif, raw_wmbuf + i * sll, i) == -1) {
                throw IIIFImageError(file_, __LINE__,
                                     "ERROR in read_watermark: TIFFReadScanline failed on scanline " +
                                     std::to_string(i) + " in file " + wmfile);
            }
        }

        TIFFClose(tif);
        nc = spp;

        return wmbuf;
    }
    //============================================================================


    static void tiffError(const char *module, const char *fmt, va_list argptr) {
        Server::logger()->error("ERROR IN TIFF! Module: {}", module);
        char buf[512];
        vsnprintf(buf, 511, fmt, argptr);
        buf[511] = '\0';
        Server::logger()->error(buf);
    }


    static void tiffWarning(const char *module, const char *fmt, va_list argptr) {
        Server::logger()->warn("ERROR IN TIFF! Module: {}", module);
        char buf[512];
        vsnprintf(buf, 511, fmt, argptr);
        buf[511] = '\0';
        Server::logger()->warn(buf);
    }

#define N(a) (sizeof(a) / sizeof ((a)[0]))
#define TIFFTAG_SIPIMETA 65111

    static const TIFFFieldInfo xtiffFieldInfo[] = {
            {TIFFTAG_SIPIMETA, 1, 1, TIFF_ASCII, FIELD_CUSTOM, 1, 0, const_cast<char *>("SipiEssentialMetadata")},};

    static TIFFExtendProc parent_extender = nullptr;

    static void registerCustomTIFFTags(TIFF *tif) {
        /* Install the extended Tag field info */
        TIFFMergeFieldInfo(tif, xtiffFieldInfo, N(xtiffFieldInfo));
        if (parent_extender != nullptr) (*parent_extender)(tif);
    }

    [[maybe_unused]]
    void IIIFIOTiff::initLibrary() {
        static bool done = false;
        if (!done) {
            TIFFSetErrorHandler(tiffError);
            TIFFSetWarningHandler(tiffWarning);

            parent_extender = TIFFSetTagExtender(registerCustomTIFFTags);
            done = true;
        }
    }

    IIIFImage IIIFIOTiff::read(const std::string &filepath,
                               int pagenum, std::shared_ptr<IIIFRegion> region,
                               std::shared_ptr<IIIFSize> size,
                               bool force_bps_8,
                               ScalingQuality scaling_quality) {
        IIIFImage img{};
        TIFF *tif = TIFFOpen(filepath.c_str(), "r");
        if (tif == nullptr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open TIFF file '{}'", filepath));
        }

        TIFFSetErrorHandler(tiffError);
        TIFFSetWarningHandler(tiffWarning);

        uint16_t safo, ori, planar, stmp;

        (void) TIFFSetWarningHandler(nullptr);

        if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &(img.nx)) == 0) {
            TIFFClose(tif);
            std::string msg = "TIFFGetField of TIFFTAG_IMAGEWIDTH failed: " + filepath;
            throw IIIFImageError(file_, __LINE__, msg);
        }

        if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &(img.ny)) == 0) {
            TIFFClose(tif);
            std::string msg = "TIFFGetField of TIFFTAG_IMAGELENGTH failed: " + filepath;
            throw IIIFImageError(file_, __LINE__, msg);
        }

        auto sll = (unsigned int) TIFFScanlineSize(tif);
        TIFF_GET_FIELD (tif, TIFFTAG_SAMPLESPERPIXEL, &stmp, 1);
        img.nc = static_cast<int>(stmp);

        TIFF_GET_FIELD (tif, TIFFTAG_BITSPERSAMPLE, &stmp, 1);
        img.bps = stmp;
        TIFF_GET_FIELD (tif, TIFFTAG_ORIENTATION, &ori, ORIENTATION_TOPLEFT);
        img.orientation = static_cast<Orientation>(ori);

        if (1 != TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &stmp)) {
            img.photo = MINISBLACK;
        } else {
            img.photo = (PhotometricInterpretation) stmp;
        }


        std::vector<uint16_t> rcm;
        std::vector<uint16_t> gcm;
        std::vector<uint16_t> bcm;
        int colmap_len = 0;
        if (img.photo == PALETTE) {
            uint16_t *_rcm = nullptr, *_gcm = nullptr, *_bcm = nullptr;
            if (TIFFGetField(tif, TIFFTAG_COLORMAP, &_rcm, &_gcm, &_bcm) == 0) {
                TIFFClose(tif);
                std::string msg = "TIFFGetField of TIFFTAG_COLORMAP failed: " + filepath;
                throw IIIFImageError(file_, __LINE__, msg);
            }
            colmap_len = 1;
            size_t itmp = 0;
            while (itmp < img.bps) {
                colmap_len *= 2;
                itmp++;
            }
            rcm.reserve(colmap_len);
            gcm.reserve(colmap_len);
            bcm.reserve(colmap_len);
            if (colmap_len <= 256) {
                for (int ii = 0; ii < colmap_len; ii++) {
                    rcm[ii] = _rcm[ii] >> 8;
                    gcm[ii] = _gcm[ii] >> 8;
                    bcm[ii] = _bcm[ii] >> 8;
                }
            }
            else {
                for (int ii = 0; ii < colmap_len; ii++) {
                    rcm[ii] = _rcm[ii];
                    gcm[ii] = _gcm[ii];
                    bcm[ii] = _bcm[ii];
                }
            }
        }

        TIFF_GET_FIELD (tif, TIFFTAG_PLANARCONFIG, &planar, PLANARCONFIG_CONTIG);
        TIFF_GET_FIELD (tif, TIFFTAG_SAMPLEFORMAT, &safo, SAMPLEFORMAT_UINT);

        uint16_t *es;
        int eslen = 0;
        if (TIFFGetField(tif, TIFFTAG_EXTRASAMPLES, &eslen, &es) == 1) {
            for (int i = 0; i < eslen; i++) {
                ExtraSamples extra;
                switch (es[i]) {
                    case 0:
                        extra = UNSPECIFIED;
                        break;
                    case 1:
                        extra = ASSOCALPHA;
                        break;
                    case 2:
                        extra = UNASSALPHA;
                        break;
                    default:
                        extra = UNSPECIFIED;
                }
                img.es.push_back(extra);
            }
        }

        char *str;
        if (1 == TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.ImageDescription"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_MAKE, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.Make"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_MODEL, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.Model"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_SOFTWARE, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.Software"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_DATETIME, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.DateTime"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_ARTIST, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.Artist"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_HOSTCOMPUTER, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.HostComputer"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_COPYRIGHT, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.Copyright"), std::string(str));
        }
        if (1 == TIFFGetField(tif, TIFFTAG_DOCUMENTNAME, &str)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.DocumentName"), std::string(str));
        }
        // ???????? What shall we do with this meta data which is not standard in exif??????
        // We could add it as Xmp?
        //
/*
            if (1 == TIFFGetField(tif, TIFFTAG_PAGENAME, &str)) {
                if (img->exif == NULL) img->exif = std::make_shared<SipiExif>();
                img->exif->addKeyVal(string("Exif.Image.PageName"), string(str));
            }
            if (1 == TIFFGetField(tif, TIFFTAG_PAGENUMBER, &str)) {
                if (img->exif == NULL) img->exif = std::make_shared<SipiExif>();
                img->exif->addKeyVal(string("Exif.Image.PageNumber"), string(str));
            }
*/
        float f;
        if (1 == TIFFGetField(tif, TIFFTAG_XRESOLUTION, &f)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.XResolution"), f);
        }
        if (1 == TIFFGetField(tif, TIFFTAG_YRESOLUTION, &f)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.YResolution"), f);
        }

        short s;
        if (1 == TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &s)) {
            img.ensure_exif();
            img.exif->addKeyVal(std::string("Exif.Image.ResolutionUnit"), s);
        }

        //
        // read iptc header
        //
        unsigned int iptc_length = 0;
        unsigned char *iptc_content = nullptr;

        if (TIFFGetField(tif, TIFFTAG_RICHTIFFIPTC, &iptc_length, &iptc_content) != 0) {
            try {
                img.iptc = std::make_shared<IIIFIptc>(iptc_content, iptc_length);
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }

        //
        // read exif here....
        //
        toff_t exif_ifd_offs;
        if (1 == TIFFGetField(tif, TIFFTAG_EXIFIFD, &exif_ifd_offs)) {
            img.ensure_exif();
            readExif(img, tif, exif_ifd_offs); // TODO:::::::::: change signature
            unsigned short ori;
            if (img.exif->getValByKey("Exif.Image.Orientation", ori)) {
                if (static_cast<Orientation>(ori) != img.orientation) {
                    Server::logger()->warn("Inconsistent orientation: TIFF-orientation={} EXIF-orientation={}", img.orientation, ori);
                }
             }
        }

        //
        // read xmp header
        //
        int xmp_length;
        char *xmp_content = nullptr;

        if (1 == TIFFGetField(tif, TIFFTAG_XMLPACKET, &xmp_length, &xmp_content)) {
            try {
                img.xmp = std::make_shared<IIIFXmp>(xmp_content, xmp_length);
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }

        //
        // Read ICC-profile
        //
        unsigned int icc_len;
        unsigned char *icc_buf;
        float *whitepoint_ti = nullptr;
        float whitepoint[2];

        if (1 == TIFFGetField(tif, TIFFTAG_ICCPROFILE, &icc_len, &icc_buf)) {
            try {
                img.icc = std::make_shared<IIIFIcc>(icc_buf, icc_len);
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        } else if (1 == TIFFGetField(tif, TIFFTAG_WHITEPOINT, &whitepoint_ti)) {
            whitepoint[0] = whitepoint_ti[0];
            whitepoint[1] = whitepoint_ti[1];
            //
            // Wow, we have TIFF colormetry..... Who is still using this???
            //
            float *primaries_ti = nullptr;
            float primaries[6];

            if (1 == TIFFGetField(tif, TIFFTAG_PRIMARYCHROMATICITIES, &primaries_ti)) {
                primaries[0] = primaries_ti[0];
                primaries[1] = primaries_ti[1];
                primaries[2] = primaries_ti[2];
                primaries[3] = primaries_ti[3];
                primaries[4] = primaries_ti[4];
                primaries[5] = primaries_ti[5];
            } else {
                //
                // not defined, let's take the sRGB primaries
                //
                primaries[0] = 0.6400;
                primaries[1] = 0.3300;
                primaries[2] = 0.3000;
                primaries[3] = 0.6000;
                primaries[4] = 0.1500;
                primaries[5] = 0.0600;
            }

            unsigned short *tfunc = new unsigned short[3 * (1 << img.bps)], *tfunc_ti;
            unsigned int tfunc_len, tfunc_len_ti;

            if (1 == TIFFGetField(tif, TIFFTAG_TRANSFERFUNCTION, &tfunc_len_ti, &tfunc_ti)) {
                if ((tfunc_len_ti / (1 << img.bps)) == 1) {
                    memcpy(tfunc, tfunc_ti, tfunc_len_ti);
                    memcpy(tfunc + tfunc_len_ti, tfunc_ti, tfunc_len_ti);
                    memcpy(tfunc + 2 * tfunc_len_ti, tfunc_ti, tfunc_len_ti);
                    tfunc_len = tfunc_len_ti;
                } else {
                    memcpy(tfunc, tfunc_ti, tfunc_len_ti);
                    tfunc_len = tfunc_len_ti / 3;
                }
            } else {
                delete[] tfunc;
                tfunc = nullptr;
                tfunc_len = 0;
            }

            img.icc = std::make_shared<IIIFIcc>(whitepoint, primaries, tfunc, tfunc_len);
            delete[] tfunc;
        }

        //
        // Read SipiEssential metadata
        //
        char *emdatastr;
        if (1 == TIFFGetField(tif, TIFFTAG_SIPIMETA, &emdatastr)) {
            IIIFEssentials se(emdatastr);
            img.essential_metadata(se);
        }

        short compression{COMPRESSION_NONE};
        TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);


        std::unique_ptr<uint8_t[]> inbuf;

        if ((region == nullptr) || (region->getType() == IIIFRegion::FULL) || (compression != COMPRESSION_NONE)) {
            if (planar == PLANARCONFIG_CONTIG) { // RGBRGBRGBRGB...
                uint32_t i;
                inbuf = std::make_unique<uint8_t[]>(img.ny * sll);

                auto *raw_inbuf = inbuf.get();
                for (i = 0; i < img.ny; i++) {
                    if (TIFFReadScanline(tif, raw_inbuf + i * sll, i, 0) == -1) {
                        TIFFClose(tif);
                        throw IIIFImageError(file_, __LINE__,
                                             fmt::format("TIFFReadScanline failed on scanline {} in file '{}'", i, filepath));
                    }
                }
            } else if (planar == PLANARCONFIG_SEPARATE) { // RRRRR…RRR GGGGG…GGGG BBBBB…BBB
                inbuf = std::make_unique<uint8_t[]>(img.nc * img.ny * sll);
                auto *raw_inbuf = inbuf.get();
                for (uint32_t j = 0; j < img.nc; j++) {
                    for (uint32_t i = 0; i < img.ny; i++) {
                        if (TIFFReadScanline(tif, raw_inbuf + j * img.ny * sll + i * sll, i, j) == -1) {
                            TIFFClose(tif);
                            throw IIIFImageError(file_, __LINE__,
                                                 fmt::format("TIFFReadScanline failed on scanline {} in file '{}'", i, filepath));
                        }
                    }
                }
                //
                // rearrange the data to RGBRGBRGB…RGB
                //
                inbuf = separateToContig(std::move(inbuf), img.nx, img.ny, img.nc, sll);
            }
            if ((region != nullptr) && (region->getType() != IIIFRegion::FULL)) {
                inbuf = crop(std::move(inbuf), img.nx, img.ny, img.nc, region);
            }
        } else {
            int roi_x, roi_y;
            size_t roi_w, roi_h;
            region->crop_coords(img.nx, img.ny, roi_x, roi_y, roi_w, roi_h);
            int ps; // pixel size in bytes

            switch (img.bps) {
                case 1: {
                    std::string msg = "Images with 1 bit/sample not supported in file " + filepath;
                    throw IIIFImageError(file_, __LINE__, msg);
                }
                case 8: {
                    ps = 1;
                    break;
                }
                case 16: {
                    ps = 2;
                    break;
                }
            }

            inbuf = std::make_unique<uint8_t[]>(ps * roi_w * roi_h * img.nc);
            auto *raw_inbuf = inbuf.get();
            auto dataptr = std::make_unique<uint8_t[]>(ps * roi_w * roi_h * img.nc);
            auto *raw_dataptr = dataptr.get();

            if (planar == PLANARCONFIG_CONTIG) { // RGBRGBRGBRGBRGBRGBRGBRGB
                for (uint32_t i = 0; i < roi_h; i++) {
                    if (TIFFReadScanline(tif, raw_dataptr, roi_y + i, 0) == -1) {
                        TIFFClose(tif);
                        std::string msg =
                                "TIFFReadScanline failed on scanline " + std::to_string(i) + " in file " + filepath;
                        throw IIIFImageError(file_, __LINE__, msg);
                    }

                    memcpy(raw_inbuf + ps * i * roi_w * img.nc, raw_dataptr + ps * roi_x * img.nc, ps * roi_w * img.nc);
                }
                img.nx = roi_w;
                img.ny = roi_h;
            } else if (planar == PLANARCONFIG_SEPARATE) { // RRRRR…RRR GGGGG…GGGG BBBBB…BBB
                for (uint32_t j = 0; j < img.nc; j++) {
                    for (uint32_t i = 0; i < roi_h; i++) {
                        if (TIFFReadScanline(tif, raw_dataptr, roi_y + i, j) == -1) {
                            TIFFClose(tif);
                            std::string msg =
                                    "TIFFReadScanline failed on scanline " + std::to_string(i) + " in file " +
                                    filepath;
                            throw IIIFImageError(file_, __LINE__, msg);
                        }

                        memcpy(raw_inbuf + ps * roi_w * (j * roi_h + i), raw_dataptr + ps * roi_x, ps * roi_w);
                    }
                }
                img.nx = roi_w;
                img.ny = roi_h;
                //
                // rearrange the data to RGBRGBRGB…RGB
                //
                if (img.bps <= 8) {
                    inbuf = separateToContig(std::move(inbuf), img.nx, img.ny, img.nc, roi_w * ps); // convert to RGBRGBRGB...
                } else {
                    auto *raw_inbuf16 = (uint16_t *) inbuf.release();
                    std::unique_ptr<uint16_t[]> inbuf16(raw_inbuf16);
                    inbuf16 = separateToContig(std::move(inbuf16), img.nx, img.ny, img.nc, roi_w * ps); // convert to RGBRGBRGB...
                    auto *raw_tmpptr8 = (uint8_t *) inbuf16.release();
                    std::unique_ptr<uint8_t[]> tmpptr8(raw_tmpptr8);
                    inbuf = std::move(tmpptr8);
                }
            }
        }
        TIFFClose(tif);

        if (img.photo == PALETTE) {
            //
            // ok, we have a palette color image we have to convert to RGB...
            //
            uint16_t cm_max = 0;
            for (int i = 0; i < colmap_len; i++) {
                if (rcm[i] > cm_max) cm_max = rcm[i];
                if (gcm[i] > cm_max) cm_max = gcm[i];
                if (bcm[i] > cm_max) cm_max = bcm[i];
            }
            if (colmap_len <= 256) { // we have bps <= 8
                if (cm_max < 256) { // the range of the colormap values is 0-255
                    auto dataptr = std::make_unique<uint8_t[]>(3 * img.nx * img.ny);
                    for (size_t i = 0; i < img.nx * img.ny; i++) {
                        dataptr[3 * i] = (uint8_t) rcm[inbuf[i]];
                        dataptr[3 * i + 1] = (uint8_t) gcm[inbuf[i]];
                        dataptr[3 * i + 2] = (uint8_t) bcm[inbuf[i]];
                    }
                    inbuf = std::move(dataptr);
                    img.bps = 8;
                }
                else { // the range of the colormap values greater than 0-255, e.g. 0-65535
                    auto dataptr = std::make_unique<uint16_t[]>(3 * img.nx * img.ny);
                    for (size_t i = 0; i < img.nx * img.ny; i++) {
                        dataptr[3 * i] = rcm[inbuf[i]];
                        dataptr[3 * i + 1] = gcm[inbuf[i]];
                        dataptr[3 * i + 2] = bcm[inbuf[i]];
                    }
                    auto *raw_tmpptr = (uint8_t *) dataptr.release();
                    std::unique_ptr<uint8_t[]> tmpptr(raw_tmpptr);
                    inbuf = std::move(tmpptr);
                    img.bps = 16;
                }
            }
            else { // we have between 9 and 16 bps
                auto *raw_inbuf = (uint16_t *) inbuf.release();
                std::unique_ptr<uint16_t[]> inbuf16(raw_inbuf);
                if (cm_max < 256) { // the range of the colormap values is 0-255
                    auto dataptr = std::make_unique<uint8_t[]>(3 * img.nx * img.ny);
                    for (size_t i = 0; i < img.nx * img.ny; i++) {
                        dataptr[3 * i] = (uint8_t) rcm[inbuf16[i]];
                        dataptr[3 * i + 1] = (uint8_t) gcm[inbuf16[i]];
                        dataptr[3 * i + 2] = (uint8_t) bcm[inbuf16[i]];
                    }
                    inbuf = std::move(dataptr);
                    img.bps = 8;
                }
                else { // the range of the colormap values greater than 0-255, e.g. 0-65535
                    auto dataptr = std::make_unique<uint16_t[]>(3 * img.nx * img.ny);
                    for (size_t i = 0; i < img.nx * img.ny; i++) {
                        dataptr[3 * i] = rcm[inbuf16[i]];
                        dataptr[3 * i + 1] = gcm[inbuf16[i]];
                        dataptr[3 * i + 2] = bcm[inbuf16[i]];
                    }
                    auto *raw_tmpptr = (uint8_t *) dataptr.release();
                    std::unique_ptr<uint8_t[]> tmpptr(raw_tmpptr);
                    inbuf = std::move(tmpptr);
                    img.bps = 16;
                }
            }
            img.photo = RGB;
            img.nc = 3;
        }

        if (img.bps <= 8) {
            img.bpixels = std::move(inbuf);
        } else {
            auto *raw_tmpptr = (uint16_t *) inbuf.release();
            std::unique_ptr<uint16_t[]> inbuf16(raw_tmpptr);
            img.wpixels = std::move(inbuf16);
        }


        if (img.icc == nullptr) {
            switch (img.photo) {
                case MINISBLACK: {
                    if (img.bps == 1) {
                        std::unique_ptr<byte[]> tmp = cvrt1BitTo8Bit(img, sll, 0, 255);
                        img.bpixels = std::move(tmp);
                        img.bps = 8;
                    }
                    img.icc = std::make_shared<IIIFIcc>(icc_GRAY_D50);
                    break;
                }
                case MINISWHITE: {
                    if (img.bps == 1) {
                        std::unique_ptr<byte[]> tmp = cvrt1BitTo8Bit(img, sll, 255, 0);
                        img.bpixels = std::move(tmp);
                        img.bps = 8;
                    }
                    img.icc = std::make_shared<IIIFIcc>(icc_GRAY_D50);
                    break;
                }
                case SEPARATED: {
                    img.icc = std::make_shared<IIIFIcc>(icc_CYMK_standard);
                    break;
                }
                case YCBCR: // fall through!
                case RGB: {
                    img.icc = std::make_shared<IIIFIcc>(icc_sRGB);
                    break;
                }
                case CIELAB: {
                    //
                    // we have to convert to JPEG2000/littleCMS standard
                    //
                    if (img.bps == 8) {
                        for (size_t y = 0; y < img.ny; y++) {
                            for (size_t x = 0; x < img.nx; x++) {
                                union {
                                    unsigned char u;
                                    signed char s;
                                } v{};
                                v.u = img.bpixels[img.nc * (y * img.nx + x) + 1];
                                img.bpixels[img.nc * (y * img.nx + x) + 1] = 128 + v.s;
                                v.u = img.bpixels[img.nc * (y * img.nx + x) + 2];
                                img.bpixels[img.nc * (y * img.nx + x) + 2] = 128 + v.s;
                            }
                        }
                        img.icc = std::make_shared<IIIFIcc>(icc_LAB);
                    } else if (img.bps == 16) {
                        for (size_t y = 0; y < img.ny; y++) {
                            for (size_t x = 0; x < img.nx; x++) {
                                union {
                                    unsigned short u;
                                    signed short s;
                                } v{};
                                v.u = img.wpixels[img.nc * (y * img.nx + x) + 1];
                                img.wpixels[img.nc * (y * img.nx + x) + 1] = 32768 + v.s;
                                v.u = img.wpixels[img.nc * (y * img.nx + x) + 2];
                                img.wpixels[img.nc * (y * img.nx + x) + 2] = 32768 + v.s;
                            }
                        }
                        img.icc = std::make_shared<IIIFIcc>(icc_LAB);
                    } else {
                        throw IIIFImageError(file_, __LINE__, fmt::format("Unsupported bits per sample (bps={})", img.bps));
                    }
                    break;
                }

                default: {
                    throw IIIFImageError(file_, __LINE__, fmt::format("Unsupported photometric interpretation (photo={})", img.photo));
                }
            }
        }
        /*
        if ((img->nc == 3) && (img->photo == PHOTOMETRIC_YCBCR)) {
            std::shared_ptr<SipiIcc> target_profile = std::make_shared<SipiIcc>(img->icc);
            switch (img->bps) {
                case 8: {
                    img->convertToIcc(target_profile, TYPE_YCbCr_8);
                    break;
                }
                case 16: {
                    img->convertToIcc(target_profile, TYPE_YCbCr_16);
                    break;
                }
                default: {
                    throw Sipi::SipiImageError(file_, __LINE__, "Unsupported bits/sample (" + std::to_string(bps) + ")!");
                }
            }
        }
        else if ((img->nc == 4) && (img->photo == PHOTOMETRIC_SEPARATED)) { // CMYK image
            std::shared_ptr<SipiIcc> target_profile = std::make_shared<SipiIcc>(icc_sRGB);
            switch (img->bps) {
                case 8: {
                    img->convertToIcc(target_profile, TYPE_CMYK_8);
                    break;
                }
                case 16: {
                    img->convertToIcc(target_profile, TYPE_CMYK_16);
                    break;
                }
                default: {
                    throw Sipi::SipiImageError(file_, __LINE__, "Unsupported bits/sample (" + std::to_string(bps) + ")!");
                }
            }
        }
        */
        //
        // resize/Scale the image if necessary
        //
        if (size != nullptr) {
            size_t nnx, nny;
            int reduce = -1;
            bool redonly;
            IIIFSize::SizeType rtype = size->get_size(img.nx, img.ny, nnx, nny, reduce, redonly);
            if (rtype != IIIFSize::FULL) {
                switch (scaling_quality.jpeg) {
                    case HIGH:
                        img.scale(nnx, nny);
                        break;
                    case MEDIUM:
                        img.scaleMedium(nnx, nny);
                        break;
                    case LOW:
                        img.scaleFast(nnx, nny);
                }
            }
        }
        if (force_bps_8) {
            if (!img.to8bps()) {
                throw IIIFImageError(file_, __LINE__, "Cannont convert to 8 Bits(sample");
            }
        }
        return img;
    }


    IIIFImgInfo IIIFIOTiff::getDim(const std::string &filepath, int pagenum) {
        TIFF *tif;
        IIIFImgInfo info;
        if (nullptr != (tif = TIFFOpen(filepath.c_str(), "r"))) {
            //
            // OK, it's a TIFF file
            //
            (void) TIFFSetWarningHandler(nullptr);
            unsigned int tmp_width;
            if (TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &tmp_width) == 0) {
                TIFFClose(tif);
                throw IIIFImageError(file_, __LINE__, fmt::format("TIFFGetField of TIFFTAG_IMAGEWIDTH failed: '{}'", filepath));
            }
            info.width = static_cast<int>(tmp_width);
            unsigned int tmp_height;
            if (TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &tmp_height) == 0) {
                TIFFClose(tif);
                throw IIIFImageError(file_, __LINE__, fmt::format("TIFFGetField of TIFFTAG_IMAGELENGTH failed: '{}'", filepath));
            }
            info.height = tmp_height;
            info.success = IIIFImgInfo::DIMS;

            unsigned short ori;
            TIFF_GET_FIELD (tif, TIFFTAG_ORIENTATION, &ori, ORIENTATION_TOPLEFT);
            info.orientation = static_cast<Orientation>(ori);

            char *emdatastr;
            if (1 == TIFFGetField(tif, TIFFTAG_SIPIMETA, &emdatastr)) {
                IIIFEssentials se(emdatastr);
                info.origmimetype = se.mimetype();
                info.origname = se.origname();
                info.success = IIIFImgInfo::ALL;
            }

            TIFFClose(tif);
        }
        return info;
    }


    void IIIFIOTiff::write(IIIFImage &img, const std::string &filepath, const IIIFCompressionParams &params) {
        TIFF *tif;
        MEMTIFF *memtif = nullptr;
        auto rowsperstrip = (uint32_t) -1;
        if ((filepath == "stdout:") || (filepath == "HTTP")) {
            memtif = memTiffOpen();
            tif = TIFFClientOpen("MEMTIFF", "w", (thandle_t) memtif, memTiffReadProc, memTiffWriteProc, memTiffSeekProc,
                                 memTiffCloseProc, memTiffSizeProc, memTiffMapProc, memTiffUnmapProc);
        } else {
            if ((tif = TIFFOpen(filepath.c_str(), "w")) == nullptr) {
                if (memtif != nullptr) memTiffFree(memtif);
                std::string msg = "TIFFopen of \"" + filepath + "\" failed!";
                throw IIIFImageError(file_, __LINE__, msg);
            }
        }
        TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (int) img.nx);
        TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (int) img.ny);
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, TIFFDefaultStripSize(tif, rowsperstrip));
        TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
        TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
        bool its_1_bit = false;
        if ((img.photo == PhotometricInterpretation::MINISWHITE) ||
            (img.photo == PhotometricInterpretation::MINISBLACK)) {
            its_1_bit = true;

            if (img.bps == 8) {
                for (size_t i = 0; i < img.nx * img.ny; i++) {
                    if ((img.bpixels[i] != 0) && (img.bpixels[i] != 255)) {
                        its_1_bit = false;
                    }
                }
            } else if (img.bps == 16) {
                for (size_t i = 0; i < img.nx * img.ny; i++) {
                    if ((img.wpixels[i] != 0) && (img.wpixels[i] != 65535)) {
                        its_1_bit = false;
                    }
                }
            }

            if (its_1_bit) {
                TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t) 1);
                TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_CCITTFAX4); // that's out default....
            } else {
                TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t) img.bps);
            }
        } else {
            TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, (uint16_t) img.bps);
            try {
                std::string compression = params.at(TIFF_COMPRESSION);
                if (compression == "COMPRESSION_LZW") {
                    TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
                }
            }
            catch (const std::out_of_range &err) { }
        }
        if (img.photo == PhotometricInterpretation::CIELAB) {
            if (img.bps == 8) {
                for (size_t y = 0; y < img.ny; y++) {
                    for (size_t x = 0; x < img.nx; x++) {
                        union {
                            unsigned char u;
                            signed char s;
                        } v{};
                        v.s = img.bpixels[img.nc * (y * img.nx + x) + 1] - 128;
                        img.bpixels[img.nc * (y * img.nx + x) + 1] = v.u;
                        v.s = img.bpixels[img.nc * (y * img.nx + x) + 2] - 128;
                        img.bpixels[img.nc * (y * img.nx + x) + 2] = v.u;
                    }
                }
            } else if (img.bps == 16) {
                for (size_t y = 0; y < img.ny; y++) {
                    for (size_t x = 0; x < img.nx; x++) {
                        union {
                            unsigned short u;
                            signed short s;
                        } v{};
                        v.s = img.wpixels[img.nc * (y * img.nx + x) + 1] - 32768;
                        img.wpixels[img.nc * (y * img.nx + x) + 1] = v.u;
                        v.s = img.wpixels[img.nc * (y * img.nx + x) + 2] - 32768;
                        img.wpixels[img.nc * (y * img.nx + x) + 2] = v.u;
                    }
                }
            } else {
                throw IIIFImageError(file_, __LINE__, fmt::format("Unsupported bits per sample (bps={})", img.bps));
            }

            //delete img->icc; we don't want to add the ICC profile in this case (doesn't make sense!)
            img.icc = nullptr;
        }
        TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, img.nc);

        if (img.es.size() > 0) {
            TIFFSetField(tif, TIFFTAG_EXTRASAMPLES, img.es.size(), img.es.data());
        }

        TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, img.photo);

        //
        // let's get the TIFF metadata if there is some. We stored the TIFF metadata in the exifData meber variable!
        //
        if ((img.exif != nullptr) & (!(img.skip_metadata & SKIP_EXIF))) {
            std::string value;

            if (img.exif->getValByKey("Exif.Image.ImageDescription", value)) {
                TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.Make", value)) {
                TIFFSetField(tif, TIFFTAG_MAKE, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.Model", value)) {
                TIFFSetField(tif, TIFFTAG_MODEL, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.Software", value)) {
                TIFFSetField(tif, TIFFTAG_SOFTWARE, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.DateTime", value)) {
                TIFFSetField(tif, TIFFTAG_DATETIME, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.Artist", value)) {
                TIFFSetField(tif, TIFFTAG_ARTIST, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.HostComputer", value)) {
                TIFFSetField(tif, TIFFTAG_HOSTCOMPUTER, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.Copyright", value)) {
                TIFFSetField(tif, TIFFTAG_COPYRIGHT, value.c_str());
            }

            if (img.exif->getValByKey("Exif.Image.DocumentName", value)) {
                TIFFSetField(tif, TIFFTAG_DOCUMENTNAME, value.c_str());
            }

            float f;

            if (img.exif->getValByKey("Exif.Image.XResolution", f)) {
                TIFFSetField(tif, TIFFTAG_XRESOLUTION, f);
            }

            if (img.exif->getValByKey("Exif.Image.YResolution", f)) {
                TIFFSetField(tif, TIFFTAG_XRESOLUTION, f);
            }

            short s;

            if (img.exif->getValByKey("Exif.Image.ResolutionUnit", s)) {
                TIFFSetField(tif, TIFFTAG_RESOLUTIONUNIT, s);
            }
        }
        IIIFEssentials es = img.essential_metadata();
        if (((img.icc != nullptr) || es.use_icc()) & (!(img.skip_metadata & SKIP_ICC))) {
            std::vector<unsigned char> buf;
            try {
                if (es.use_icc()) {
                    buf = es.icc_profile();
                } else {
                    buf = img.icc->iccBytes();
                }

                if (buf.size() > 0) {
                    TIFFSetField(tif, TIFFTAG_ICCPROFILE, buf.size(), buf.data());
                }
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }
        //
        // write IPTC data, if available
        //
        if ((img.iptc != nullptr) & (!(img.skip_metadata & SKIP_IPTC))) {
            try {
                std::vector<unsigned char> buf = img.iptc->iptcBytes();
                if (buf.size() > 0) {
                    TIFFSetField(tif, TIFFTAG_RICHTIFFIPTC, buf.size(), buf.data());
                }
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }

        //
        // write XMP data
        //
        if ((img.xmp != nullptr) & (!(img.skip_metadata & SKIP_XMP))) {
            try {
                std::string buf = img.xmp->xmpBytes();
                if (!buf.empty() > 0) {
                    TIFFSetField(tif, TIFFTAG_XMLPACKET, buf.size(), buf.c_str());
                }
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }
        //
        // Custom tag for SipiEssential metadata
        //
        if (es.is_set()) {
            std::string emdata = es;
            TIFFSetField(tif, TIFFTAG_SIPIMETA, emdata.c_str());
        }
        //TIFFCheckpointDirectory(tif);
        if (its_1_bit) {
            unsigned int sll;
            std::unique_ptr<byte[]> buf = cvrt8BitTo1bit(img, sll);
            byte *raw_buf = buf.get();
            for (size_t i = 0; i < img.ny; i++) {
                TIFFWriteScanline(tif, raw_buf + i * sll, (int) i, 0);
            }

        } else if (img.bps == 8 ) {
            uint8_t *rawdata = img.bpixels.get();
            for (size_t i = 0; i < img.ny; i++) {
                TIFFWriteScanline(tif, rawdata + i * img.nc * img.nx, (int) i, 0);
            }
        } else if (img.bps == 16) {
            uint16_t *rawdata = (uint16_t *) img.wpixels.get();
            for (size_t i = 0; i < img.ny; i++) {
                TIFFWriteScanline(tif, rawdata + i * img.nc * img.nx, (int) i, 0);
            }
        }
        //
        // write exif data
        //
        if (img.exif != nullptr) {
            TIFFWriteDirectory(tif);
            writeExif(img, tif);
        }
        TIFFClose(tif);

        if (memtif != nullptr) {
            if (filepath == "stdout:") {
                size_t n = 0;

                while (n < memtif->flen) {
                    n += fwrite(&(memtif->data[n]), 1, memtif->flen - n > 10240 ? 10240 : memtif->flen - n, stdout);
                }

                fflush(stdout);
            } else if (filepath == "HTTP") {
                try {
                    img.connection()->sendAndFlush(memtif->data, memtif->flen);
                } catch (int i) {
                    memTiffFree(memtif);
                    throw IIIFImageError(file_, __LINE__,
                                         "Sending data failed! Broken pipe?: " + filepath + " !");
                }
            } else {
                memTiffFree(memtif);
                throw IIIFImageError(file_, __LINE__, "Unknown output method: " + filepath + " !");
            }

            memTiffFree(memtif);
        }
    }

    void IIIFIOTiff::readExif(IIIFImage &img, TIFF *tif, toff_t exif_offset) {
        uint16_t curdir = TIFFCurrentDirectory(tif);

        if (TIFFReadEXIFDirectory(tif, exif_offset)) {
            for (int i = 0; i < exiftag_list_len; i++) {
                switch (exiftag_list[i].datatype) {
                    case EXIF_DT_RATIONAL: {
                        float f;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &f)) {
                            Exiv2::Rational r = IIIFExif::toRational(f);
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", r);
                        }
                        break;
                    }
                    case EXIF_DT_UINT8: {
                        unsigned char uc;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &uc)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", uc);
                        }
                        break;
                    }
                    case EXIF_DT_UINT16: {
                        unsigned short us;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &us)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", us);
                        }
                        break;
                    }
                    case EXIF_DT_UINT32: {
                        unsigned int ui;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &ui)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", ui);
                        }
                        break;
                    }
                    case EXIF_DT_STRING: {
                        char *tmpstr = nullptr;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &tmpstr)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpstr);
                        }
                        break;
                    }
                    case EXIF_DT_RATIONAL_PTR: {
                        float *tmpbuf;
                        uint16_t len;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &len, &tmpbuf)) {
                            auto *r = new Exiv2::Rational[len];
                            for (int i; i < len; i++) {
                                r[i] = IIIFExif::toRational(tmpbuf[i]);
                            }
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", r, len);
                            delete[] r;
                        }
                        break;
                    }
                    case EXIF_DT_UINT8_PTR: {
                        uint8_t *tmpbuf;
                        uint16_t len;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &len, &tmpbuf)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpbuf, len);
                        }
                        break;
                    }
                    case EXIF_DT_UINT16_PTR: {
                        uint16_t *tmpbuf;
                        uint16_t len; // in bytes !!
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &len, &tmpbuf)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpbuf, len);
                        }
                        break;
                    }
                    case EXIF_DT_UINT32_PTR: {
                        uint32_t *tmpbuf;
                        uint16_t len;
                        if (TIFFGetField(tif, exiftag_list[i].tag_id, &len, &tmpbuf)) {
                            img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpbuf, len);
                        }
                        break;
                    }
                    case EXIF_DT_PTR: {
                        unsigned char *tmpbuf;
                        uint16_t len;

                        if (exiftag_list[i].len == 0) {
                            if (TIFFGetField(tif, exiftag_list[i].tag_id, &len, &tmpbuf)) {
                                img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpbuf, len);
                            }
                        } else {
                            len = exiftag_list[i].len;
                            if (TIFFGetField(tif, exiftag_list[i].tag_id, &tmpbuf)) {
                                img.exif->addKeyVal(exiftag_list[i].tag_id, "Photo", tmpbuf, len);
                            }
                        }
                        break;
                    }
                    default: {
                        // NO ACTION HERE At THE MOMENT...
                    }
                }
            }
        }
        TIFFSetDirectory(tif, curdir);
    }


    void IIIFIOTiff::writeExif(const IIIFImage &img, TIFF *tif) {
        // add EXIF tags to the set of tags that libtiff knows about
        // necessary if we want to set EXIFTAG_DATETIMEORIGINAL, for example
        //const TIFFFieldArray *exif_fields = _TIFFGetExifFields();
        //_TIFFMergeFields(tif, exif_fields->fields, exif_fields->count);


        TIFFCreateEXIFDirectory(tif);
        int count = 0;
        for (int i = 0; i < exiftag_list_len; i++) {
            switch (exiftag_list[i].datatype) {
                case EXIF_DT_RATIONAL: {
                    Exiv2::Rational r;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", r)) {
                        float f = (float) r.first / (float) r.second;
                        TIFFSetField(tif, exiftag_list[i].tag_id, f);
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT8: {
                    uint8_t uc;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", uc)) {
                        TIFFSetField(tif, exiftag_list[i].tag_id, uc);
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT16: {
                    uint16_t us;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", us)) {
                        TIFFSetField(tif, exiftag_list[i].tag_id, us);
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT32: {
                    uint32_t ui;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", ui)) {
                        TIFFSetField(tif, exiftag_list[i].tag_id, ui);
                        count++;
                    }
                    break;
                }
                case EXIF_DT_STRING: {
                    std::string tmpstr;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", tmpstr)) {
                        TIFFSetField(tif, exiftag_list[i].tag_id, tmpstr.c_str());
                        count++;
                    }
                    break;
                }
                case EXIF_DT_RATIONAL_PTR: {
                    std::vector<Exiv2::Rational> vr;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", vr)) {
                        int len = vr.size();
                        auto *f = new float[len];
                        for (int i = 0; i < len; i++) {
                            f[i] = (float) vr[i].first / (float) vr[i].second; //!!!!!!!!!!!!!!!!!!!!!!!!!
                        }
                        TIFFSetField(tif, exiftag_list[i].tag_id, len, f);
                        delete[] f;
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT8_PTR: {
                    std::vector<uint8_t> vuc;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", vuc)) {
                        size_t len = vuc.size();
                        TIFFSetField(tif, exiftag_list[i].tag_id, len, vuc.data());
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT16_PTR: {
                    std::vector<uint16_t> vus;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", vus)) {
                        int len = vus.size();
                        TIFFSetField(tif, exiftag_list[i].tag_id, len, vus.data());
                        count++;
                    }
                    break;
                }
                case EXIF_DT_UINT32_PTR: {
                    std::vector<uint32_t> vui;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", vui)) {
                        int len = vui.size();
                        TIFFSetField(tif, exiftag_list[i].tag_id, len, vui.data());
                        count++;
                    }
                    break;
                }
                case EXIF_DT_PTR: {
                    std::vector<unsigned char> vuc;
                    if (img.exif->getValByKey(exiftag_list[i].tag_id, "Photo", vuc)) {
                        int len = vuc.size();
                        TIFFSetField(tif, exiftag_list[i].tag_id, len, vuc.data());
                        count++;
                    }

                    break;
                }

                default: {
                    // NO ACTION HERE AT THE MOMENT...
                }
            }
        }

        if (count > 0) {
            uint64_t exif_dir_offset = 0L;
            TIFFWriteCustomDirectory(tif, &exif_dir_offset);
            TIFFSetDirectory(tif, 0);
            TIFFSetField(tif, TIFFTAG_EXIFIFD, exif_dir_offset);
        }
        //TIFFCheckpointDirectory(tif);
    }
    //============================================================================


}
