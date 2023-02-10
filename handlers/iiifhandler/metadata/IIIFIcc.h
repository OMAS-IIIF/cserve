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
#ifndef __iiif_icc_h
#define __iiif_icc_h

#include <string>
#include <vector>

#include <cstdio>
#include <climits>
#include <ctime>
#include <cstddef>


#define CMS_NO_REGISTER_KEYWORD 1

#include <lcms2.h>

#include "../IIIFPhotometricInterpretation.h"


namespace cserve {

    extern void icc_error_logger(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *Text);


    /*! Defines predefined profiles which are used by SipiIcc */
    typedef enum {
        icc_undefined,      //!< No profile data defined, empty profile
        icc_unknown,        //!< This is a unknown, proprietary profile, probably embedded
        icc_sRGB,           //!< Standard sRGB profile
        icc_AdobeRGB,       //!< Standard AdobeRGB profile
        icc_RGB,            //!< A RGB profile that's given with parameters such as white point, primary colors etc. (see TIFF specs)
        icc_CYMK_standard,  //!< A "standard" CMYK profile. We currently use the "USWebCoatedSWOP" profile
        icc_GRAY_D50,       //!< A standard profile for gray value images using a D50 light source and a gamma of 2.2
        icc_LUM_D65,        //!< A standard profile for gray value images as used be JPEG2000 JP2_sLUM_SPACE
        icc_ROMM_GRAY,       //!< A profile used by the JPEG2000 ISO suite....
        icc_LAB
    } PredefinedProfiles;

    /*!
     * This class implements the handling of ICC color profiles
     */
    class IIIFIcc {
    private:
        cmsHPROFILE icc_profile{};            //!< Handle of the littleCMS profile data
        PredefinedProfiles profile_type;    //!< Profile type that is represented

    public:
        /*!
         * Constructor (default) which results in empty, undefined profile
         */
        inline IIIFIcc() {
            icc_profile = nullptr;
            profile_type = icc_undefined;
        };

        /*!
         * Constructor which takes a blob that contains the ICC profile
         * \param[in] buf Buffer holding the binary profile data
         * \param]in len Length of the buffer
         */
        IIIFIcc(const unsigned char *buf, int len);

        /*!
         * Copy constructor. Uses deep copy and allocates new data buffers.
         * \param[in] icc_p Profile that acts as template for the new profile.
         */
        IIIFIcc(const IIIFIcc &icc_p);

        IIIFIcc(IIIFIcc &&icc_p) noexcept;

        /*!
         * Constructor using littleCMS profile
         * \param[in] icc_profile_p LittleCMS profile
         */
        explicit IIIFIcc(cmsHPROFILE &icc_profile_p);

        /**
         * Constructor to create a predefined profile
         * \param[in] predef Desired predefined profile
         */
        explicit IIIFIcc(PredefinedProfiles predef);

        /**
         * Constructor of an ICC RGB profile using white point, primaries and transfer function (if available),
         * otherwise a curve with gamma = 2.2 is assumed. The parameters must be given as retrieved by libtiff!
         * \param[in] white_point Array of 2 floats giving the x and y of the whitepoint
         * \param[in] primaries Array of 6 floats giving the x and y of each red, green and blue primaries
         * \param[in] tfunc Transfer function tables as retrieved by libtiff with either (1 << bitspersample) or 3*(1 << bitspersample) entries
         * \param[in] Length of tranfer function table
         */
        IIIFIcc(const float white_point_p[], const float primaries_p[], const unsigned short *tfunc = nullptr,
                int tfunc_len = 0);

        /**
         * Destructor
         */
        ~IIIFIcc();

        /*!
         * Assignment operator which makes deep copy
         * \param[in] rhs Instance of SipiIcc
         */
        IIIFIcc &operator=(const IIIFIcc &rhs);

        IIIFIcc &operator=(IIIFIcc &&rhs) noexcept;

        /*!
         * Get the blob containing the ICC profile
         * @param[out] len Length of the buffer returned
         * @returns Buffer containing the binary ICC profile
         */
        std::unique_ptr<unsigned char[]> iccBytes(unsigned int &len);

        /*!
         * Get the blob containing the ICC profile as std::vector
         * @return std:vector containing the binary ICC profile
         */
        std::vector<unsigned char> iccBytes();

        /*!
         * Retireve the littleCMS profile
         * \returns Handle to littleCMS profile
         */
        [[nodiscard]]
        cmsHPROFILE getIccProfile() const;

        /*!
         * Get the profile type
         * \retruns Gives the predefined profile type
         */
        [[nodiscard]] inline PredefinedProfiles getProfileType() const { return profile_type; };

        /*!
         * returns a littleCMS formatter with the given bits/sample
         * \param[in] bps Desired bits/sample
         * \returns Formatter as used by cmsTransfrom
         */
        unsigned int iccFormatter(size_t bps) const;

        /*!
         * returns a littleCMS formatter with the given bits/sample for the given SipImage
         * \param img The SipiImage instance whose color profile is used to create the formatter
         * \returns Formatter as used by cmsTransfrom
         */
        static unsigned int iccFormatter(size_t nc, size_t bps, PhotometricInterpretation photo) ;

        /**
         * Print info to output stream
         * \param[in] lhs Output stream
         * \param[in] rhs SipiIcc instance
         */
        friend std::ostream &operator<<(std::ostream &lhs, IIIFIcc &rhs);
    };

}

#endif
