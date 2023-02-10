//
// Created by Lukas Rosenthaler on 22.07.22.
//

#ifndef CSERVER_IIIFIMGTOOLS_H
#define CSERVER_IIIFIMGTOOLS_H

#include "IIIFImage.h"

namespace cserve {

    std::unique_ptr<uint8_t[]> separateToContig(std::unique_ptr<uint8_t[]> inbuf, size_t nx, size_t ny, size_t nc, size_t sll);

    std::unique_ptr<uint16_t[]> separateToContig(std::unique_ptr<uint16_t[]> inbuf, size_t nx, size_t ny, size_t nc, size_t sll);

    std::unique_ptr<byte[]> crop(std::unique_ptr<byte[]> inbuf, size_t nx, size_t ny, size_t nc, const std::shared_ptr<IIIFRegion> &region);

    std::unique_ptr<word[]> crop(std::unique_ptr<word[]> inbuf, size_t nx, size_t ny, size_t nc, const std::shared_ptr<IIIFRegion> &region);

}

#endif //CSERVER_IIIFIMGTOOLS_H
