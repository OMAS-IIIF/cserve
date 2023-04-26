//
// Created by Lukas Rosenthaler on 22.07.22.
//

#ifndef CSERVER_IIIFIMGTOOLS_H
#define CSERVER_IIIFIMGTOOLS_H

#include <memory>
#include <vector>

#include "IIIFImage.h"

namespace cserve {

    template<typename T>
    void one2eight(const uint8_t *in, T *out, uint32_t len, uint8_t black, uint8_t white) {
        static uint8_t mask[8] = {0b10000000,
                                  0b01000000,
                                  0b00100000,
                                  0b00010000,
                                  0b00001000,
                                  0b00000100,
                                  0b00000010,
                                  0b00000001};

        for (uint32_t i = 0; i < len; i += 8) {
            for (uint32_t k = 0; (k < 8) && ((k + i) < len); ++k) {
                out[i + k] = mask[k] & in[i] ? white : black;
            }
        }
    }

    template<typename T>
    void four2eight(const uint8_t *in, T *out, uint32_t len, bool is_palette = false) {
        static uint8_t mask[2] = { 0b11110000, 0b00001111 };

        if (is_palette) {
            for (uint32_t i = 0; i < len; i += 2) {
                out[i] = mask[0] & in[i] >> 4;
                if ((i + 1) < len) {
                    out[i + 1] = mask[1] & in[i];
                }
            }
        }
        else {
            for (uint32_t i = 0; i < len; i += 2) {
                out[i] = mask[0] & in[i] >> 4;
                if ((i + 1) < len) {
                    out[i + 1] = (mask[1] & in[i]) << 4;
                }
            }
        }
    }

    template<typename T>
    void twelve2sixteen(const uint8_t *in, T *out, uint32_t len, bool is_palette = false) {
        static uint8_t mask[2] = { 0b11110000, 0b00001111 };
        if (is_palette) {
            for (uint32_t i = 0; i < len; i += 3) {
                out[i] = (in[i] << 4) | ((in[i + 1] & mask[0]) >> 4);
                if ((i + 1) < len) {
                    out[i + 1] = ((in[i + 1] & mask[1]) << 8) | in[i + 2];
                }
            }
        }
        else {
            for (uint32_t i = 0; i < len; i += 3) {
                out[i] = (in[i] << 4) | ((in[i + 1] & mask[0]) >> 4);
                if ((i + 1) < len) {
                    out[i + 1] = (((in[i + 1] & mask[1]) << 8) | in[i + 2]) << 4;
                }
            }
        }
    }

    std::unique_ptr<uint8_t[]> cvrt1BitTo8Bit(std::unique_ptr<uint8_t[]> &&inbuf,
                                              uint32_t nx, uint32_t ny, uint32_t sll,
                                              uint8_t black, uint8_t white);

    std::unique_ptr<uint8_t[]> cvrt4BitTo8Bit(std::unique_ptr<uint8_t[]> &&inbuf,
                                              uint32_t nx, uint32_t ny, uint32_t sll);

    template<typename T>
    std::unique_ptr<T> separateToContig(std::unique_ptr<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    extern template std::unique_ptr<uint8_t[]> separateToContig<uint8_t[]>(std::unique_ptr<uint8_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);
    extern template std::unique_ptr<uint16_t[]> separateToContig<uint16_t[]>(std::unique_ptr<uint16_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    template<typename T>
    std::vector<T> separateToContig(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    extern template std::vector<uint8_t> separateToContig<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);
    extern template std::vector<uint16_t> separateToContig<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    template<typename T>
    std::unique_ptr<T> doCrop(std::unique_ptr<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc,
                              const std::shared_ptr<IIIFRegion> &region);

    extern template std::unique_ptr<uint8_t[]> doCrop<uint8_t[]>(std::unique_ptr<uint8_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);
    extern template std::unique_ptr<uint16_t[]> doCrop<uint16_t[]>(std::unique_ptr<uint16_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);


    template<typename T>
    std::vector<T> doCrop(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);

    extern template std::vector<uint8_t> doCrop<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);
    extern template std::vector<uint16_t> doCrop<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);

    template<typename T>
    T bilinn(const std::vector<T> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n);

    extern template uint8_t bilinn<uint8_t>(const std::vector<uint8_t> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n);
    extern template uint16_t bilinn<uint16_t>(const std::vector<uint16_t> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n);

    template<typename T>
    std::vector<T> doReduce(std::vector<T> &&inbuf, uint32_t reduce, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny);

    extern template std::vector<uint8_t> doReduce<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t reduce, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny);
    extern template std::vector<uint16_t> doReduce<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t reduce, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny);

    template<typename T>
    std::vector<T> doScaleFast(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    extern template std::vector<uint8_t> doScaleFast<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    extern template std::vector<uint16_t> doScaleFast<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template<typename T>
    std::vector<T> doScaleMedium(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    extern template std::vector<uint8_t> doScaleMedium<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    extern template std::vector<uint16_t> doScaleMedium<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template<typename T>
    std::vector<T> doScale(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    extern template std::vector<uint8_t> doScale<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    extern template std::vector<uint16_t> doScale<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template<typename T>
    std::vector<T> doRotate(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror);

    extern template std::vector<uint8_t> doRotate<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror);
    extern template std::vector<uint16_t> doRotate<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror);

}

#endif //CSERVER_IIIFIMGTOOLS_H
