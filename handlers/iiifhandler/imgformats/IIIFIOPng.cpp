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
        tmp->compression = PNG_TEXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = (char *) data;
        tmp->text_length = len;
        tmp->itxt_length = 0;
        tmp->lang = (char *) "";
        tmp->lang_key = (char *) "";
    }
    //=============================================

    void PngTextPtr::add_iTXt(char *key, char *data, unsigned int len) {
        png_text *tmp = this->next();
        tmp->compression = PNG_ITXT_COMPRESSION_zTXt;
        tmp->key = key;
        tmp->text = data;
        tmp->text_length = 0;
        tmp->itxt_length = len;
        tmp->lang = (char *) "";
        tmp->lang_key = (char *) "";
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
        png_infop end_info;
        //
        // open the input file
        //
        if ((infile = fopen(filepath.c_str(), "rb")) == nullptr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open PNG file '{}'", filepath));
        }

        fread(header, 1, 8, infile);

        if (png_sig_cmp(header, 0, 8) != 0) {
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
        if ((end_info = png_create_info_struct(png_ptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate mempry fpr png_infop !", filepath));
        }
        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);

        IIIFImage img{};
        img.nx = png_get_image_width(png_ptr, info_ptr);
        img.ny = png_get_image_height(png_ptr, info_ptr);
        img.bps = png_get_bit_depth(png_ptr, info_ptr);
        img.nc = png_get_channels(png_ptr, info_ptr);

        png_uint_32 res_x, res_y;
        int unit_type;
        if (png_get_pHYs(png_ptr, info_ptr, &res_x, &res_y, &unit_type)) {
            img.exif = std::make_shared<IIIFExif>();
            float fres_x, fres_y;
            if (unit_type == PNG_RESOLUTION_METER) {
                fres_x = res_x / 39.37007874015748;
                fres_y = res_y / 39.37007874015748;
            }
            else {
                fres_x = res_x;
                fres_y = res_y;
            }
            img.exif->addKeyVal("Exif.Image.XResolution", fres_x);
            img.exif->addKeyVal("Exif.Image.YResolution", fres_x);
            img.exif->addKeyVal("Exif.Image.ResolutionUnit", 2); // DPI
        }

        int colortype = png_get_color_type(png_ptr, info_ptr);
        switch (colortype) {
            case PNG_COLOR_TYPE_GRAY: { // implies nc = 1, (bit depths 1, 2, 4, 8, 16)
                img.photo = MINISBLACK;
                break;
            }
            case PNG_COLOR_TYPE_GRAY_ALPHA: { // implies nc = 2, (bit depths 8, 16)
                img.photo = MINISBLACK;
                img.es.push_back(ASSOCALPHA);
                break;
            }
            case PNG_COLOR_TYPE_PALETTE: { // we will not support it for now, (bit depths 1, 2, 4, 8)
                img.photo = PALETTE;
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
        png_text *png_texts;
        int num_comments = png_get_text(png_ptr, info_ptr, &png_texts, nullptr);

        for (int i = 0; i < num_comments; i++) {
            if (strcmp(png_texts[i].key, xmp_tag) == 0) {
                img.xmp = std::make_shared<IIIFXmp>((char *) png_texts[i].text, (int) png_texts[i].text_length);
            } else if (strcmp(png_texts[i].key, exif_tag) == 0) {
                try {
                    img.exif = std::make_shared<IIIFExif>((unsigned char *) png_texts[i].text,
                                                           (unsigned int) png_texts[i].text_length);
                }
                catch (IIIFError &err) {
                    Server::logger()->warn("Creating EXIF datablock failed.");
                }
            } else if (strcmp(png_texts[i].key, iptc_tag) == 0) {
                img.iptc = std::make_shared<IIIFIptc>((unsigned char *) png_texts[i].text,
                                                       (unsigned int) png_texts[i].text_length);
            } else if (strcmp(png_texts[i].key, sipi_tag) == 0) {
                IIIFEssentials se(png_texts[i].text);
                img.essential_metadata(se);
            } else {
                Server::logger()->warn(fmt::format("PNG-COMMENT: key=\"{}\" text=\"{}\"\n", png_texts[i].key, png_texts[i].text));
            }
        }

        png_size_t sll = png_get_rowbytes(png_ptr, info_ptr);

        if (colortype == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
            img.nc = 3;
            img.photo = RGB;
            sll = 3 * sll;
        }

        if (colortype == PNG_COLOR_TYPE_GRAY && img.bps < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
            img.bps = 8;
        }

        auto buffer = std::make_unique<uint8_t[]>(img.ny * sll);
        auto raw_buffer = buffer.get();
        auto row_pointers = std::make_unique<png_bytep[]>(img.ny);

        for (size_t i = 0; i < img.ny; i++) {
            row_pointers[i] = (raw_buffer + i * sll);
        }

        png_read_image(png_ptr, row_pointers.get());
        png_read_end(png_ptr, end_info);
        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

        if (colortype == PNG_COLOR_TYPE_PALETTE) {
            img.nc = 3;
            img.photo = RGB;
        }

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
        png_infop end_info;
        //
        // open the input file
        //
        if ((infile = fopen(filepath.c_str(), "rb")) == nullptr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open PNG file '{}'", filepath));
        }

        fread(header, 1, 8, infile);

        if (png_sig_cmp(header, 0, 8) != 0) {
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
        if ((end_info = png_create_info_struct(png_ptr)) == nullptr) {
            fclose(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading PNG file '{}': Could not allocate mempry fpr png_infop !", filepath));
        }
        png_init_io(png_ptr, infile);
        png_set_sig_bytes(png_ptr, 8);
        png_read_info(png_ptr, info_ptr);

        IIIFImgInfo info{};
        info.width = png_get_image_width(png_ptr, info_ptr);
        info.height = png_get_image_height(png_ptr, info_ptr);
        info.success = IIIFImgInfo::DIMS;

        IIIFImage img{};
        img.nx = png_get_image_width(png_ptr, info_ptr);
        img.ny = png_get_image_height(png_ptr, info_ptr);
        fclose(infile);

        png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);

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

        PngTextPtr chunk_ptr(4);

        //
        // other metadata comes here
        //

        std::vector<unsigned char> exif_buf;
        if (img.exif) {
            exif_buf = img.exif->exifBytes();
            chunk_ptr.add_zTXt(exif_tag, (char *) exif_buf.data(), exif_buf.size());
        }

        std::vector<unsigned char> iptc_buf;
        if (img.iptc) {
            iptc_buf = img.iptc->iptcBytes();
            chunk_ptr.add_zTXt(iptc_tag, (char *) iptc_buf.data(), iptc_buf.size());
        }

        std::string xmp_buf;
        if (img.xmp != nullptr) {
            xmp_buf = img.xmp->xmpBytes();
            chunk_ptr.add_iTXt(xmp_tag, (char *) xmp_buf.data(), xmp_buf.size());
        }

        if (es.is_set()) {
            std::string esstr = es;
            unsigned int len = esstr.length();
            char iiif_buf[512 + 1];
            strncpy(iiif_buf, esstr.c_str(), 512);
            iiif_buf[512] = '\0';
            chunk_ptr.add_iTXt(sipi_tag, iiif_buf, len);
        }

        if (chunk_ptr.num() > 0) {
            png_set_text(png_ptr, info_ptr, chunk_ptr.ptr(), chunk_ptr.num());
        }

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

        png_write_info(png_ptr, info_ptr);
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_SWAP_ENDIAN,
                      nullptr); // we expect the data to be little endian...
        png_write_end(png_ptr, info_ptr);

        png_free(png_ptr, row_pointers);

        png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
        png_destroy_write_struct(&png_ptr, &info_ptr);

        if (outfile != nullptr) fclose(outfile);
    }

}
