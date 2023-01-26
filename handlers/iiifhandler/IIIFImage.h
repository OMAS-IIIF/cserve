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
#ifndef __sipi_image_h
#define __sipi_image_h

#include <sstream>
#include <utility>
#include <string>
#include <unordered_map>
#include <exception>

#include "IIIFError.h"
//#include "IIIFImgTools.h"
#include "metadata/IIIFXmp.h"
#include "metadata/IIIFIcc.h"
#include "metadata/IIIFIptc.h"
#include "metadata/IIIFExif.h"
#include "metadata/IIIFIcc.h"
#include "metadata/IIIFEssentials.h"
#include "iiifparser/IIIFRegion.h"
#include "iiifparser/IIIFSize.h"

#include "Connection.h"
#include "Hash.h"
#include "IIIFPhotometricInterpretation.h"


namespace cserve {

    typedef unsigned char byte;
    typedef unsigned short word;

    /*! The meaning of extra channels as used in the TIF format */
    typedef enum : unsigned short {
        UNSPECIFIED = 0,    //!< Unknown meaning
        ASSOCALPHA = 1,     //!< Associated alpha channel
        UNASSALPHA = 2      //!< Unassociated alpha channel
    } ExtraSamples;

    typedef enum {
        SKIP_NONE = 0x00, SKIP_ICC = 0x01, SKIP_XMP = 0x02, SKIP_IPTC = 0x04, SKIP_EXIF = 0x08, SKIP_ALL = 0xFF
    } SkipMetadata;

    typedef enum {
        HIGH = 0, MEDIUM = 1, LOW = 2
    } ScalingMethod;

    typedef struct _ScalingQuality {
        ScalingMethod jk2;
        ScalingMethod jpeg;
        ScalingMethod tiff;
        ScalingMethod png;
    } ScalingQuality;

    typedef enum : unsigned short { // from the TIFF specification...
        TOPLEFT = 1,  //!< The 0th row represents the visual top of the image, and the 0th column represents the visual left-hand side.
        TOPRIGHT = 2, //!< The 0th row represents the visual top of the image, and the 0th column represents the visual right-hand side.
        BOTRIGHT = 3, //!< The 0th row represents the visual bottom of the image, and the 0th column represents the visual right-hand side.
        BOTLEFT = 4,  //!< The 0th row represents the visual bottom of the image, and the 0th column represents the visual left-hand side.
        LEFTTOP = 5,  //!< The 0th row represents the visual left-hand side of the image, and the 0th column represents the visual top.
        RIGHTTOP = 6, //!< The 0th row represents the visual right-hand side of the image, and the 0th column represents the visual top.
        RIGHTBOT = 7, //!< The 0th row represents the visual right-hand side of the image, and the 0th column represents the visual bottom.
        LEFTBOT = 8   //!< The 0th row represents the visual left-hand side of the image, and the 0th column represents the visual bottom.
    } Orientation;

    class IIIFImgInfo {
    public:
        enum { FAILURE = 0, DIMS = 1, ALL = 2 } success;
        uint32_t width;
        uint32_t height;
        Orientation orientation;
        int tile_width;
        int tile_height;
        int clevels;
        int numpages;
        std::string internalmimetype;
        std::string origname;
        std::string origmimetype;

        inline IIIFImgInfo() : success(FAILURE), width(0), height(0), orientation(TOPLEFT), tile_width(0),
                               tile_height(0), clevels(0), numpages(0) {};

    };

    enum {
        JPEG_QUALITY,
        J2K_Sprofile,
        J2K_Creversible,
        J2K_Clayers,
        J2K_Clevels,
        J2K_Corder,
        J2K_Cprecincts,
        J2K_Cblk,
        J2K_Cuse_sop,
        J2K_Stiles,
        J2K_rates,
        TIFF_COMPRESSION
    } IIIFCompressionParamName;

    typedef std::unordered_map<int, std::string> IIIFCompressionParams;

    enum InfoError { INFO_ERROR };

    inline std::string orientation_str(Orientation orientation) {
        const std::map<Orientation,std::string> enum_strings = {
                { TOPLEFT, std::string("TOPLEFT") },
                { TOPRIGHT, std::string("TOPRIGHT") },
                { BOTRIGHT, std::string("BOTRIGHT") },
                { BOTLEFT, std::string("BOTLEFT") },
                { LEFTTOP, std::string("LEFTTOP") },
                { RIGHTTOP, std::string("RIGHTTOP") },
                { RIGHTBOT, std::string("RIGHTBOT") },
                { LEFTBOT, std::string("LEFTBOT") }
        };
        auto   it  = enum_strings.find(orientation);
        return it == enum_strings.end() ? "Out of range" : it->second;
    }

class IIIFImageError : public IIIFError {
    public:
        inline IIIFImageError(const char *file, int line, int errnum = 0)
            : IIIFError(file, line, "", errnum) {
            cname = __func__;
        }

        inline IIIFImageError(const char *file, int line, const char *msg, int errnum = 0)
            : IIIFError(file, line, msg, errnum) {
            cname = __func__;
        }

        inline IIIFImageError(const char *file_p, int line_p, const std::string &msg_p, int errnum_p = 0)
                : IIIFError(file_p, line_p, msg_p, errnum_p) {
            cname = __func__;
        }

        inline friend std::ostream &operator<<(std::ostream &outStream, const IIIFImageError &rhs) {
            std::string errStr = rhs.to_string();
            outStream << errStr; // TODO: remove the endl, the logging code should do it
            return outStream;
        }
        //============================================================================
    };

    class IIIFIO;

    /*!
    * \class SipiImage
    *
    * Base class for all images in the Sipi package.
    * This class implements all the data and handling (methods) associated with
    * images in Sipi. Please note that the map of io-classes (see \ref SipiIO) has to
    * be instantiated in the SipiImage.cpp! Thus adding a new file format requires that SipiImage.cpp
    * is being modified!
    */
    class IIIFImage {
        friend IIIFImage separateToContig(IIIFImage img, unsigned int sll);
        friend std::unique_ptr<byte[]> cvrt1BitTo8Bit(const IIIFImage &img, unsigned int sll, unsigned int black, unsigned int white);
        friend std::unique_ptr<byte[]> cvrt8BitTo1bit(const IIIFImage &img, unsigned int &sll);
        friend class IIIFIcc;       //!< We need SipiIcc as friend class
        friend class IIIFIOTiff;    //!< I/O class for the TIFF file format
        friend class IIIFIOJ2k;     //!< I/O class for the JPEG2000 file format
        friend class IIIFIOJpeg;    //!< I/O class for the JPEG file format
        friend class IIIFIOPng;     //!< I/O class for the PNG file format
    private:
        static std::unordered_map<std::string, std::shared_ptr<IIIFIO>> io; //!< member variable holding a map of I/O class instances for the different file formats
        static byte bilinn(const byte buf[], int nx, double x, double y, int c, int n);

        static word bilinn(const word buf[], int nx, double x, double y, int c, int n);

        void ensure_exif();

    protected:
        size_t nx;         //!< Number of horizontal pixels (width)
        size_t ny;         //!< Number of vertical pixels (height)
        size_t nc;         //!< Total number of samples per pixel
        size_t bps;        //!< bits per sample. Currently only 8 and 16 are supported
        std::vector<ExtraSamples> es; //!< meaning of extra samples
        Orientation orientation;            //!< Orientation/location of (0,0)
        PhotometricInterpretation photo;    //!< Image type, that is the meaning of the channels
        std::unique_ptr<byte[]> bpixels;   //!< Pointer to block of memory holding the pixels
        std::unique_ptr<word[]> wpixels;   //!< Pointer to block of memory holding the pixels
        std::shared_ptr<IIIFXmp> xmp{};   //!< Pointer to instance SipiXmp class (\ref SipiXmp), or NULL
        std::shared_ptr<IIIFIcc> icc{};   //!< Pointer to instance of SipiIcc class (\ref SipiIcc), or NULL
        std::shared_ptr<IIIFIptc> iptc{}; //!< Pointer to instance of SipiIptc class (\ref SipiIptc), or NULL
        std::shared_ptr<IIIFExif> exif{}; //!< Pointer to instance of SipiExif class (\ref SipiExif), or NULL
        IIIFEssentials emdata; //!< Metadata to be stored in file header
        Connection *conobj; //!< Pointer to HTTP connection
        SkipMetadata skip_metadata; //!< If true, all metadata is stripped off

    public:
        //
        /*!
         * Default constructor. Creates an empty image
         */
        IIIFImage();

        /*!
         * Copy constructor. Makes a deep copy of the image
         *
         * \param[in] img_p An existing instance if SipiImage
         */
        IIIFImage(const IIIFImage &img_p);

        /*!
         * Copy constructor for std::move semantics
         * @param img_p
         */
        IIIFImage(IIIFImage &&img_p) noexcept;

        /*!
         * Create an empty image with the pixel buffer available, but all pixels set to 0. Orientation is
         * assumed to be TOPLEFT!
         *
         * \param[in] nx_p Dimension in x direction
         * \param[in] ny_p Dimension in y direction
         * \param[in] nc_p Number of channels
         * \param[in] bps_p Bits per sample, either 8 or 16 are allowed
         * \param[in] photo_p The photometric interpretation
         */
        [[maybe_unused]]
        IIIFImage(size_t nx_p, size_t ny_p, size_t nc_p, size_t bps_p, PhotometricInterpretation photo_p);

        /*!
         * Getter for nx
         */
        inline size_t getNx() const { return nx; };

        /*!
         * Getter for ny
         */
        inline size_t getNy() const { return ny; };

        /*!
         * Getter for nc (includes alpha channels!)
         */
        inline size_t getNc() const { return nc; };

        /*!
         * Getter for number of alpha channels
         */
        inline size_t getNalpha() const { return es.size(); }

        /*!
         * Get bits per sample of image
         * @return bis per sample (bps)
         */
        [[nodiscard]]
        inline size_t getBps() const { return bps; }

        /*!
         * Get orientation
         * @return Returns orientation tag
         */
        inline Orientation getOrientation() const { return orientation; };

        /*!
         * Set orientation parameter
         * @param ori orientation to be set
         */
        inline void setOrientation(Orientation ori) { orientation = ori; };

        /*!
         * Getter for photometric interpretation
         * @return
         */
        [[nodiscard]]
        inline PhotometricInterpretation getPhoto() const { return photo; }

        /*!
         * Getter for Exif data
         * @return Shared pointer to IIIFExif instance or nullptr
         */
        inline std::shared_ptr<IIIFExif> getExif() const { return exif; }

        /*!
         * Getter for IPTC data
         * @return Shared poinmter to IPTC instance or nullptr
         */
        inline std::shared_ptr<IIIFIptc> getIptc() const { return iptc; }

        /*!
         * Getter for XMP data
         * @return shared pointer to XMP instance or nullptr
         */
        inline std::shared_ptr<IIIFXmp> getXmp() const { return xmp; }

        /*!
         * Getter for ICC data
         * @return shared pointer to ICC instance or nullptr
         */
        inline std::shared_ptr<IIIFIcc> getIcc() const { return icc; }

        /*! Destructor
         *
         * Destroys the image and frees all the resources associated with it
         */
        ~IIIFImage() = default;

        /*!
         * Sets a pixel to a given value
         *
         * \param[in] x X position
         * \param[in] y Y position
         * \param[in] c Color channels
         * \param[in] val Pixel value
         */
        [[maybe_unused]]
        void setPixel(unsigned int x, unsigned int y, unsigned int c, int val);

        /*!
         * Assignment operator
         *
         * Makes a deep copy of the instance
         *
         * \param[in] img_p Instance of a SipiImage
         */
        IIIFImage &operator=(const IIIFImage &img_p);

        IIIFImage &operator=(IIIFImage &&img_p) noexcept;



        /*!
         * Set the metadata that should be skipped in writing a file
         *
         * \param[in] smd Logical "or" of bitmasks for metadata to be skipped
         */
        inline void setSkipMetadata(SkipMetadata smd) { skip_metadata = smd; };


        /*!
         * Stores the connection parameters of the shttps server in an Image instance
         *
         * \param[in] conn_p Pointer to connection data
         */
        inline void connection(Connection *conobj_p) { conobj = conobj_p; };

        /*!
         * Retrieves the connection parameters of the mongoose server from an Image instance
         *
         * \returns Pointer to connection data
         */
        [[nodiscard]]
        inline Connection *connection() const { return conobj; };

        inline void essential_metadata(const IIIFEssentials &emdata_p) { emdata = emdata_p; }

        [[nodiscard]]
        inline IIIFEssentials essential_metadata() const { return emdata; }

        /*!
         * Read an image from the given path
         *
         * \param[in] filepath A string containing the path to the image file
         * \param[in] region Pointer to a SipiRegion which indicates that we
         *            are only interested in this region. The image will be cropped.
         * \param[in] size Pointer to a size object. The image will be scaled accordingly
         * \param[in] force_bps_8 We want in any case a 8 Bit/sample image. Reduce if necessary
         *
         * \throws SipiError
         */
        static IIIFImage read(std::string filepath,
                              int pagenum = 0,
                              std::shared_ptr<IIIFRegion> region = nullptr,
                              std::shared_ptr<IIIFSize> size = nullptr,
                              bool force_bps_8 = false,
                              ScalingQuality scaling_quality = {HIGH, HIGH, HIGH, HIGH});

        /*!
         * Read an image that is to be considered an "original image". In this case
         * a SipiEssentials object is created containing the original name, the
         * original mime type. In addition also a checksum of the pixel values
         * is added in order to guarantee the integrity of the image pixels.
         * if the image is written as J2K or as TIFF image, these informations
         * are added to the file header (in case of TIFF as a private tag 65111,
         * in case of J2K as comment box).
         * If the file read already contains a SipiEssentials as embedded metadata,
         * it is not overwritten, put the embedded and pixel checksums are compared.
         *
         * \param[in] filepath A string containing the path to the image file
         * \param[in] region Pointer to a SipiRegion which indicates that we
         *            are only interested in this region. The image will be cropped.
         * \param[in] size Pointer to a size object. The image will be scaled accordingly
         * \param[in] origname Original file name
         * \param[in] htype The checksum method that should be used if the checksum is
         *            being calculated for the first time.
         *
         * \returns true, if everything worked. False, if the checksums do not match.
         */
        static IIIFImage readOriginal(const std::string &filepath,
                                      int pagenum,
                                      std::shared_ptr<IIIFRegion> region,
                                      std::shared_ptr<IIIFSize> size,
                                      const std::string &origname,
                                      HashType htype);


        /*!
         * Read an image that is to be considered an "original image". In this case
         * a SipiEssentials object is created containing the original name, the
         * original mime type. In addition also a checksum of the pixel values
         * is added in order to guarantee the integrity of the image pixels.
         * if the image is written as J2K or as TIFF image, these informations
         * are added to the file header (in case of TIFF as a private tag 65111,
         * in case of J2K as comment box).
         * If the file read already contains a SipiEssentials as embedded metadata,
         * it is not overwritten, put the embedded and pixel checksums are compared.
         *
         * \param[in] filepath A string containing the path to the image file
         * \param[in] region Pointer to a SipiRegion which indicates that we
         *            are only interested in this region. The image will be cropped.
         * \param[in] size Pointer to a size object. The image will be scaled accordingly
         * \param[in] htype The checksum method that should be used if the checksum is
         *            being calculated for the first time.
         *
         * \returns true, if everything worked. False, if the checksums do not match.
         */
        [[maybe_unused]] static IIIFImage readOriginal(const std::string &filepath,
                                      int pagenum = 0,
                                      std::shared_ptr<IIIFRegion> region = nullptr,
                                      std::shared_ptr<IIIFSize> size = nullptr,
                                      HashType htype = HashType::sha256);

        /*!
         * Get the dimension of the image
         *
         * \param[in] filepath Pathname of the image file
         * \param[in] pagenum Page that is to be used (for PDF's and multipage TIF's only, first page is 1)
         * \return Info about image (see SipiImgInfo)
         */
        [[maybe_unused]] static IIIFImgInfo getDim(const std::string &filepath, int pagenum = 0) ;

        /*!
         * Get the dimension of the image object
         *
         * @param[out] width Width of the image in pixels
         * @param[out] height Height of the image in pixels
         */
        [[maybe_unused]] void getDim(size_t &width, size_t &height) const;

        /*!
         * Write an image to somewhere
         *
         * This method writes the image to a destination. The destination can be
         * - a file if w path (filename) is given
         * - stdout of the filepath is "-"
         * - to the websocket, if the filepath is the string "HTTP" (given the webserver is activated)
         *
         * \param[in] ftype The file format that should be used to write the file. Supported are
         * - "tif" for TIFF files
         * - "j2k" for JPEG2000 files
         * - "png" for PNG files
         * \param[in] filepath String containing the path/filename
         */
        void write(const std::string &ftype, const std::string &filepath, const IIIFCompressionParams &params = {});


        /*!
         * Convert full range YCbCr (YCC) to RGB colors
         */
        [[maybe_unused]] void convertYCC2RGB();


        /*!
         * Converts the image representation
         *
         * \param[in] target_icc_p ICC profile which determines the new image representation
         * \param[in] bps Bits/sample of the new image representation
         */
        void convertToIcc(const IIIFIcc &target_icc_p, int new_bps);


        /*!
         * Removes a channel from a multi component image
         *
         * \param[in] chan Index of component to remove, starting with 0
         */
        [[maybe_unused]] void removeChan(unsigned int chan);

        /*!
         * Crops an image to a region
         *
         * \param[in] x Horizontal start position of region. If negative, it's set to 0, and the width is adjusted
         * \param[in] y Vertical start position of region. If negative, it's set to 0, and the height is adjusted
         * \param[in] width Width of the region. If the region goes beyond the image dimensions, it's adjusted.
         * \param[in] height Height of the region. If the region goes beyond the image dimensions, it's adjusted
         */
        [[maybe_unused]]
        void crop(int x, int y, size_t width = 0, size_t height = 0);

        /*!
         * Crops an image to a region
         *
         * \param[in] Pointer to SipiRegion
         * \param[in] ny Vertical start position of region. If negative, it's set to 0, and the height is adjusted
         * \param[in] width Width of the region. If the region goes beyond the image dimensions, it's adjusted.
         * \param[in] height Height of the region. If the region goes beyond the image dimensions, it's adjusted
         */
        [[maybe_unused]] bool crop(const std::shared_ptr<IIIFRegion> &region);

        /*!
         * Resize an image using a high speed algorithm which may result in poor image quality
         *
         * \param[in] nnx New horizontal dimension (width)
         * \param[in] nny New vertical dimension (height)
         */
        bool scaleFast(size_t nnx, size_t nny);

        /*!
         * Resize an image using some balance between speed and quality
         *
         * \param[in] nnx New horizontal dimension (width)
         * \param[in] nny New vertical dimension (height)
         */
        bool scaleMedium(size_t nnx, size_t nny);

        /*!
         * Resize an image using the best (but slow) algorithm
         *
         * \param[in] nnx New horizontal dimension (width)
         * \param[in] nny New vertical dimension (height)
         */
        bool scale(size_t nnx = 0, size_t nny = 0);


        /*!
         * Rotate an image
         *
         * The angles 0, 90, 180, 270 are treated specially!
         *
         * \param[in] angle Rotation angle
         * \param[in] mirror If true, mirror the image before rotation
         */
        bool rotate(float angle, bool mirror = false);

        /*!
         * Convert an image from 16 to 8 bit. The algorithm just divides all pixel values
         * by 256 using the ">> 8" operator (fast & efficient)
         *
         * \returns Returns true on success, false on error
         */
        bool to8bps();

        /*!
         * Convert an image to a bitonal representation using Steinberg-Floyd dithering.
         *
         * The method does nothing if the image is already bitonal. Otherwise, the image is converted
         * into a gray value image if necessary and then a FLoyd-Steinberg dithering is applied.
         *
         * \returns Returns true on success, false on error
         */
        bool toBitonal();

        /*!
         * Add a watermark to a file...
         *
         * \param[in] wmfilename Path to watermakfile (which must be a TIFF file at the moment)
         */
        bool add_watermark(const std::string &wmfilename);


        /*!
         * Calculates the difference between 2 images.
         *
         * The difference between 2 images can contain (and usually will) negative values.
         * In order to create a standard image, the values at "0" will be lifted to 127 (8-bit images)
         * or 32767. The span will be defined by max(minimum, maximum), where minimum and maximum are
         * absolute values. Thus a new pixelvalue will be calculated as follows:
         * ```
         * int maxmax = abs(min) > abs(max) ? abs(min) : abs(min);
         * newval = (byte) ((oldval + maxmax)*UCHAR_MAX/(2*maxmax));
         * ```
         * \param[in] rhs right hand side of "-="
         */
        IIIFImage &operator-=(const IIIFImage &rhs);

        /*!
         * Calculates the difference between 2 images.
         *
         * The difference between 2 images can contain (and usually will) negative values.
         * In order to create a standard image, the values at "0" will be lifted to 127 (8-bit images)
         * or 32767. The span will be defined by max(minimum, maximum), where minimum and maximum are
         * absolute values. Thus a new pixelvalue will be calculated as follows:
         * ```
         * int maxmax = abs(min) > abs(max) ? abs(min) : abs(min);
         * newval = (byte) ((oldval + maxmax)*UCHAR_MAX/(2*maxmax));
         * ```
         *
         * \param[in] lhs left-hand side of "-" operator
         * \param[in] rhs right hand side of "-" operator
         */
        IIIFImage &operator-(const IIIFImage &rhs);

        IIIFImage &operator+=(const IIIFImage &rhs);

        IIIFImage &operator+(const IIIFImage &rhs);

        bool operator==(const IIIFImage &rhs);

        /*!
        * The overloaded << operator which is used to write the error message to the output
        *
        * \param[in] lhs The output stream
        * \param[in] rhs Reference to an instance of a SipiImage
        * \returns Returns ostream object
        */
        friend std::ostream &operator<<(std::ostream &lhs, const IIIFImage &rhs);
    };
}

#endif
