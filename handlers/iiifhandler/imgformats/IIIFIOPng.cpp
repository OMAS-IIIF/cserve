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
#include <vector>
#include <cstdio>
#include <cmath>
#include <syslog.h>
#include <cstring>

#include <png.h>
#include <zlib.h>

#include "IIIFIOPng.h"
#include "../../../lib/Cserve.h"


// bad hack in order to include definitions in png.h on debian systems
#if !defined(PNG_TEXT_SUPPORTED)
#  define PNG_TEXT_SUPPORTED 1
#endif
#if !defined(PNG_iTXt_SUPPORTED)
#  define PNG_iTXt_SUPPORTED 1
#endif

#define PNG_BYTES_TO_CHECK 4

static const char file_[] = __FILE__;

namespace cserve {

    static char lang_en[] = "en";
    static char exif_tag[] = "Raw profile type exif";
    static char iptc_tag[] = "Raw profile type iptc";
    static char xmp_tag[] = "XML:com.adobe.xmp";
    static char sipi_tag[] = "SIPI:io.sipi.essentials";

    //============== HELPER CLASS ==================
    class PngTextPtr {
    private:
        png_text *text_ptr;
        unsigned int num_text;
        unsigned int num_text_len;
    public:
        inline explicit PngTextPtr(unsigned int len = 16) : num_text_len(len) {
            num_text = 0;
            text_ptr = new png_text[num_text_len];
        };

        inline ~PngTextPtr() { delete[] text_ptr; };

        [[nodiscard]] inline unsigned int num() const { return num_text; };

        inline png_text *ptr() { return text_ptr; };

        png_text *next();

        void add_zTXt(char *key, char *data, unsigned int len);

        void add_iTXt(char *key, char *data, unsigned int len);

        void add_text(char *key, char *data, unsigned int len);
    };

    png_text *PngTextPtr::next() {
        if (num_text < num_text_len) {
            return &(text_ptr[num_text++]);
        } else {
            auto *tmpptr = new png_text[num_text_len + 16];
            for (unsigned int i = 0; i < num_text_len; i++) tmpptr[i] = text_ptr[i];
            delete[] text_ptr;
            text_ptr = tmpptr;
            return &(text_ptr[num_text++]);
        }
    }
    //=============================================

    void PngTextPtr::add_zTXt(char *key, char *data, unsigned int len) {
        png_text *tmp = this->next();
        memset(tmp, 0, sizeof(png_text));
        tmp->compression = PNG_TEXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = (char *) data;
        tmp->text_length = len;
        tmp->itxt_length = 0;
        tmp->lang = nullptr;
        tmp->lang_key = nullptr;
    }
    //=============================================

    void PngTextPtr::add_text(char *key, char *str, unsigned int len = 0) {
        png_text *tmp = this->next();
        memset(tmp, 0, sizeof(png_text));
        tmp->compression = PNG_TEXT_COMPRESSION_NONE;
        tmp->key = key;
        tmp->text = str;
        tmp->text_length = len;
        tmp->itxt_length = 0;
        tmp->lang = nullptr;
        tmp->lang_key = nullptr;
    }

    void PngTextPtr::add_iTXt(char *key, char *data, unsigned int len) {
        png_text *tmp = this->next();
        memset(tmp, 0, sizeof(png_text));
        tmp->compression = PNG_ITXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = data;
        tmp->text_length = 0;
        tmp->itxt_length = len;
        tmp->lang = nullptr;
        tmp->lang_key = nullptr;
    }
    //=============================================

    void iiif_error_fn(png_structp png_ptr, png_const_charp error_msg) {
        Server::logger()->error(error_msg);
        throw IIIFImageError(file_, __LINE__, error_msg);
    }

    void iiif_warning_fn(png_structp png_ptr, png_const_charp warning_msg) {
        Server::logger()->warn(warning_msg);
    }

    IIIFImage IIIFIOPng::read(const std::string &filepath,
                              int pagenum,
                              std::shared_ptr<IIIFRegion> region,
                              std::shared_ptr<IIIFSize> size,
                              bool force_bps_8,
                              ScalingQuality scaling_quality)
    {
        FILE *infile;
        unsigned char header[8];
        png_structp png_ptr;
        png_infop info_ptr;
        //
        // open the input file
        //
        if ((infile = fopen(filepath.c_str(), "rb")) == nullptr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open PNG file '{}'", filepath));
        }

        fread(header, 1, PNG_BYTES_TO_CHECK, infile);

        if (png_sig_cmp(header, 0, PNG_BYTES_TO_CHECK) != 0) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("'{}' is not a PNG file", filepath));
        }
        if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) nullptr, iiif_error_fn,
                                              (png_error_ptr) nullptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate memory fpr png_structp !", filepath));
        }
        if ((info_ptr = png_create_info_struct(png_ptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate memory fpr png_infop !", filepath));
        }

        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
        png_read_info(png_ptr, info_ptr);

        IIIFImage img{};
        png_uint_32 width, height;
        int color_type, bit_depth, interlace_type;
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
                     &interlace_type, NULL, NULL);

        img.nx = width;
        img.ny = height;
        img.orientation = TOPLEFT;

        png_set_packing(png_ptr);

        png_uint_32 res_x, res_y;
        float fres_x{0.0F}, fres_y{0.0F};
        int unit_type;
        if (png_get_pHYs(png_ptr, info_ptr, &res_x, &res_y, &unit_type)) {
            if (unit_type == PNG_RESOLUTION_METER) {
                fres_x = static_cast<float>(res_x) / 39.37007874015748F;
                fres_y = static_cast<float>(res_y) / 39.37007874015748F;
            }
            else {
                fres_x = static_cast<float>(res_x);
                fres_y = static_cast<float>(res_y);
            }
        }

        int colortype = png_get_color_type(png_ptr, info_ptr);
        switch (colortype) {
            case PNG_COLOR_TYPE_GRAY: { // implies nc = 1, (bit depths 1, 2, 4, 8, 16)
                png_set_expand_gray_1_2_4_to_8(png_ptr);
                img.photo = MINISBLACK;
                break;
            }
            case PNG_COLOR_TYPE_GRAY_ALPHA: { // implies nc = 2, (bit depths 8, 16)
                png_set_expand_gray_1_2_4_to_8(png_ptr);
                img.photo = MINISBLACK;
                img.es.push_back(ASSOCALPHA);
                break;
            }
            case PNG_COLOR_TYPE_PALETTE: { // we will not support it for now, (bit depths 1, 2, 4, 8)
                png_set_palette_to_rgb(png_ptr);
                img.photo = RGB;
                break;
            }
            case PNG_COLOR_TYPE_RGB: { // implies nc = 3 (standard case :-), (bit_depths 8, 16)
                img.photo = RGB;
                break;
            }
            case PNG_COLOR_TYPE_RGBA: { // implies nc = 4, (bit_depths 8, 16)
                img.photo = RGB;
                img.es.push_back(ASSOCALPHA);
                break;
            }
            default: {}
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0)
            png_set_tRNS_to_alpha(png_ptr);

        png_color_16 *image_background;
        if (png_get_bKGD(png_ptr, info_ptr, &image_background) != 0) {
            png_set_background(png_ptr, image_background,
                               PNG_BACKGROUND_GAMMA_FILE, 1, 1.0);
        }

        png_read_update_info(png_ptr, info_ptr);

        //
        // check for ICC profiles...
        //
        int srgb_intent;
        if (png_get_sRGB(png_ptr, info_ptr, &srgb_intent) != 0) {
            img.icc = std::make_shared<IIIFIcc>(icc_sRGB);
        } else {
            png_charp name;
            int compression_type = PNG_COMPRESSION_TYPE_BASE;
            png_bytep profile;
            png_uint_32 proflen;
            if (png_get_iCCP(png_ptr, info_ptr, &name, &compression_type, &profile, &proflen) != 0) {
                img.icc = std::make_shared<IIIFIcc>((unsigned char *) profile, (int) proflen);
            }
        }

        //
        // get exif data if available
        //
        png_byte *exifbuf;
        unsigned int exifbuf_len;
        if (png_get_eXIf_1(png_ptr, info_ptr, &exifbuf_len, &exifbuf) > 0) {
            img.exif = std::make_shared<IIIFExif>(exifbuf, exifbuf_len);
        }

        png_text *png_texts;
        int num_comments = png_get_text(png_ptr, info_ptr, &png_texts, nullptr);

        for (int i = 0; i < num_comments; i++) {
            size_t png_text_len;
            if ((png_texts[i].compression == PNG_TEXT_COMPRESSION_NONE) || (png_texts[i].compression == PNG_TEXT_COMPRESSION_zTXt)) {
                png_text_len = png_texts[i].text_length;
            }
            else if ((png_texts[i].compression == PNG_ITXT_COMPRESSION_NONE) || (png_texts[i].compression == PNG_ITXT_COMPRESSION_zTXt)) {
                png_text_len = png_texts[i].itxt_length;
            }
            if (strcmp(png_texts[i].key, xmp_tag) == 0) {
                try {
                    img.xmp = std::make_shared<IIIFXmp>((char *) png_texts[i].text, png_text_len);
                }
                catch(const IIIFError &err) {
                    Server::logger()->warn("Couldn't get XMP Info: {}", err.to_string());
                }
            }
            else if (strcmp(png_texts[i].key, iptc_tag) == 0) {
                try {
                    img.iptc = std::make_shared<IIIFIptc>(png_texts[i].text);
                }
                catch(const IIIFError &err) {
                    Server::logger()->warn("Couldn't get IPTC Info: {}", err.to_string());
                }
            }
            else if (strcmp(png_texts[i].key, sipi_tag) == 0) {
                IIIFEssentials se(png_texts[i].text);
                img.essential_metadata(se);
            }
            else if (strcmp(png_texts[i].key, exif_tag) == 0) {
                // do nothing!
            }
            else {
                Server::logger()->warn(fmt::format("PNG-COMMENT: key=\"{}\" text=\"{}\"\n", png_texts[i].key, png_texts[i].text));
            }
        }

        if (fres_x > 0.0F && fres_y > 0.0F) {
            if (img.exif == nullptr) {
                img.exif = std::make_shared<IIIFExif>();
            }
            img.exif->addKeyVal("Exif.Image.XResolution", IIIFExif::toRational(fres_x));
            img.exif->addKeyVal("Exif.Image.YResolution", IIIFExif::toRational(fres_y));
            img.exif->addKeyVal("Exif.Image.ResolutionUnit", 2); // DPI
        }

        //
        // prepare storage for reading image
        //
        size_t sll = png_get_rowbytes(png_ptr, info_ptr);
        auto buffer = std::make_unique<unsigned char[]>(height * sll);
        auto row_pointers = std::make_unique<png_bytep[]>(height);
        for (size_t i = 0; i < img.ny; i++) {
            row_pointers[i] = (buffer.get() + i * sll);
        }

        //
        // read image data
        //
        png_read_image(png_ptr, row_pointers.get());
        png_read_end(png_ptr, info_ptr);

        img.bps = png_get_bit_depth(png_ptr, info_ptr);
        img.nc = png_get_channels(png_ptr, info_ptr);

        if (color_type == PNG_COLOR_TYPE_PALETTE && img.nc == 4) {
            img.es.push_back(ASSOCALPHA);
        }

        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

        if (img.bps == 16) {
            std::unique_ptr<uint16_t[]> tmp((uint16_t *) buffer.release());
            for (int i = 0; i < img.nx * img.ny * img.nc; i++) {
                tmp[i] = ntohs(tmp[i]);
            }
            img.wpixels = std::move(tmp);
        }
        else {
            img.bpixels = std::move(buffer);
        }

        fclose(infile);

        if (region != nullptr) { //we just use the image.crop method
            img.crop(region);
        }

        //
        // resize/Scale the image if necessary
        //
        if (size != nullptr) {
            size_t nnx, nny;
            int reduce = -1;
            bool redonly;
            IIIFSize::SizeType rtype = size->get_size(img.nx, img.ny, nnx, nny, reduce, redonly);
            if (rtype != IIIFSize::FULL) {
                switch (scaling_quality.png) {
                    case HIGH:
                        img.scale(nnx, nny);
                        break;
                    case MEDIUM:
                        img.scaleMedium(nnx, nny);
                        break;
                    case LOW:
                        img.scaleFast(nnx, nny);
                        break;
                }
            }
        }

        if (force_bps_8) {
            if (!img.to8bps()) {
                throw IIIFImageError(file_, __LINE__, "Cannot convert to 8 Bits(sample");
            }
        }
        return img;
    };



    IIIFImgInfo IIIFIOPng::getDim(const std::string &filepath, int pagenum) {
        FILE *infile;
        unsigned char header[8];
        png_structp png_ptr;
        png_infop info_ptr;
        //
        // open the input file
        //
        if ((infile = fopen(filepath.c_str(), "rb")) == nullptr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open PNG file '{}'", filepath));
        }

        fread(header, 1, PNG_BYTES_TO_CHECK, infile);

        if (png_sig_cmp(header, 0, PNG_BYTES_TO_CHECK) != 0) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("'{}' is not a PNG file", filepath));
        }
        if ((png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, (png_voidp) nullptr, iiif_error_fn,
                                              (png_error_ptr) nullptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate memory fpr png_structp !", filepath));
        }
        if ((info_ptr = png_create_info_struct(png_ptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate memory fpr png_infop !", filepath));
        }

        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, PNG_BYTES_TO_CHECK);
        png_read_info(png_ptr, info_ptr);

        IIIFImgInfo info{};
        info.width = png_get_image_width(png_ptr, info_ptr);
        info.height = png_get_image_height(png_ptr, info_ptr);
        info.orientation = TOPLEFT;
        info.success = IIIFImgInfo::DIMS;

        IIIFImage img{};
        img.nx = png_get_image_width(png_ptr, info_ptr);
        img.ny = png_get_image_height(png_ptr, info_ptr);
        fclose(infile);

        png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);

        return info;
    }
    /*==========================================================================*/


    void create_text_chunk(PngTextPtr *png_textptr, char *key, char *str, unsigned int len) {
        png_text *chunk = png_textptr->next();
        chunk->compression = PNG_TEXT_COMPRESSION_NONE;
        chunk->key = key;
        chunk->text = str;
        chunk->text_length = len;
        chunk->itxt_length = 0;
        chunk->lang = lang_en;
        chunk->lang_key = nullptr;
    }

    /*==========================================================================*/

    static void conn_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
        auto *conn = (Connection *) png_get_io_ptr(png_ptr);
        try {
            conn->sendAndFlush(data, length);
        } catch (const InputFailure &err) {
            // TODO: do nothing ??
        }

    }

    /*==========================================================================*/

    static void conn_flush_data(png_structp png_ptr) {
        auto *conn = (Connection *) png_get_io_ptr(png_ptr);
        try {
            conn->flush();
        } catch (const InputFailure &err) {
            // TODO: do nothing ??
        }
    }

    /*==========================================================================*/

    void IIIFIOPng::write(IIIFImage &img,
                          const std::string &filepath,
                          const IIIFCompressionParams &params) {
        FILE *outfile = nullptr;
        png_structp png_ptr;

        if (!(png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, iiif_error_fn, iiif_warning_fn))) {
            throw IIIFImageError(file_, __LINE__,
                                 fmt::format("Error writing PNG file '{}': png_create_write_struct failed !", filepath));
        }

        if (filepath == "stdout:") {
            outfile = stdout;
        } else if (filepath == "HTTP") {
            png_set_write_fn(png_ptr, img.connection(), conn_write_data, conn_flush_data);
        } else {
            if (!(outfile = fopen(filepath.c_str(), "wb"))) {
                png_free_data(png_ptr, nullptr, PNG_FREE_ALL, -1);
                throw IIIFImageError(file_, __LINE__,
                                     fmt::format("Error writing PNG file '{}'}: Could notopen output file !", filepath));
            }
        }

        png_infop info_ptr;
        if (!(info_ptr = png_create_info_struct(png_ptr))) {
            png_free_data(png_ptr, nullptr, PNG_FREE_ALL, -1);
            throw IIIFImageError(file_, __LINE__,
                                 fmt::format("Error writing PNG file '{}': png_create_info_struct !", filepath));
        }

        if (outfile != nullptr) png_init_io(png_ptr, outfile);

        png_set_filter(png_ptr, 0, PNG_FILTER_NONE);

        /* set the zlib compression level */
        png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

        int color_type;
        if (img.nc == 1) { // grey value
            color_type = PNG_COLOR_TYPE_GRAY;
        } else if ((img.nc == 2) && (img.es.size() == 1)) { // grey value with alpha
            color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
        } else if (img.nc == 3) { // RGB
            color_type = PNG_COLOR_TYPE_RGB;
        } else if ((img.nc == 4) && (img.es.size() == 1)) { // RGB + ALPHA
            color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        }
        else if (img.nc == 4) {
            img.convertToIcc(IIIFIcc(PredefinedProfiles::icc_sRGB), 8);
            color_type = PNG_COLOR_TYPE_RGB;
            img.nc = 3;
            img.bps = 8;
        }
        else {
            png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
            throw IIIFImageError(file_, __LINE__,
                                 fmt::format("Error writing PNG file '{}': cannot handle number of channels () !", filepath));
        }

        png_set_IHDR(png_ptr, info_ptr, img.nx, img.ny, img.bps, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        //
        // ICC profile handfling is special...
        //
        IIIFEssentials es = img.essential_metadata();
        if ((img.icc != nullptr) || es.use_icc()) {
            if ((img.icc != nullptr) && (img.icc->getProfileType() == icc_LAB)) {
                img.convertToIcc(IIIFIcc(PredefinedProfiles::icc_sRGB), img.bps);
            }
            std::vector<unsigned char> icc_buf;
            try {
                if (es.use_icc()) {
                    icc_buf = es.icc_profile();
                }
                else {
                    icc_buf = img.icc->iccBytes();
                }
                png_set_iCCP(png_ptr, info_ptr, "ICC", PNG_COMPRESSION_TYPE_BASE, icc_buf.data(), icc_buf.size());
            } catch (IIIFError &err) {
                Server::logger()->error(err.to_string());
            }
        }

        std::vector<unsigned char> exif_buf;
        if (img.exif) {
            exif_buf = img.exif->exifBytes();
            png_set_eXIf_1(png_ptr, info_ptr, exif_buf.size(), exif_buf.data());
        }

        PngTextPtr chunk_ptr;

        //
        // other metadata comes here
        //
        std::unique_ptr<char[]> iptc_tmp;
        if (img.iptc) {
            unsigned int len;
            auto iptc_hex = img.iptc->iptcHexBytes(len);
            iptc_tmp = std::make_unique<char[]>(len + 1);
            memcpy (iptc_tmp.get(), iptc_hex.get(), len);
            iptc_tmp[len] = '\0';
            chunk_ptr.add_zTXt(iptc_tag, iptc_tmp.get(), len);
        }

        std::string xmp_buf;
        if (img.xmp != nullptr) {
            xmp_buf = img.xmp->xmpBytes();
            chunk_ptr.add_iTXt(xmp_tag, (char *) xmp_buf.data(), xmp_buf.size());
        }

        if (es.is_set()) {
            std::string esstr = std::string(es);
            unsigned int len = esstr.length();
            char iiif_buf[512 + 1];
            strncpy(iiif_buf, esstr.c_str(), 512);
            iiif_buf[512] = '\0';
            chunk_ptr.add_iTXt(sipi_tag, iiif_buf, len);
        }

        if (chunk_ptr.num() > 0) {
            png_set_text(png_ptr, info_ptr, chunk_ptr.ptr(), chunk_ptr.num());
        }
        png_write_info(png_ptr, info_ptr);

        auto *row_pointers = (png_bytep *) png_malloc(png_ptr, img.ny * sizeof(png_byte *));

        if (img.bps == 8) {
            png_bytep raw_tmp = img.bpixels.get();
            for (size_t i = 0; i < img.ny; i++) {
                row_pointers[i] = (raw_tmp + i * img.nx * img.nc);
            }
        } else if (img.bps == 16) {
            auto raw_tmp = (png_bytep) img.wpixels.get();
            for (size_t i = 0; i < img.ny; i++) {
                row_pointers[i] = (raw_tmp + 2 * i * img.nx * img.nc);
            }
        }

        png_set_rows(png_ptr, info_ptr, row_pointers);

        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN,
                      nullptr); // we expect the data to be little endian...
        png_write_end(png_ptr, info_ptr);

        png_free(png_ptr, row_pointers);

        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        if (outfile != nullptr) fclose(outfile);
    }

}
