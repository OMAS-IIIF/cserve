//
// Created by Lukas Rosenthaler on 25.07.22.
//

#ifndef CSERVER_IIIFPHOTOMETRICINTERPRETATION_H
#define CSERVER_IIIFPHOTOMETRICINTERPRETATION_H

/*
#include <sstream>
#include <utility>
#include <string>
#include <unordered_map>
#include <exception>
#include "IIIFError.h"
#include "metadata/IIIFXmp.h"
#include "metadata/IIIFIcc.h"
#include "metadata/IIIFIptc.h"
#include "metadata/IIIFExif.h"
#include "metadata/IIIFEssentials.h"
#include "iiifparser/IIIFRegion.h"
#include "iiifparser/IIIFSize.h"
#include "Connection.h"
#include "Hash.h"
*/

namespace cserve {
    /*! Implements the values of the photometric tag of the TIFF format */
    typedef enum : unsigned short {
        MINISWHITE = 0,     //!< B/W or gray value image with 0 = white and 1 (255) = black
        MINISBLACK = 1,     //!< B/W or gray value image with 0 = black and 1 (255) = white (is default in SIPI)
        RGB = 2,            //!< Color image with RGB values
        PALETTE = 3,        //!< Palette color image, is not suppoted by Sipi
        MASK = 4,           //!< Mask image, not supported by Sipi
        SEPARATED = 5,      //!< Color separated image, is assumed to be CMYK
        YCBCR = 6,          //!< Color representation with YCbCr, is supported by Sipi, but converted to an ordinary RGB
        CIELAB = 8,         //!< CIE*a*b image, only very limited support (untested!)
        ICCLAB = 9,         //!< ICCL*a*b image, only very limited support (untested!)
        ITULAB = 10,        //!< ITUL*a*b image, not supported yet (what is this by the way?)
        CFA = 32803,        //!< Color field array, used for DNG and RAW image. Not supported!
        LOGL = 32844,       //!< LOGL format (not supported)
        LOGLUV = 32845,     //!< LOGLuv format (not supported)
        LINEARRAW = 34892,  //!< Linear raw array for DNG and RAW formats. Not supported!
        INVALID = 65535     //!< an invalid value
    } PhotometricInterpretation;
}
#endif //CSERVER_IIIFPHOTOMETRICINTERPRETATION_H
