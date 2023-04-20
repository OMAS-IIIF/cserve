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
#include <unistd.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>

#include <fcntl.h>
#include <cstdio>

#include "../IIIFError.h"
#include "../iiifparser/IIIFSize.h"
#include "IIIFIOJpeg.h"
#include "Connection.h"
#include "Cserve.h"

#include "jpeglib.h"
#include "jerror.h"
#include "spdlog/fmt/bundled/format.h"

#define ICC_MARKER  (JPEG_APP0 + 2)    /* JPEG marker code for ICC */
#define ESSENTIAL_METADATA_MARKER (JPEG_APP0 + 7)
//#define ICC_OVERHEAD_LEN  14        /* size of non-profile data in APP2 */
//#define MAX_BYTES_IN_MARKER  65533    /* maximum data len of a JPEG marker */


static const char file_[] = __FILE__;


namespace cserve {

    inline bool getbyte(int &c, FILE *f) {
        if ((c = getc(f)) == EOF) {
            return false;
        }
        else {
            return true;
        }
    }

    inline bool getword(int &c, FILE *f) {
        int cc_;
        int dd_;
        if (((cc_ = getc(f)) == EOF) || ((dd_ = getc(f)) == EOF)) {
            return false;
        }
        else {
            c = (cc_ << 8) + dd_;
            return true;
        }
    }

    static void unaligned_memcpy(void *to, const void *from, size_t len) {
        char *toptr = (char *) to;
        char *fromptr = (char *) from;
        while (toptr < (char *) to + len) {
            *toptr++ = *fromptr++;
        }
    }


    /*!
     * Special exception within the JPEG routines which can be caught separately
     */
    class JpegError : public std::runtime_error {
    public:
        inline JpegError() : std::runtime_error("!! JPEG_ERROR !!") {}

        inline explicit JpegError(const char *msg) : std::runtime_error(msg) {}

        [[nodiscard]]
        inline const char *what() const noexcept override {
            return std::runtime_error::what();
        }
    };
    //------------------------------------------------------------------


    typedef struct FileBuffer {
        JOCTET *buffer;
        size_t buflen;
        int file_id;
    } FileBuffer;

    /*!
     * Function which initializes the structures for managing the IO
     */
    static void init_file_destination(j_compress_ptr cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;
        cinfo->dest->free_in_buffer = file_buffer->buflen;
        cinfo->dest->next_output_byte = file_buffer->buffer;
    }
    //=============================================================================

    /*!
     * Function empty the libjeg buffer and write the data to the socket
     */
    static boolean empty_file_buffer(j_compress_ptr cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;
        size_t n = file_buffer->buflen;
        size_t nn = 0;
        do {
            ssize_t tmp_n = write(file_buffer->file_id, file_buffer->buffer + nn, n);
            if (tmp_n < 0) {
                throw JpegError("Couldn't write to file!");
            } else {
                n -= tmp_n;
                nn += tmp_n;
            }
        } while (n > 0);

        cinfo->dest->free_in_buffer = file_buffer->buflen;
        cinfo->dest->next_output_byte = file_buffer->buffer;

        return true;
    }
    //=============================================================================

    /*!
     * Finish writing data
     */
    static void term_file_destination(j_compress_ptr cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;
        size_t n = cinfo->dest->next_output_byte - file_buffer->buffer;
        size_t nn = 0;
        do {
            auto tmp_n = write(file_buffer->file_id, file_buffer->buffer + nn, n);
            if (tmp_n < 0) {
                throw IIIFImageError(file_, __LINE__, "Couldn't write to file!");
            } else {
                n -= tmp_n;
                nn += tmp_n;
            }
        } while (n > 0);

        delete[] file_buffer->buffer;
        delete file_buffer;
        cinfo->client_data = nullptr;

        delete cinfo->dest;
        cinfo->dest = nullptr;
    }
    //=============================================================================

    /*!
     * This function is used to setup the I/O destination to the HTTP socket
     */
    static void jpeg_file_dest(struct jpeg_compress_struct *cinfo, int file_id) {
        struct jpeg_destination_mgr *destmgr;
        FileBuffer *file_buffer;
        cinfo->client_data = new FileBuffer;
        file_buffer = (FileBuffer *) cinfo->client_data;

        file_buffer->buffer = new JOCTET[65536]; //(JOCTET *) malloc(buflen*sizeof(JOCTET));
        file_buffer->buflen = 65536;
        file_buffer->file_id = file_id;

        //destmgr = (struct jpeg_destination_mgr *) malloc(sizeof(struct jpeg_destination_mgr));
        destmgr = new struct jpeg_destination_mgr;

        destmgr->init_destination = init_file_destination;
        destmgr->empty_output_buffer = empty_file_buffer;
        destmgr->term_destination = term_file_destination;

        cinfo->dest = destmgr;
    }
    //=============================================================================


    static void init_file_source(struct jpeg_decompress_struct *cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;
        cinfo->src->next_input_byte = file_buffer->buffer;
        cinfo->src->bytes_in_buffer = 0;
    }
    //=============================================================================

    static int file_source_fill_input_buffer(struct jpeg_decompress_struct *cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;
        size_t nbytes = 0;
        do {
            auto n = read(file_buffer->file_id, file_buffer->buffer + nbytes, file_buffer->buflen - nbytes);
            if (n < 0) {
                break; // error
            }
            if (n == 0) break; // EOF reached...
            nbytes += n;
        } while (nbytes < file_buffer->buflen);
        if (nbytes <= 0) {
            ERREXIT(cinfo, 999);
            /*
            WARNMS(cinfo, JWRN_JPEG_EOF);
            infile_buffer->buffer[0] = (JOCTET) 0xFF;
            infile_buffer->buffer[1] = (JOCTET) JPEG_EOI;
            nbytes = 2;
            */
        }
        cinfo->src->next_input_byte = file_buffer->buffer;
        cinfo->src->bytes_in_buffer = nbytes;
        return TRUE;
    }
    //=============================================================================

    static void file_source_skip_input_data(struct jpeg_decompress_struct *cinfo, long num_bytes) {
        if (num_bytes > 0) {
            while (num_bytes > (long) cinfo->src->bytes_in_buffer) {
                num_bytes -= (long) cinfo->src->bytes_in_buffer;
                (void) file_source_fill_input_buffer(cinfo);
            }
        }
        cinfo->src->next_input_byte += (size_t) num_bytes;
        cinfo->src->bytes_in_buffer -= (size_t) num_bytes;
    }
    //=============================================================================

    static void term_file_source(struct jpeg_decompress_struct *cinfo) {
        auto *file_buffer = (FileBuffer *) cinfo->client_data;

        delete[] file_buffer->buffer;
        delete file_buffer;
        cinfo->client_data = nullptr;
        delete cinfo->src;
        cinfo->src = nullptr;
    }
    //=============================================================================

    static void jpeg_file_src(struct jpeg_decompress_struct *cinfo, int file_id) {
        struct jpeg_source_mgr *srcmgr;
        FileBuffer *file_buffer;
        cinfo->client_data = new FileBuffer;
        file_buffer = (FileBuffer *) cinfo->client_data;

        file_buffer->buffer = new JOCTET[65536];
        file_buffer->buflen = 65536;
        file_buffer->file_id = file_id;

        //srcmgr = (struct jpeg_source_mgr *) malloc(sizeof(struct jpeg_source_mgr));
        srcmgr = new struct jpeg_source_mgr;
        srcmgr->init_source = init_file_source;
        srcmgr->fill_input_buffer = file_source_fill_input_buffer;
        srcmgr->skip_input_data = file_source_skip_input_data;
        srcmgr->resync_to_restart = jpeg_resync_to_restart; // default!
        srcmgr->term_source = term_file_source;

        cinfo->src = srcmgr;
    }
    //=============================================================================


    /*!
     * Struct that is used to hold the variables for defining the
     * private I/O routines which are used to write the the HTTP socket
     */
    typedef struct HtmlBuffer {
        JOCTET *buffer; //!< Buffer for holding data to be written out
        size_t buflen;  //!< length of the buffer
        Connection *conobj; //!< Pointer to the connection objects
    } HtmlBuffer;

    /*!
     * Function which initializes the structures for managing the IO
     */
    static void init_html_destination(j_compress_ptr cinfo) {
        auto *html_buffer = (HtmlBuffer *) cinfo->client_data;
        cinfo->dest->free_in_buffer = html_buffer->buflen;
        cinfo->dest->next_output_byte = html_buffer->buffer;
    }
    //=============================================================================

    /*!
     * Function empty the libjeg buffer and write the data to the socket
     */
    static boolean empty_html_buffer(j_compress_ptr cinfo) {
        auto *html_buffer = (HtmlBuffer *) cinfo->client_data;
        try {
            html_buffer->conobj->sendAndFlush(html_buffer->buffer, static_cast<std::streamsize>(html_buffer->buflen));
        } catch (int i) { // an error occurred (possibly a broken pipe)
            throw JpegError("Couldn't write to HTTP socket");
        }
        cinfo->dest->free_in_buffer = html_buffer->buflen;
        cinfo->dest->next_output_byte = html_buffer->buffer;

        return true;
    }
    //=============================================================================

    /*!
     * Finish writing data
     */
    static void term_html_destination(j_compress_ptr cinfo) {
        std::cerr << "term_html_destination(j_compress_ptr cinfo)" << std::endl;
        auto *html_buffer = (HtmlBuffer *) cinfo->client_data;
        size_t nbytes = cinfo->dest->next_output_byte - html_buffer->buffer;
        try {
            html_buffer->conobj->sendAndFlush(html_buffer->buffer, static_cast<std::streamsize>(nbytes));
        } catch (int i) { // an error occured in sending the data (broken pipe?)
            throw JpegError("Couldn't write to HTTP socket");
        }
        delete[] html_buffer->buffer; //free(html_buffer->buffer);
        delete html_buffer; //free(html_buffer);
        cinfo->client_data = nullptr;

        delete cinfo->dest; //free(cinfo->dest);
        cinfo->dest = nullptr;
    }
    //=============================================================================


    static void cleanup_html_destination(j_compress_ptr cinfo) {
        auto *html_buffer = (HtmlBuffer *) cinfo->client_data;
        delete[] html_buffer->buffer;
        delete html_buffer;
        cinfo->client_data = nullptr;

        delete cinfo->dest;
        cinfo->dest = nullptr;
    }

    //=============================================================================

    /*!
     * This function is used to setup the I/O destination to the HTTP socket
     */
    static void
    jpeg_html_dest(struct jpeg_compress_struct *cinfo, Connection *conobj) {
        struct jpeg_destination_mgr *destmgr;
        HtmlBuffer *html_buffer;
        cinfo->client_data = new HtmlBuffer;// malloc(sizeof(HtmlBuffer));
        html_buffer = (HtmlBuffer *) cinfo->client_data;

        html_buffer->buffer = new JOCTET[65536]; //(JOCTET *) malloc(65536 * sizeof(JOCTET));
        html_buffer->buflen = 65536;
        html_buffer->conobj = conobj;

        //destmgr = (struct jpeg_destination_mgr *) malloc(sizeof(struct jpeg_destination_mgr));
        destmgr = new struct jpeg_destination_mgr;

        destmgr->init_destination = init_html_destination;
        destmgr->empty_output_buffer = empty_html_buffer;
        destmgr->term_destination = term_html_destination;

        cinfo->dest = destmgr;
    }
    //=============================================================================

    void IIIFIOJpeg::parse_photoshop(IIIFImage &img, char *data, int length) {
        int slen;
        unsigned int datalen = 0;
        char *ptr = data;
        unsigned short id;
        char sig[5];
        char name[256];
        int i;

        while ((ptr - data) < length) {
            sig[0] = *ptr;
            sig[1] = *(ptr + 1);
            sig[2] = *(ptr + 2);
            sig[3] = *(ptr + 3);
            sig[4] = '\0';
            if (strcmp(sig, "8BIM") != 0) break;
            ptr += 4;

            //
            // tag-ID processing
            id = ((unsigned char) *(ptr + 0) << 8) | (unsigned char) *(ptr + 1); // ID
            ptr += 2; // ID

            //
            // name processing (Pascal string)
            slen = (uint8_t) (*ptr);
            for (i = 0; (i < slen) && (i < 256); i++) name[i] = *(ptr + i + 1);
            name[i] = '\0';
            slen++; // add length byte
            if ((slen % 2) == 1) slen++;
            ptr += slen;

            //
            // data processing
            datalen = ((unsigned char) *ptr << 24) | ((unsigned char) *(ptr + 1) << 16) |
                      ((unsigned char) *(ptr + 2) << 8) | (unsigned char) *(ptr + 3);

            ptr += 4;

            switch (id) {
                case 0x0404: { // IPTC data
                    //cerr << ">>> Photoshop: IPTC" << endl;
                    if (img.iptc == nullptr) img.iptc = std::make_shared<IIIFIptc>((unsigned char *) ptr, datalen);
                    // IPTC – handled separately!
                    break;
                }
                case 0x040f: { // ICC data
                    //cerr << ">>> Photoshop: ICC" << endl;
                    // ICC profile
                    if (img.icc == nullptr) img.icc = std::make_shared<IIIFIcc>((unsigned char *) ptr, datalen);
                    break;
                }
                case 0x0422: { // EXIF data
                    if (img.exif == nullptr) img.exif = std::make_shared<IIIFExif>((unsigned char *) ptr, datalen);
                    uint16_t ori;
                    if (img.exif->getValByKey("Exif.Image.Orientation", ori)) {
                        img.orientation = Orientation(ori);
                    }
                    break;
                }
                case 0x0424: { // XMP data
                    //cerr << ">>> Photoshop: XMP" << endl;
                    // XMP data
                    if (img.xmp == nullptr) img.xmp = std::make_shared<IIIFXmp>(ptr, datalen);
                }
                default: {
                    // URL
                    char *str = (char *) calloc(1, (datalen + 1) * sizeof(char));
                    memcpy(str, ptr, datalen);
                    str[datalen] = '\0';
                    //fprintf(stderr, "XXX=%s\n", str);
                    break;
                }
            }

            if ((datalen % 2) == 1) datalen++;
            ptr += datalen;
        }
    }
    //=============================================================================


    /*!
    * This function is used to catch libjpeg errors which otherwise would
    * result in a call exit()
    */
    static void jpegErrorExit(j_common_ptr cinfo) {
        char jpegLastErrorMsg[JMSG_LENGTH_MAX];
        /* Create the message */
        (*(cinfo->err->format_message))(cinfo, jpegLastErrorMsg);
        /* Jump to the setjmp point */
        throw JpegError(jpegLastErrorMsg);
    }
    //=============================================================================


    IIIFImage IIIFIOJpeg::read(const std::string &filepath,
                               std::shared_ptr<IIIFRegion> region,
                               std::shared_ptr<IIIFSize> size,
                               bool force_bps_8,
                               ScalingQuality scaling_quality)
    {
        int infile;
        //
        // open the input file
        //
        if ((infile = ::open(filepath.c_str(), O_RDONLY)) == -1) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open JPEG file '{}'", filepath));
        }
        // workaround for bug #0011: jpeglib crashes the app when the file is not a jpeg file
        // we check the magic number before calling any jpeglib routines
        unsigned char magic[2];
        if (::read(infile, magic, 2) != 2) {
            ::close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Cannot read JPEG file '{}'", filepath));
        }
        if ((magic[0] != 0xff) || (magic[1] != 0xd8)) {
            ::close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("File '{}' is not a JPEG file", filepath));
        }
        // move infile position back to the beginning of the file
        ::lseek(infile, 0, SEEK_SET);

        struct jpeg_decompress_struct cinfo{};
        struct jpeg_error_mgr jerr{};

        JSAMPARRAY linbuf;
        jpeg_saved_marker_ptr marker;

        //
        // let's create the decompressor
        //
        jpeg_create_decompress (&cinfo);

        cinfo.dct_method = JDCT_FLOAT;

        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = jpegErrorExit;

        try {
            //jpeg_stdio_src(&cinfo, infile);
            jpeg_file_src(&cinfo, infile);
            jpeg_save_markers(&cinfo, JPEG_COM, 0xffff);
            for (int i = 0; i < 16; i++) {
                jpeg_save_markers(&cinfo, JPEG_APP0 + i, 0xffff);
            }
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}' Error: {}", filepath, jpgerr.what()));
        }

        //
        // now we read the header
        //
        int res;
        try {
            res = jpeg_read_header(&cinfo, TRUE);
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}' Error: {}", filepath, jpgerr.what()));
        }
        if (res != JPEG_HEADER_OK) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}'", filepath));
        }

        boolean no_cropping = false;
        if (region == nullptr) no_cropping = true;
        if ((region != nullptr) && (region->getType()) == IIIFRegion::FULL) no_cropping = true;

        uint32_t nnx, nny;
        IIIFSize::SizeType rtype = IIIFSize::FULL;
        if (size != nullptr) {
            rtype = size->get_type();
        }

        if (no_cropping) {
            //
            // here we prepare tha scaling/reduce stuff...
            //
            uint32_t reduce = 0;
            bool redonly = true; // we assume that only a reduce is necessary
            if ((size != nullptr) && (rtype != IIIFSize::FULL)) {
                size->get_size(cinfo.image_width, cinfo.image_height, nnx, nny, reduce, redonly);
            }
            else {
                reduce = 1;
            }

            cinfo.scale_num = 1;
            cinfo.scale_denom = reduce;
        }
        cinfo.do_fancy_upsampling = false;

        IIIFImage img{};
        img.bps = 8;
        //img.nx = cinfo.output_width;
        //img.ny = cinfo.output_height;
        //img.nc = cinfo.output_components;
        img.orientation = TOPLEFT; // may be changed in parse_photoshop or EXIF marker

        //
        // getting Metadata
        //
        marker = cinfo.marker_list;
        unsigned char *icc_buffer = nullptr;
        int icc_buffer_len = 0;
        while (marker) {
            if (marker->marker == JPEG_COM) {
                std::string emdatastr((char *) marker->data, marker->data_length);
                if (emdatastr.compare(0, 5, "SIPI:", 5) == 0) {
                    IIIFEssentials se(emdatastr);
                    img.essential_metadata(se);
                }
            } else if (marker->marker == JPEG_APP0 + 1) { // EXIF, XMP MARKER....
                //
                // first we try to find the exif part
                //
                auto *pos = (unsigned char *) memmem(marker->data, marker->data_length, "Exif\000\000", 6);
                if (pos != nullptr) {
                    img.exif = std::make_shared<IIIFExif>(pos + 6, marker->data_length - (pos - marker->data) - 6);
                    uint16_t ori;
                    if (img.exif->getValByKey("Exif.Image.Orientation", ori)) {
                        img.orientation = Orientation(ori);
                    }
                }

                //
                // first we try to find the xmp part: TODO: reading XMP which spans multiple segments. See ExtendedXMP !!!
                //
                pos = (unsigned char *) memmem(marker->data, marker->data_length, "http://ns.adobe.com/xap/1.0/\000",
                                               29);
                if (pos != nullptr) {
                    try {
                        char start[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'b', 'e', 'g', 'i', 'n',
                                        '\0'};
                        char end[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'e', 'n', 'd', '\0'};

                        char *s;
                        unsigned int ll = 0;
                        do {
                            s = start;
                            // skip to the start marker
                            while ((ll < marker->data_length) && (*pos != *s)) {
                                pos++; //// ISSUE: code fails here if there are many concurrent access; data overrrun??
                                ll++;
                            }
                            // read the start marker
                            while ((ll < marker->data_length) && (*s != '\0') && (*pos == *s)) {
                                pos++;
                                s++;
                                ll++;
                            }
                        } while ((ll < marker->data_length) && (*s != '\0'));
                        if (ll == marker->data_length) {
                            // we didn't find anything, go to next marker....
                           break;
                        }
                        // now we start reading the data
                        while ((ll < marker->data_length) && (*pos != '>')) {
                            ll++;
                            pos++;
                        }
                        pos++; // finally we have the start of XMP string
                        unsigned char *start_xmp = pos;

                        unsigned char *end_xmp;
                        do {
                            s = end;
                            while (*pos != *s) pos++;
                            end_xmp = pos; // a candidate
                            while ((*s != '\0') && (*pos == *s)) {
                                pos++;
                                s++;
                            }
                        } while (*s != '\0');
                        while (*pos != '>') {
                            pos++;
                        }
                        //pos++;

                        size_t xmp_len = end_xmp - start_xmp;

                        std::string xmpstr((char *) start_xmp, xmp_len);
                        size_t npos = xmpstr.find("</x:xmpmeta>");
                        xmpstr = xmpstr.substr(0, npos + 12);

                        img.xmp = std::make_shared<IIIFXmp>(xmpstr);
                    } catch (IIIFImageError &err) {
                        Server::logger()->warn(fmt::format("Failed to parse XMP in JPPEG file '{}'", filepath));
                    }
                }
            } else if (marker->marker == JPEG_APP0 + 2) { // ICC MARKER.... may span multiple marker segments
                //
                // first we try to find the exif part
                //
                auto *pos = (unsigned char *) memmem(marker->data, marker->data_length, "ICC_PROFILE\0", 12);
                if (pos != nullptr) {
                    auto len = marker->data_length - (pos - (unsigned char *) marker->data) - 14;
                    auto *tmpptr = (unsigned char *) realloc(icc_buffer, icc_buffer_len + len);
                    if (tmpptr == nullptr) { // cleanup
                        free (icc_buffer);
                        jpeg_destroy_decompress(&cinfo);
                        close(infile);
                        throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file '{}'. realloc failed!", filepath));
                    }
                    icc_buffer = tmpptr;
                    unaligned_memcpy(icc_buffer + icc_buffer_len, pos + 14, (size_t) len);
                    icc_buffer_len += static_cast<int>(len);
                }
            } else if (marker->marker == ESSENTIAL_METADATA_MARKER) {
                if (strncmp("SIPI", (char *) marker->data, 4) == 0) {
                    unsigned char *ptr = (unsigned char *) marker->data + 4;
                    size_t datalen = (*ptr << 24) | (*(ptr + 1) << 16) | (*(ptr + 2) << 8) | (*(ptr + 3));
                    std::string es_str{(char *) marker->data + 8, datalen};
                    IIIFEssentials se(es_str);
                    img.essential_metadata(se);
                }
            } else if (marker->marker == JPEG_APP0 + 13) { // PHOTOSHOP MARKER....
                if (strncmp("Photoshop 3.0", (char *) marker->data, 14) == 0) {
                    parse_photoshop(img, (char *) marker->data + 14, (int) marker->data_length - 14);
                }
            } else {
                //fprintf(stderr, "4) MARKER= %d, %d Bytes, ==> %s\n\n", marker->marker - JPEG_APP0, marker->data_length, marker->data);
            }
            marker = marker->next;
        }
        if (icc_buffer != nullptr) {
            img.icc = std::make_shared<IIIFIcc>(icc_buffer, icc_buffer_len);
        }

        try {
            jpeg_start_decompress(&cinfo);
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file '{}'. Error: {}", filepath, jpgerr.what()));
        }


        img.bps = 8;
        img.nx = cinfo.output_width;
        img.ny = cinfo.output_height;
        img.nc = cinfo.output_components;

        int colspace = cinfo.out_color_space; // JCS_UNKNOWN, JCS_GRAYSCALE, JCS_RGB, JCS_YCbCr, JCS_CMYK, JCS_YCCK
        switch (colspace) {
            case JCS_RGB: {
                img.photo = RGB;
                break;
            }
            case JCS_GRAYSCALE: {
                img.photo = MINISBLACK;
                break;
            }
            case JCS_CMYK: {
                img.photo = SEPARATED;
                break;
            }
            case JCS_YCbCr: {
                img.photo = YCBCR;
                break;
            }
            case JCS_YCCK: {
                throw IIIFImageError(file_, __LINE__, "Unsupported JPEG colorspace (JCS_YCCK)!");
            }
            case JCS_UNKNOWN: {
                throw IIIFImageError(file_, __LINE__, "Unsupported JPEG colorspace (JCS_UNKNOWN)!");
            }
            default: {
                throw IIIFImageError(file_, __LINE__, "Unsupported JPEG colorspace!");
            }
        }
        uint32_t sll = cinfo.output_components * cinfo.output_width * sizeof(uint8_t);

        img.bpixels = std::vector<uint8_t>(img.ny * sll);
        byte *rawbuf = img.bpixels.data();
        try {
            linbuf = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, sll, 1);
            for (size_t i = 0; i < img.ny; i++) {
                jpeg_read_scanlines(&cinfo, linbuf, 1);
                memcpy(&(rawbuf[i * sll]), linbuf[0], (size_t) sll);
            }
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}': Error: {}", filepath, jpgerr.what()));
        }
        try {
            jpeg_finish_decompress(&cinfo);
        } catch (JpegError &jpgerr) {
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}': Error: {}", filepath, jpgerr.what()));
        }

        try {
            jpeg_destroy_decompress(&cinfo);
        } catch (JpegError &jpgerr) {
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file: '{}': Error: {}", filepath, jpgerr.what()));
        }
        close(infile);

        //
        // do some cropping...
        //
        if (!no_cropping) { // not no cropping (!!) means "do crop"!
            //
            // let's first crop the region (we read the full size image in this case)
            //
            (void) img.crop(region);

            //
            // no we scale the region to the desired size
            //
            uint32_t reduce = 0;
            bool redonly;
            (void) size->get_size(img.nx, img.ny,
                                  nnx, nny, reduce, redonly);
        }

        //
        // resize/Scale the image if necessary
        //
        if ((size != nullptr) && (rtype != IIIFSize::FULL)) {
            switch (scaling_quality.jpeg) {
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

        return img;
    }
    //============================================================================


    IIIFImgInfo IIIFIOJpeg::getDim(const std::string &filepath) {
        // portions derived from IJG code */

        int infile;
        IIIFImgInfo info;

        //
        // open the input file
        //
        if ((infile = ::open(filepath.c_str(), O_RDONLY)) == -1) {
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }
        // workaround for bug #0011: jpeglib crashes the app when the file is not a jpeg file
        // we check the magic number before calling any jpeglib routines
        unsigned char magic[2];
        if (::read(infile, magic, 2) != 2) {
            ::close(infile);
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }
        if ((magic[0] != 0xff) || (magic[1] != 0xd8)) {
            ::close(infile);
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }

        // move infile position back to the beginning of the file
        ::lseek(infile, 0, SEEK_SET);

        struct jpeg_decompress_struct cinfo{};
        struct jpeg_error_mgr jerr{};

        jpeg_saved_marker_ptr marker;

        jpeg_create_decompress (&cinfo);

        cinfo.dct_method = JDCT_FLOAT;

        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = jpegErrorExit;

        try {
            jpeg_file_src(&cinfo, infile);
            jpeg_save_markers(&cinfo, JPEG_COM, 0xffff);
            for (int i = 0; i < 16; i++) {
                jpeg_save_markers(&cinfo, JPEG_APP0 + i, 0xffff);
            }
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }

        //
        // now we read the header
        //
        int res;
        try {
            res = jpeg_read_header(&cinfo, TRUE);
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }
        if (res != JPEG_HEADER_OK) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            info.success = IIIFImgInfo::FAILURE;
            return info;
        }

        IIIFImage img{};
        //
        // getting Metadata
        //
        marker = cinfo.marker_list;
        unsigned char *icc_buffer = nullptr;
        int icc_buffer_len = 0;
        while (marker) {
            if (marker->marker == JPEG_COM) {
                std::string emdatastr((char *) marker->data, marker->data_length);
                if (emdatastr.compare(0, 5, "SIPI:", 5) == 0) {
                    IIIFEssentials se(emdatastr);
                    img.essential_metadata(se);
                }
            } else if (marker->marker == JPEG_APP0 + 1) { // EXIF, XMP MARKER....
                //
                // first we try to find the exif part
                //
                auto *pos = (unsigned char *) memmem(marker->data, marker->data_length, "Exif\000\000", 6);
                if (pos != nullptr) {
                    img.exif = std::make_shared<IIIFExif>(pos + 6, marker->data_length - (pos - marker->data) - 6);
                }

                //
                // first we try to find the xmp part: TODO: reading XMP which spans multiple segments. See ExtendedXMP !!!
                //
                pos = (unsigned char *) memmem(marker->data, marker->data_length, "http://ns.adobe.com/xap/1.0/\000",
                                               29);
                if (pos != nullptr) {
                    try {
                        char start[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'b', 'e', 'g', 'i', 'n',
                                        '\0'};
                        char end[] = {'<', '?', 'x', 'p', 'a', 'c', 'k', 'e', 't', ' ', 'e', 'n', 'd', '\0'};

                        char *s;
                        unsigned int ll = 0;
                        do {
                            s = start;
                            // skip to the start marker
                            while ((ll < marker->data_length) && (*pos != *s)) {
                                pos++; //// ISSUE: code fails here if there are many concurrent access; data overrrun??
                                ll++;
                            }
                            // read the start marker
                            while ((ll < marker->data_length) && (*s != '\0') && (*pos == *s)) {
                                pos++;
                                s++;
                                ll++;
                            }
                        } while ((ll < marker->data_length) && (*s != '\0'));
                        if (ll == marker->data_length) {
                            // we didn't find anything....
                            throw IIIFImageError(file_, __LINE__, "XMP Problem");
                        }
                        // now we start reading the data
                        while ((ll < marker->data_length) && (*pos != '>')) {
                            ll++;
                            pos++;
                        }
                        pos++; // finally we have the start of XMP string
                        unsigned char *start_xmp = pos;

                        unsigned char *end_xmp;
                        do {
                            s = end;
                            while (*pos != *s) pos++;
                            end_xmp = pos; // a candidate
                            while ((*s != '\0') && (*pos == *s)) {
                                pos++;
                                s++;
                            }
                        } while (*s != '\0');
                        while (*pos != '>') {
                            pos++;
                        }
                        //pos++;

                        size_t xmp_len = end_xmp - start_xmp;

                        std::string xmpstr((char *) start_xmp, xmp_len);
                        size_t npos = xmpstr.find("</x:xmpmeta>");
                        xmpstr = xmpstr.substr(0, npos + 12);

                        img.xmp = std::make_shared<IIIFXmp>(xmpstr);
                    } catch (IIIFImageError &err) {
                        Server::logger()->warn(fmt::format("Failed to parse XMP in JPPEG file '{}'", filepath));
                    }
                }
            } else if (marker->marker == JPEG_APP0 + 13) { // PHOTOSHOP MARKER....
                if (strncmp("Photoshop 3.0", (char *) marker->data, 14) == 0) {
                    parse_photoshop(img, (char *) marker->data + 14, (int) marker->data_length - 14);
                }
            }
            marker = marker->next;
        }

        try {
            jpeg_start_decompress(&cinfo);
        } catch (JpegError &jpgerr) {
            jpeg_destroy_decompress(&cinfo);
            close(infile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error reading JPEG file '{}'. Error: {}", filepath, jpgerr.what()));
        }

        info.width = cinfo.output_width;
        info.height = cinfo.output_height;
        info.orientation = TOPLEFT;
        if (img.exif != nullptr) {
            uint16_t ori;
            if (img.exif->getValByKey("Exif.Image.Orientation", ori)) {
                info.orientation = Orientation(ori);
            }
        }
        uint32_t reduce = 2;
        uint32_t tmp_nnx = IIIFSize::epsilon_ceil_division(static_cast<float>(info.width), static_cast<float>(reduce));
        uint32_t tmp_nny = IIIFSize::epsilon_ceil_division(static_cast<float>(info.height), static_cast<float>(reduce));
        while ((tmp_nnx > 128) && (tmp_nny > 128)) {
            tmp_nnx = IIIFSize::epsilon_ceil_division(static_cast<float>(info.width), static_cast<float>(reduce));
            tmp_nny = IIIFSize::epsilon_ceil_division(static_cast<float>(info.height), static_cast<float>(reduce));
            SubImageInfo sub{reduce, tmp_nnx, tmp_nny, 0, 0};
            info.resolutions.push_back(sub);
            reduce *= 2;
        }
        info.success = IIIFImgInfo::DIMS;
        jpeg_destroy_decompress(&cinfo);
        close(infile);
        return info;
    }
    //============================================================================


    void IIIFIOJpeg::write(IIIFImage &img, const std::string &filepath, const IIIFCompressionParams &params) {
        int quality = 80;
        if (!params.empty()) {
            try {
                quality = stoi(params.at(JPEG_QUALITY));
            }
            catch(const std::out_of_range &er) {
                throw IIIFImageError(file_, __LINE__, "JPEG quality argument must be integer between 0 and 100");
            }
            catch (const std::invalid_argument& ia) {
                throw IIIFImageError(file_, __LINE__, "JPEG quality argument must be integer between 0 and 100");
            }
            if ((quality < 0) || (quality > 100)){
                throw IIIFImageError(file_, __LINE__, "JPEG quality argument must be integer between 0 and 100");
            }
        }

        if (img.bps == 16) img.to8bps();

        //
        // we have to check if the image has an alpha channel (not supported by JPEG). If
        // so, we remove it!
        //
        if ((img.nc > 3) && (img.getNalpha() > 0)) { // we have an alpha channel....
            for (size_t i = 3; i < (img.getNalpha() + 3); i++) img.removeChan(i);
        }

        struct jpeg_compress_struct cinfo{};
        struct jpeg_error_mgr jerr{};

        cinfo.err = jpeg_std_error(&jerr);
        jerr.error_exit = jpegErrorExit;

        int outfile = -1;        /* target file */
        JSAMPROW row_pointer[1];    /* pointer to JSAMPLE row[s] */
        uint32_t row_stride;        /* physical row width in image buffer */

        try {
            jpeg_create_compress(&cinfo);
        } catch (JpegError &jpgerr) {
            throw IIIFImageError(file_, __LINE__, fmt::format("JPEG writing of file '{}' failed: {}", filepath, jpgerr.what()));
        }

        if (filepath == "HTTP") { // we are transmitting the data through the webserver
            Connection *conobj = img.connection();
            try {
                jpeg_html_dest(&cinfo, conobj);
            } catch (JpegError &jpgerr) {
                cleanup_html_destination(&cinfo);
                jpeg_destroy_compress(&cinfo);
                throw IIIFImageError(file_, __LINE__, fmt::format("JPEG writing of file '{}' failed: {}", filepath, jpgerr.what()));
            }
        } else {
            if (filepath == "stdout:") {
                jpeg_stdio_dest(&cinfo, stdout);
            } else {
                if ((outfile = open(filepath.c_str(), O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) ==
                    -1) {
                    jpeg_destroy_compress(&cinfo);
                    throw IIIFImageError(file_, __LINE__, fmt::format("Cannot open JPEG file '{}'", filepath));
                }
                jpeg_file_dest(&cinfo, outfile);
            }
        }

        cinfo.image_width = (int) img.nx;    /* image width and height, in pixels */
        cinfo.image_height = (int) img.ny;
        cinfo.input_components = (int) img.nc;        /* # of color components per pixel */
        switch (img.photo) {
            case MINISWHITE:
            case MINISBLACK: {
                if (img.nc != 1) {
                    jpeg_destroy_compress(&cinfo);
                    throw IIIFImageError(file_, __LINE__,fmt::format("Num of components not 1 (nc = {})!", img.nc));
                }
                cinfo.in_color_space = JCS_GRAYSCALE;
                cinfo.jpeg_color_space = JCS_GRAYSCALE;
                break;
            }
            case RGB: {
                if (img.nc != 3) {
                    jpeg_destroy_compress(&cinfo);
                    throw IIIFImageError(file_, __LINE__,fmt::format("Num of components not 3 (nc = {})!", img.nc));
                }
                cinfo.in_color_space = JCS_RGB;
                cinfo.jpeg_color_space = JCS_RGB;
                break;
            }
            case SEPARATED: {
                if (img.nc != 4) {
                    jpeg_destroy_compress(&cinfo);
                    throw IIIFImageError(file_, __LINE__,fmt::format("Num of components not 4 (nc = {})!", img.nc));
                }
                cinfo.in_color_space = JCS_CMYK;
                cinfo.jpeg_color_space = JCS_CMYK;
                break;
            }
            case YCBCR: {
                if (img.nc != 3) {
                    jpeg_destroy_compress(&cinfo);
                    throw IIIFImageError(file_, __LINE__,fmt::format("Num of components not 3 (nc = {})!", img.nc));
                }
                cinfo.in_color_space = JCS_YCbCr;
                cinfo.jpeg_color_space = JCS_YCbCr;
                break;
            }
            case CIELAB: {
                img.convertToIcc(IIIFIcc(PredefinedProfiles::icc_sRGB), 8);
                cinfo.in_color_space = JCS_RGB;
                cinfo.jpeg_color_space = JCS_RGB;
                break;
            }
            default: {
                jpeg_destroy_compress(&cinfo);
                throw IIIFImageError(file_, __LINE__, fmt::format("Unsupported JPEG colorspace: {}", img.photo));
            }
        }
        cinfo.progressive_mode = TRUE;
        cinfo.write_Adobe_marker = TRUE;
        cinfo.write_JFIF_header = TRUE;
        try {
            jpeg_set_defaults(&cinfo);
            jpeg_set_quality(&cinfo, quality, TRUE /* TRUE, then limit to baseline-JPEG values */);

            jpeg_simple_progression(&cinfo);
            jpeg_start_compress(&cinfo, TRUE);
        } catch (JpegError &jpgerr) {
            jpeg_finish_compress(&cinfo);
            jpeg_destroy_compress(&cinfo);
            if (outfile != -1) close(outfile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error compressing JPEG: ", jpgerr.what()));
        }

        //
        // Here we write the marker
        //
        //
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // ATTENTION: The markers must be written in the right sequence: APP0, APP1, APP2, ..., APP15
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //

        if (img.exif != nullptr) {
            std::vector<unsigned char> buf = img.exif->exifBytes();
            if (buf.size() <= 65535) {
                char start[] = "Exif\000\000";
                size_t start_l = sizeof(start) - 1;  // remove trailing '\0';
                auto exifchunk = std::make_unique<unsigned char[]>(buf.size() + start_l);
                unaligned_memcpy(exifchunk.get(), start, (size_t) start_l);
                unaligned_memcpy(exifchunk.get() + start_l, buf.data(), (size_t) buf.size());

                try {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (JOCTET *) exifchunk.get(), start_l + buf.size());
                } catch (JpegError &jpgerr) {
                    jpeg_finish_compress(&cinfo);
                    jpeg_destroy_compress(&cinfo);
                    if (outfile != -1) close(outfile);
                    throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG EXIF markers: ", jpgerr.what()));
                }
            } else {
                Server::logger()->warn("EXIF data to big for JPEG!");
            }
        }

        if (img.xmp != nullptr) {
            std::string buf = img.xmp->xmpBytes();

            if ((!buf.empty()) && (buf.size() <= 65535)) {
                char start[] = "http://ns.adobe.com/xap/1.0/\000";
                size_t start_l = sizeof(start) - 1; // remove trailing '\0';
                auto xmpchunk = std::make_unique<char[]>(buf.size() + start_l);
                unaligned_memcpy(xmpchunk.get(), start, (size_t) start_l);
                unaligned_memcpy(xmpchunk.get() + start_l, buf.data(), (size_t) buf.size());
                try {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 1, (JOCTET *) xmpchunk.get(), start_l + buf.size());
                } catch (JpegError &jpgerr) {
                    jpeg_finish_compress(&cinfo);
                    jpeg_destroy_compress(&cinfo);
                    if (outfile != -1) close(outfile);
                    throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG XMP markers: ", jpgerr.what()));
                }
            } else {
                // std::cerr << "xml to big" << std::endl;
            }
        }

        IIIFEssentials es = img.essential_metadata();

        if ((img.icc != nullptr) || es.use_icc()) {
            std::vector<unsigned char> buf;
            try {
                if (es.use_icc()) {
                    buf = es.icc_profile();
                } else {
                    buf = img.icc->iccBytes();
                }
            } catch (IIIFError &err) {
                Server::logger()->warn(err.what());
            }
            unsigned char start[14] = {0x49, 0x43, 0x43, 0x5F, 0x50, 0x52, 0x4F, 0x46, 0x49, 0x4C, 0x45,
                                       0x0}; //"ICC_PROFILE\000";
            size_t start_l = 14;
            unsigned int n = buf.size() / (65533 - start_l + 1) + 1;

            auto iccchunk = std::make_unique<unsigned char[]>(65533);

            unsigned int n_towrite = buf.size();
            unsigned int n_nextwrite = 65533 - start_l;
            unsigned int n_written = 0;
            for (unsigned int i = 0; i < n; i++) {
                start[12] = (unsigned char) (i + 1);
                start[13] = (unsigned char) n;
                if (n_nextwrite > n_towrite) n_nextwrite = n_towrite;
                unaligned_memcpy(iccchunk.get(), start, (size_t) start_l);
                unaligned_memcpy(iccchunk.get() + start_l, buf.data() + n_written, (size_t) n_nextwrite);
                try {
                    jpeg_write_marker(&cinfo, ICC_MARKER, iccchunk.get(), n_nextwrite + start_l);
                } catch (JpegError &jpgerr) {
                    jpeg_finish_compress(&cinfo);
                    jpeg_destroy_compress(&cinfo);
                   if (outfile != -1) close(outfile);
                    throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG markers: ", jpgerr.what()));
                }

                n_towrite -= n_nextwrite;
                n_written += n_nextwrite;
            }
            if (n_towrite != 0) {
                IIIFImageError err(file_, __LINE__, "Something weird happened while writing JPEG ICC markers!");
                Server::logger()->warn(err.what());
            }
        }

        if (img.essential_metadata().is_set()) {
            char start[]{'S', 'I', 'P', 'I'};// = "SIPI\000";
            size_t start_l = sizeof(start);
            std::string es_str = std::string(img.essential_metadata());
            unsigned char siz[4];
            siz[0] = (unsigned char) ((es_str.size() >> 24) & 0x000000ff);
            siz[1] = (unsigned char) ((es_str.size() >> 16) & 0x000000ff);
            siz[2] = (unsigned char) ((es_str.size() >> 8) & 0x000000ff);
            siz[3] = (unsigned char) (es_str.size() & 0x000000ff);
            auto eschunk = std::make_unique<char[]>(start_l + 4 + es_str.size());
            unaligned_memcpy(eschunk.get(), start, (size_t) start_l);
            unaligned_memcpy(eschunk.get() + start_l, siz, (size_t) 4);
            unaligned_memcpy(eschunk.get() + start_l + 4, es_str.data(), es_str.size());
            try {
                jpeg_write_marker(&cinfo, ESSENTIAL_METADATA_MARKER, (JOCTET *) eschunk.get(), start_l + 4 + es_str.size());
            } catch (JpegError &jpgerr) {
                jpeg_destroy_compress(&cinfo);
                if (outfile != -1) close(outfile);
                throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG SIPI marker: ", jpgerr.what()));
            }
        }

        if (img.iptc != nullptr) {
            std::vector<unsigned char> buf = img.iptc->iptcBytes();
            if (buf.size() <= 65535) {
                char start[] = " Photoshop 3.0\0008BIM\004\004\000\000";
                size_t start_l = sizeof(start) - 1;
                unsigned char siz[4];
                siz[0] = (unsigned char) ((buf.size() >> 24) & 0x000000ff);
                siz[1] = (unsigned char) ((buf.size() >> 16) & 0x000000ff);
                siz[2] = (unsigned char) ((buf.size() >> 8) & 0x000000ff);
                siz[3] = (unsigned char) (buf.size() & 0x000000ff);

                auto iptcchunk = std::make_unique<char[]>(start_l + 4 + buf.size());
                unaligned_memcpy(iptcchunk.get(), start, (size_t) start_l);
                unaligned_memcpy(iptcchunk.get() + start_l, siz, (size_t) 4);
                unaligned_memcpy(iptcchunk.get() + start_l + 4, buf.data(), buf.size());

                try {
                    jpeg_write_marker(&cinfo, JPEG_APP0 + 13, (JOCTET *) iptcchunk.get(), start_l + 4 + buf.size());
                } catch (JpegError &jpgerr) {
                    jpeg_destroy_compress(&cinfo);
                    if (outfile != -1) close(outfile);
                    throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG IPTC markers: ", jpgerr.what()));
                }
            }
            else {
            // std::cerr << "iptc to big" << std::endl;
            }
        }

        if (es.is_set()) {
            try {
                std::string esstr = std::string(es);
                unsigned int len = esstr.length();
                char sipi_buf[512 + 1];
                strncpy(sipi_buf, esstr.c_str(), 512);
                sipi_buf[512] = '\0';
                jpeg_write_marker(&cinfo, JPEG_COM, (JOCTET *) sipi_buf, len);
            } catch (JpegError &jpgerr) {
                jpeg_destroy_compress(&cinfo);
                if (outfile != -1) close(outfile);
                throw IIIFImageError(file_, __LINE__, fmt::format("Error writing JPEG IIIF essential markers: ", jpgerr.what()));
            }
        }

        row_stride = img.nx * img.nc;    /* JSAMPLEs per row in image_buffer */

        byte *rawbuf = img.bpixels.data();
        try {
            while (cinfo.next_scanline < cinfo.image_height) {
                // jpeg_write_scanlines expects an array of pointers to scanlines.
                // Here the array is only one element long, but you could pass
                // more than one scanline at a time if that's more convenient.
                row_pointer[0] = &rawbuf[cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
            }
        } catch (JpegError &jpgerr) {
            jpeg_destroy_compress(&cinfo);
            if (outfile != -1) close(outfile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error compressing JPEG: ", jpgerr.what()));
        }

        try {
            jpeg_finish_compress(&cinfo);
        } catch (JpegError &jpgerr) {
            jpeg_destroy_compress(&cinfo);
            if (outfile != -1) close(outfile);
            throw IIIFImageError(file_, __LINE__, fmt::format("Error compressing JPEG: ", jpgerr.what()));
        }
        if (outfile != -1) close(outfile);

        jpeg_destroy_compress(&cinfo);
    }

} // namespace
