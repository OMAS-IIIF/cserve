//
// Created by Lukas Rosenthaler on 22.07.22.
//

#ifndef CSERVER_IIIFIMGTOOLS_H
#define CSERVER_IIIFIMGTOOLS_H

#include "IIIFImage.h"

namespace cserve {

    /*!
 * Converts an image from RRRRRR...GGGGGG...BBBBB to RGBRGBRGBRGB....
 * \param img Pointer to SipiImage instance
 * \param[in] sll Scanline length in bytes
 */
    IIIFImage separateToContig(IIIFImage img, unsigned int sll);

    std::unique_ptr<uint8_t[]> separateToContig(std::unique_ptr<uint8_t[]> inbuf, int nx, int ny, int nc, int sll);

    std::unique_ptr<uint16_t[]> separateToContig(std::unique_ptr<uint16_t[]> inbuf, int nx, int ny, int nc, int sll);

    std::unique_ptr<byte[]> crop(std::unique_ptr<byte[]> inbuf, int nx, int ny, int nc, const std::shared_ptr<IIIFRegion> &region);

    std::unique_ptr<word[]> crop(std::unique_ptr<word[]> inbuf, int nx, int ny, int nc, const std::shared_ptr<IIIFRegion> &region);

    /*!
     * Converts a bitonal 1 bit image to a bitonal 8 bit image
     *
     * \param img Pointer to SipiImage instance
     * \param[in] Length of scanline in bytes
     * \param[in] Value to be used for black pixels
     * \param[in] Value to be used for white pixels
     */
    std::unique_ptr<byte[]> cvrt1BitTo8Bit(const IIIFImage &img, unsigned int sll, unsigned int black, unsigned int white);

    /*!
     * Converts a 8 bps bitonal image to 1 bps bitonal image
     *
     * \param[in] img Reference to SipiImage instance
     * \param[out] sll Scan line length
     * \returns Buffer of 1-bit data (padded to bytes). NOTE: This buffer has to be deleted by the caller!
     */
    std::unique_ptr<byte[]> cvrt8BitTo1bit(const IIIFImage &img, unsigned int &sll);

}

#endif //CSERVER_IIIFIMGTOOLS_H
