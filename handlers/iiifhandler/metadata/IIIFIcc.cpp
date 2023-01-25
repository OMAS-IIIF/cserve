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

#include <memory>
#include <ctime>

#include "IIIFIcc.h"


static const char file_[] = __FILE__;

#include "Cserve.h"
#include "../IIIFError.h"
#include "../AdobeRGB1998_icc.h"
#include "../USWebCoatedSWOP_icc.h"
#include "../Rec709-Rec1886_icc.h"

#include "../IIIFImage.h"
#include "makeunique.h"
#include "../IIIFPhotometricInterpretation.h"

namespace cserve {

    void icc_error_logger(cmsContext ContextID, cmsUInt32Number ErrorCode, const char *text) {
        Server::logger()->error("ICC-CMS-ERROR: {}", text);
    }

    IIIFIcc::IIIFIcc(const unsigned char *icc_buf, int icc_len) {
        cmsSetLogErrorHandler(icc_error_logger);
        if ((icc_profile = cmsOpenProfileFromMem(icc_buf, icc_len)) == nullptr) {
            std::cerr << "THROWING ERROR IN ICC" << std::endl;
            throw IIIFError(file_, __LINE__, "cmsOpenProfileFromMem failed");
        }
        unsigned int len = cmsGetProfileInfoASCII(icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        auto buf = std::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        if (strcmp(buf.get(), "sRGB IEC61966-2.1") == 0) {
            profile_type = icc_sRGB;
        }
        else if (strncmp(buf.get(), "AdobeRGB", 8) == 0) {
            profile_type = icc_AdobeRGB;
        }
        else {
            profile_type = icc_unknown;
        }
    }

    IIIFIcc::IIIFIcc(const IIIFIcc &icc_p) {
        cmsSetLogErrorHandler(icc_error_logger);
        if (icc_p.icc_profile != nullptr) {
            cmsUInt32Number len = 0;
            cmsSaveProfileToMem(icc_p.icc_profile, nullptr, &len);
            auto buf = std::make_unique<char[]>(len);
            cmsSaveProfileToMem(icc_p.icc_profile, buf.get(), &len);
            icc_profile = cmsOpenProfileFromMem(buf.get(), len);

            if (icc_profile == nullptr) {
                throw IIIFError(file_, __LINE__, "cmsOpenProfileFromMem failed");
            }

            profile_type = icc_p.profile_type;
        }
        else {
            icc_profile = nullptr;
            profile_type = icc_undefined;
        }
    }

    IIIFIcc::IIIFIcc(IIIFIcc &&other) noexcept : icc_profile(nullptr), profile_type(icc_undefined) {
        profile_type = other.profile_type;
        icc_profile = other.icc_profile;

        other.profile_type = icc_undefined;
        other.icc_profile = nullptr;
    }

    IIIFIcc::IIIFIcc(cmsHPROFILE &icc_profile_p) {
        cmsSetLogErrorHandler(icc_error_logger);
        if (icc_profile_p != nullptr) {
            cmsUInt32Number len = 0;
            cmsSaveProfileToMem(icc_profile_p, nullptr, &len);
            auto buf = std::make_unique<char[]>(len);
            cmsSaveProfileToMem(icc_profile_p, buf.get(), &len);
            if ((icc_profile = cmsOpenProfileFromMem(buf.get(), len)) == nullptr) {
                throw IIIFError(file_, __LINE__, "cmsOpenProfileFromMem failed");
            }
        }
        profile_type = icc_unknown;
    }

    IIIFIcc::IIIFIcc(PredefinedProfiles predef) {
        cmsSetLogErrorHandler(icc_error_logger);
        switch (predef) {
            case icc_undefined: {
                icc_profile = nullptr;
                profile_type = icc_undefined;
            }
            case icc_unknown: {
                throw IIIFError(file_, __LINE__, "Profile type \"icc_inknown\" not allowed");
            }
            case icc_sRGB: {
                icc_profile = cmsCreate_sRGBProfile();
                profile_type = icc_sRGB;
                break;
            }
            case icc_AdobeRGB: {
                icc_profile = cmsOpenProfileFromMem(AdobeRGB1998_icc, AdobeRGB1998_icc_len);
                profile_type = icc_AdobeRGB;
                break;
            }
            case icc_RGB: {
                throw IIIFError(file_, __LINE__, "Profile type \"icc_RGB\" uses other constructor");
            }
            case icc_CYMK_standard: {
                icc_profile = cmsOpenProfileFromMem(USWebCoatedSWOP_icc, USWebCoatedSWOP_icc_len);
                profile_type = icc_CYMK_standard;
                break;
            }
            case icc_GRAY_D50: {
                cmsContext context = cmsCreateContext(nullptr, nullptr);
                cmsToneCurve *gamma_2_2 = cmsBuildGamma(context, 2.2);
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), gamma_2_2);
                cmsFreeToneCurve(gamma_2_2);
                cmsDeleteContext(context);
                profile_type = icc_GRAY_D50;
                break;
            }
            case icc_LUM_D65: {
                cmsContext context = cmsCreateContext(nullptr, nullptr);
                cmsToneCurve *gamma_2_4 = cmsBuildGamma(context, 2.4);
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), gamma_2_4);
                cmsFreeToneCurve(gamma_2_4);
                cmsDeleteContext(context);
                profile_type = icc_LUM_D65;
                break;
            }
            case icc_ROMM_GRAY: {
                cmsContext context = cmsCreateContext(nullptr, nullptr);
                cmsToneCurve* gamma_1_8 = cmsBuildGamma(context, 1.8);
                icc_profile = cmsCreateGrayProfileTHR(context, cmsD50_xyY(), gamma_1_8);
                cmsFreeToneCurve(gamma_1_8);
                cmsDeleteContext(context);
                profile_type = icc_ROMM_GRAY;
                break;
            }
            case icc_LAB: {
                cmsContext context = cmsCreateContext(nullptr, nullptr);
                icc_profile = cmsCreateLab4Profile(NULL);
                cmsDeleteContext(context);
                profile_type = icc_LAB;
                break;
            }
        }
    }

    IIIFIcc::IIIFIcc(float white_point_p[], float primaries_p[], const unsigned short tfunc[], const int tfunc_len) {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsCIExyY white_point;
        white_point.x = white_point_p[0];
        white_point.y = white_point_p[1];
        //white_point.Y = 1.0;

        cmsCIExyYTRIPLE primaries;
        primaries.Red.x = primaries_p[0];
        primaries.Red.y = primaries_p[1];
        primaries.Red.Y = 1.0;
        primaries.Green.x = primaries_p[2];
        primaries.Green.y = primaries_p[3];
        primaries.Green.Y = 1.0;
        primaries.Blue.x = primaries_p[4];
        primaries.Blue.y = primaries_p[5];
        primaries.Blue.Y = 1.0;

        cmsContext context = cmsCreateContext(0, 0);
        cmsToneCurve *tonecurve[3];
        if (tfunc == nullptr) {
            tonecurve[0] = cmsBuildGamma(context, 2.2);
            tonecurve[1] = cmsBuildGamma(context, 2.2);
            tonecurve[2] = cmsBuildGamma(context, 2.2);
        }
        else {
            tonecurve[0] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc);
            tonecurve[1] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc + tfunc_len);
            tonecurve[2] = cmsBuildTabulatedToneCurve16(context, tfunc_len, tfunc + 2*tfunc_len);
        }

        icc_profile = cmsCreateRGBProfileTHR(context, &white_point, &primaries, tonecurve);
        profile_type = icc_RGB;
        cmsFreeToneCurveTriple(tonecurve);
    }

    IIIFIcc::~IIIFIcc() {
        if (icc_profile != nullptr) {
            cmsCloseProfile(icc_profile);
        }
    }

    IIIFIcc& IIIFIcc::operator=(const IIIFIcc &rhs) {
        if (this != &rhs) {
            if (rhs.icc_profile != nullptr) {
                unsigned int len = 0;
                cmsSaveProfileToMem(rhs.icc_profile, nullptr, &len);
                auto buf = std::make_unique<char[]>(len);
                cmsSaveProfileToMem(rhs.icc_profile, buf.get(), &len);
                if ((icc_profile = cmsOpenProfileFromMem(buf.get(), len)) == nullptr) {
                    throw IIIFError(file_, __LINE__, "cmsOpenProfileFromMem failed");
                }
            }
            profile_type = rhs.profile_type;
        }
        return *this;
    }

    IIIFIcc &IIIFIcc::operator=(IIIFIcc &&rhs) {
        if (this != &rhs) {
            profile_type = rhs.profile_type;
            icc_profile = rhs.icc_profile;

            rhs.profile_type = icc_undefined;
            rhs.icc_profile = nullptr;
        }
        return *this;
    }

    std::unique_ptr<unsigned char[]> IIIFIcc::iccBytes(unsigned int &len) {
        std::unique_ptr<unsigned char[]> buf = nullptr;
        len = 0;
        if (icc_profile != nullptr) {
            if (!cmsSaveProfileToMem(icc_profile, nullptr, &len)) {
                throw IIIFError(file_, __LINE__, "cmsSaveProfileToMem failed");
            }
            buf = std::make_unique<unsigned char[]>(len);
            if (!cmsSaveProfileToMem(icc_profile, buf.get(), &len))  {
                throw IIIFError(file_, __LINE__, "cmsSaveProfileToMem failed");
            }
        }
        return buf;
    }

    std::vector<unsigned char> IIIFIcc::iccBytes() {
        unsigned int len = 0;
        auto buf = iccBytes(len);
        std::vector<unsigned char> data (buf.get(), buf.get() + len);
        return data;
    }

    cmsHPROFILE IIIFIcc::getIccProfile()  const {
        return icc_profile;
    }

    unsigned int IIIFIcc::iccFormatter(int bps) const {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsUInt32Number format = (bps == 16) ? BYTES_SH(2) : BYTES_SH(1);
        cmsColorSpaceSignature csig = cmsGetColorSpace(icc_profile);

        switch (csig) {
            case cmsSigLabData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_Lab);
                break;
            }
            case cmsSigYCbCrData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_YCbCr);
                break;
            }
            case cmsSigRgbData: {
                format |= CHANNELS_SH(3) | PLANAR_SH(0) | COLORSPACE_SH(PT_RGB);
                break;
            }
            case cmsSigGrayData: {
                format |= CHANNELS_SH(1) | PLANAR_SH(0) | COLORSPACE_SH(PT_GRAY);
                break;
            }
            case cmsSigCmykData: {
                format |= CHANNELS_SH(4) | PLANAR_SH(0) | COLORSPACE_SH(PT_CMYK);
                break;
            }
            default: {
                throw IIIFError(file_, __LINE__, "Unsupported iccFormatter for given profile");
            }
        }
        return format;
    }


    unsigned int IIIFIcc::iccFormatter(int nc, int bps, PhotometricInterpretation photo)  const {
        cmsSetLogErrorHandler(icc_error_logger);
        cmsUInt32Number format = 0;
        switch (bps) {
            case 8: {
                format = BYTES_SH(1) | CHANNELS_SH(nc) | PLANAR_SH(0);
                break;
            }
            case 16: {
                format = BYTES_SH(2) | CHANNELS_SH(nc) | PLANAR_SH(0);
                break;
            }
            default: {
                throw IIIFError(file_, __LINE__, "Unsupported bits/sample (" + std::to_string(bps) + ")");
            }
        }
        switch (photo) {
            case MINISWHITE: {
                format |= COLORSPACE_SH(PT_GRAY);
                break;
            }
            case MINISBLACK: {
                format |= COLORSPACE_SH(PT_GRAY);
                break;
            }
            case RGB: {
                format |= COLORSPACE_SH(PT_RGB);
                break;
            }
            case PALETTE: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"PALETTE\" not supported");
                break;
            }
            case MASK: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"MASK\" not supported");
                break;
            }
            case SEPARATED: { // --> CMYK
                format |= COLORSPACE_SH(PT_CMYK);
                break;
            }
            case YCBCR: {
                format |= COLORSPACE_SH(PT_YCbCr);
                break;
            }
            case CIELAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case ICCLAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case ITULAB: {
                format |= COLORSPACE_SH(PT_Lab);
                break;
            }
            case CFA: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"Color Field Array (CFS)\" not supported");
                break;
            }
            case LOGL: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"LOGL\" not supported");
                break;
            }
            case LOGLUV: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"LOGLUV\" not supported");
                break;
            }
            case LINEARRAW: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"LINEARRAW\" not supported");
                break;
            }
            default: {
                throw IIIFError(file_, __LINE__, "Photometric interpretation \"unknown\" not supported");
            }
        }
        return format;
    }

    std::ostream &operator<< (std::ostream &outstr, IIIFIcc &rhs) {
        unsigned int len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        auto buf = std::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoDescription, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Description   : " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoManufacturer, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = std::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoManufacturer, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Manufacturer  : " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoModel, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = std::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoModel, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Model         : " << buf.get() << std::endl;

        len = cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoCopyright, cmsNoLanguage, cmsNoCountry, nullptr, 0);
        buf = std::make_unique<char[]>(len);
        cmsGetProfileInfoASCII(rhs.icc_profile, cmsInfoCopyright, cmsNoLanguage, cmsNoCountry, buf.get(), len);
        outstr << "ICC-Copyright     : " << buf.get() << std::endl;

        struct tm datetime{};
        if (cmsGetHeaderCreationDateTime(rhs.icc_profile, &datetime)) {
            outstr << "ICC-Date          : " << asctime(&datetime);
        }

        cmsProfileClassSignature sig = cmsGetDeviceClass(rhs.icc_profile);
        outstr << "ICC profile class : ";
        switch (sig) {
            case 0x73636E72: outstr << "cmsSigInputClass"; break;
            case 0x6D6E7472: outstr << "cmsSigDisplayClass"; break;
            case 0x70727472: outstr << "cmsSigOutputClass"; break;
            case 0x6C696E6B: outstr << "cmsSigLinkClass"; break;
            case 0x61627374: outstr << "cmsSigAbstractClass"; break;
            case 0x73706163: outstr << "cmsSigColorSpaceClass"; break;
            case 0x6e6d636c: outstr << "cmsSigInputClass"; break;
            default: outstr << "unknown";
        }
        outstr << std::endl;

        cmsFloat64Number version = cmsGetProfileVersion(rhs.icc_profile);
        outstr << "ICC Version       : " << version << std::endl;

        outstr << "ICC Matrix shaper : ";
        if (cmsIsMatrixShaper(rhs.icc_profile)) {
            outstr << "yes" << std::endl;
        }
        else {
            outstr << "no" << std::endl;
        }
        cmsColorSpaceSignature csig = cmsGetPCS(rhs.icc_profile);
        outstr << "ICC color space sigature : ";
        switch (csig) {
            case 0x58595A20: outstr << "cmsSigXYZData"; break;
            case 0x4C616220: outstr << "cmsSigLabData"; break;
            case 0x4C757620: outstr << "cmsSigLuvData"; break;
            case 0x59436272: outstr << "cmsSigYCbCrData"; break;
            case 0x59787920: outstr << "cmsSigYxyData"; break;
            case 0x52474220: outstr << "cmsSigRgbData"; break;
            case 0x47524159: outstr << "cmsSigGrayData"; break;
            case 0x48535620: outstr << "cmsSigHsvData"; break;
            case 0x484C5320: outstr << "cmsSigHlsData"; break;
            case 0x434D594B: outstr << "cmsSigCmykData"; break;
            case 0x434D5920: outstr << "cmsSigCmyData"; break;
            case 0x4D434831: outstr << "cmsSigMCH1Data"; break;
            case 0x4D434832: outstr << "cmsSigMCH2Data"; break;
            case 0x4D434833: outstr << "cmsSigMCH3Data"; break;
            case 0x4D434834: outstr << "cmsSigMCH4Data"; break;
            case 0x4D434835: outstr << "cmsSigMCH5Data"; break;
            case 0x4D434836: outstr << "cmsSigMCH6Data"; break;
            case 0x4D434837: outstr << "cmsSigMCH7Data"; break;
            case 0x4D434838: outstr << "cmsSigMCH8Data"; break;
            case 0x4D434839: outstr << "cmsSigMCH9Data"; break;
            /*
            case 0x4D43483A: outstr << "cmsSigMCHAData"; break;
            case 0x4D43483B: outstr << "cmsSigMCHBData"; break;
            case 0x4D43483C: outstr << "cmsSigMCHCData"; break;
            case 0x4D43483D: outstr << "cmsSigMCHDData"; break;
            case 0x4D43483E: outstr << "cmsSigMCHEData"; break;
            case 0x4D43483F: outstr << "cmsSigMCHFData"; break;
             */
            case 0x6e6d636c: outstr << "cmsSigNamedData"; break;
            case 0x31434C52: outstr << "cmsSig1colorData"; break;
            case 0x32434C52: outstr << "cmsSig2colorData"; break;
            case 0x33434C52: outstr << "cmsSig3colorData"; break;
            case 0x34434C52: outstr << "cmsSig4colorData"; break;
            case 0x35434C52: outstr << "cmsSig5colorData"; break;
            case 0x36434C52: outstr << "cmsSig6colorData"; break;
            case 0x37434C52: outstr << "cmsSig7colorData"; break;
            case 0x38434C52: outstr << "cmsSig8colorData"; break;
            case 0x39434C52: outstr << "cmsSig9colorData"; break;
            case 0x41434C52: outstr << "cmsSig10colorData"; break;
            case 0x42434C52: outstr << "cmsSig11colorData"; break;
            case 0x43434C52: outstr << "cmsSig12colorData"; break;
            case 0x44434C52: outstr << "cmsSig13colorData"; break;
            case 0x45434C52: outstr << "cmsSig14colorData"; break;
            case 0x46434C52: outstr << "cmsSig15colorData"; break;
            case 0x4C75764B: outstr << "cmsSigLuvKData"; break;
            default: outstr << "unknown";
        }
        outstr << std::endl;

        cmsUInt32Number intent = cmsGetHeaderRenderingIntent(rhs.icc_profile);
        outstr << "ICC rendering intent : ";
        switch (intent) {
            case 0: outstr << "INTENT_PERCEPTUAL"; break;
            case 1: outstr << "INTENT_RELATIVE_COLORIMETRIC"; break;
            case 2: outstr << "INTENT_SATURATION"; break;
            case 3: outstr << "INTENT_ABSOLUTE_COLORIMETRIC"; break;
            case 10: outstr << "INTENT_PRESERVE_K_ONLY_PERCEPTUAL"; break;
            case 11: outstr << "INTENT_PRESERVE_K_ONLY_RELATIVE_COLORIMETRIC"; break;
            case 12: outstr << "INTENT_PRESERVE_K_ONLY_SATURATION"; break;
            case 13: outstr << "INTENT_PRESERVE_K_PLANE_PERCEPTUAL"; break;
            case 14: outstr << "INTENT_PRESERVE_K_PLANE_RELATIVE_COLORIMETRIC"; break;
            case 15: outstr << "INTENT_PRESERVE_K_PLANE_SATURATION"; break;
            default: outstr << "unknown";
        }
        outstr << std::endl;
        return outstr;
    }

}
