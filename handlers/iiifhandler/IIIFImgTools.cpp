//
// Created by Lukas Rosenthaler on 22.07.22.
//

#include "IIIFImgTools.h"
#include "fmt/format.h"
#include "IIIFPhotometricInterpretation.h"

static const char file_[] = __FILE__;

namespace cserve {




    template<typename T>
    void separate2contig(T *buf, uint32_t nx, uint32_t ny, uint32_t nc) {
        auto tmp = std::make_unique<T[]>(nx*nc);
    }

    std::unique_ptr<uint8_t[]> cvrt1BitTo8Bit(std::unique_ptr<uint8_t[]> &&inbuf,
                                              uint32_t nx, uint32_t ny, uint32_t sll,
                                              uint8_t black, uint8_t white) {
        static uint8_t mask[8] = {0b10000000,
                                  0b01000000,
                                  0b00100000,
                                  0b00010000,
                                  0b00001000,
                                  0b00000100,
                                  0b00000010,
                                  0b00000001};


        auto outbuf = std::make_unique<uint8_t[]>(nx*ny);

        if ((8 * sll) == nx) {
            for (uint32_t i = 0; i < ny*sll; ++i) {
                for (uint32_t k = 0; k < 8; ++k) {
                    outbuf[i*8 + k] = mask[k] & inbuf[i] ? white : black;
                }
            }
        } else {
            for (uint32_t y = 0; y < ny; ++y) {
                for (uint32_t x = 0; x < nx; x += 8) {
                    for (uint32_t k = 0; k < 8; ++k) {
                        if ((x + k) < nx) {
                            outbuf[y*nx + x + k] = mask[k] & inbuf[y*sll + x] ? white : black;
                        }
                    }
                }
            }
        }
        return outbuf;
    }

    std::unique_ptr<uint8_t[]> cvrt4BitTo8Bit(std::unique_ptr<uint8_t[]> &&inbuf,
                                              uint32_t nx, uint32_t ny, uint32_t sll) {
        auto outbuf = std::make_unique<uint8_t[]>(nx*ny);
        static uint8_t mask[8] = { 0b11110000, 0b00001111 };
        for (uint32_t y = 0; y < ny; ++y) {
            for (uint32_t x = 0; x < nx; x += 2) {
                for (uint32_t k = 0; k < 2; ++k) {
                    outbuf[y*nx + x] = (mask[0] && inbuf[y*sll + x]) >> 4;
                    if ((x + 1) < nx) {
                        outbuf[y * nx + x] = (mask[1] && inbuf[y * sll + x]);
                    }
                }
            }
        }
        return outbuf;
    }

    template<typename T>
    std::unique_ptr<T> separateToContig(std::unique_ptr<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll) {
        auto tmpptr = std::make_unique<T>(nc * ny * nx);
        for (uint32_t c = 0; c < nc; ++c) {
            for (uint32_t y = 0; y < ny; ++y) {
                for (uint32_t x = 0; x < nx; ++x) {
                    tmpptr[nc * (y * nx + x) + c] = inbuf.get()[c * ny * sll + y * nx + x];
                }
            }
        }
        return tmpptr;
    }


    template<typename T>
    std::vector<T> separateToContig(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll) {
        auto tmpptr = std::vector<T>(nc * ny * nx);
        for (uint32_t c = 0; c < nc; ++c) {
            for (uint32_t y = 0; y < ny; ++y) {
                for (uint32_t x = 0; x < nx; ++x) {
                    tmpptr[nc * (y * nx + x) + c] = inbuf[c * ny * sll + y * nx + x];
                }
            }
        }
        return tmpptr;
    }

    /*
    template<typename T>
    std::unique_ptr<T[]> separateToContig(std::unique_ptr<T[]> inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll) {
        auto tmpptr = std::make_unique<T>(nc * ny * nx);
        for (uint32_t c = 0; c < nc; ++c) {
            for (uint32_t y = 0; y < ny; ++y) {
                for (uint32_t x = 0; x < nx; ++x) {
                    tmpptr[nc * (y * nx + x) + c] = inbuf[c * ny * sll + y * nx + x];
                }
            }
        }
        return tmpptr;
    }
*/
    IIIFImage separateToContig(IIIFImage img, uint32_t sll) {
        if (img.bps == 8) {
            img.bpixels = separateToContig<uint8_t>(std::move(img.bpixels), img.nx, img.ny, img.nc, sll);
        }
        else if (img.bps == 16){
            img.wpixels = separateToContig<uint16_t>(std::move(img.wpixels), img.nx, img.ny, img.nc, sll);
        }
        else {
            throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample not supported: (bps={})", img.bps));
        }
        return img;
    }

    template<typename T>
    std::unique_ptr<T> doCrop(std::unique_ptr<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc,
                              const std::shared_ptr<IIIFRegion> &region) {
        if (region->getType() == IIIFRegion::FULL) {
            return std::move(inbuf);
        }
        int32_t x, y;
        uint32_t width, height;
        region->crop_coords(nx, ny, x, y, width, height);
        auto outbuf = std::make_unique<T>(width * height * nc);

        for (uint32_t j = 0; j < height; j++) {
            for (uint32_t i = 0; i < width; i++) {
                for (uint32_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * width + i) + k] = inbuf[nc * ((j + y) * nx + (i + x)) + k];
                }
            }
        }
        return outbuf;
    }


    template<typename T>
    std::vector<T> doCrop(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region) {
        if (region->getType() == IIIFRegion::FULL) {
            return std::move(inbuf);
        }
        int32_t x, y;
        uint32_t width, height;
        region->crop_coords(nx, ny, x, y, width, height);
        auto outbuf = std::vector<T>(width * height * nc);

        for (uint32_t j = 0; j < height; j++) {
            for (uint32_t i = 0; i < width; i++) {
                for (uint32_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * width + i) + k] = inbuf[nc * ((j + y) * nx + (i + x)) + k];
                }
            }
        }
        return outbuf;
    }

    std::vector<uint8_t> cvrt1BitTo8Bit(const IIIFImage &img, uint32_t sll, uint8_t black, uint8_t white) {
        static unsigned char mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
        //uint32_t x, y, k;

        if ((img.photo != PhotometricInterpretation::MINISWHITE) &&
            (img.photo != PhotometricInterpretation::MINISBLACK)) {
            throw IIIFImageError(file_, __LINE__,
                                 "Photometric interpretation is not MINISWHITE or  MINISBLACK");
        }

        if (img.bps != 1) {
            std::string msg = "Bits per sample is not 1 but: " + std::to_string(img.bps);
            throw IIIFImageError(file_, __LINE__, msg);
        }

        auto outbuf = std::vector<uint8_t>(img.nx * img.ny);

        if ((8 * sll) == img.nx) {
            for (uint32_t i = 0; i < img.ny*sll; ++i) {
                for (uint32_t k = 0; k < 8; ++k) {
                    outbuf[i*8 + k] = mask[k] & img.bpixels[i] ? white : black;
                }
            }
        } else {
            for (uint32_t y = 0; y < img.ny; ++y) {
                for (uint32_t x = 0; x < img.nx; x += 8) {
                    for (uint32_t k = 0; k < 8; ++k) {
                        if ((x + k) < img.nx) {
                            outbuf[y*img.nx + x + k] = mask[k] & img.bpixels[y*sll + x] ? white : black;
                        }
                    }
                }

            }
        }
        return outbuf;
    }

    std::vector<uint8_t> cvrt8BitTo1bit(const IIIFImage &img, uint32_t &sll) {
        static unsigned char mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};

        unsigned int x, y;

        if ((img.photo != PhotometricInterpretation::MINISWHITE) &&
            (img.photo != PhotometricInterpretation::MINISBLACK)) {
            throw IIIFImageError(file_, __LINE__,
                                 "Photometric interpretation is not MINISWHITE or  MINISBLACK");
        }

        if (img.bps != 8) {
            throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample is not 8 (bps={})", img.bps));
        }

        sll = (img.nx + 7) / 8;
        auto outbuf = std::vector<uint8_t>(sll * img.ny);
        if (img.photo == PhotometricInterpretation::MINISBLACK) {
            for (y = 0; y < img.ny; y++) {
                for (x = 0; x < img.nx; x++) {
                    outbuf[y * sll + (x / 8)] |= (img.bpixels[y * img.nx + x] > 128) ? mask[x % 8] : !mask[x % 8];
                }
            }
        } else { // must be MINISWHITE
            for (y = 0; y < img.ny; y++) {
                for (x = 0; x < img.nx; x++) {
                    outbuf[y * sll + (x / 8)] |= (img.bpixels[y * img.nx + x] > 128) ? !mask[x % 8] : mask[x % 8];
                }
            }
        }

        return outbuf;
    }

    template<typename T>
    std::vector<T> doReduce(std::vector<T> &&inbuf, uint32_t reduce,
                            uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny) {
        IIIFSize size(reduce);
        uint32_t r = 0;
        bool redonly;
        size.get_size(nx, ny, nnx, nny, r, redonly);
        auto outbuf = std::vector<T>(nnx * nny * nc);

        auto inbuf_raw = inbuf.data();
        auto outbuf_raw = outbuf.data();
        if (reduce <= 1) {
            memcpy(outbuf_raw, inbuf_raw, nnx * nny * nc * sizeof(T));
        }
        else {
            for (uint32_t y = 0; y < nny; ++y) {
                for (uint32_t x = 0; x < nnx; ++x) {
                    for (uint32_t c = 0; c < nc; ++c) {
                        uint32_t cnt = 0;
                        uint32_t tmp = 0;
                        for (uint32_t xx = 0; xx < reduce; ++xx) {
                            for (uint32_t yy = 0; yy < reduce; ++yy) {
                                if (((reduce*x + xx) < nx) && ((reduce*y + yy) < ny)) {
                                    tmp += static_cast<uint32_t>(inbuf[nc*((reduce*y + yy)*nx + (reduce*x + xx)) + c]);
                                    ++cnt;
                                }
                            }
                        }
                        outbuf[nc*(y*nnx + x) + c] = tmp / cnt;
                    }
                }
            }
        }
        return outbuf;
    }


#define POSITION(x, y, c, n) ((n)*((y)*nx + (x)) + c)
    template<typename T>
    T bilinn(const std::vector<T> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n) {
        int ix, iy;
        double rx, ry;
        ix = (int) x;
        iy = (int) y;
        rx = x - (double) ix;
        ry = y - (double) iy;

        if ((rx < 1.0e-2) && (ry < 1.0e-2)) {
            return (buf[POSITION(ix, iy, c, n)]);
        } else if (rx < 1.0e-2) {
            return ((byte) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry))));
        } else if (ry < 1.0e-2) {
            return ((T) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry))));
        } else {
            return ((T) lround(((double) buf[POSITION(ix, iy, c, n)] * (1 - rx - ry + rx * ry) +
                                   (double) buf[POSITION((ix + 1), iy, c, n)] * (rx - rx * ry) +
                                   (double) buf[POSITION(ix, (iy + 1), c, n)] * (ry - rx * ry) +
                                   (double) buf[POSITION((ix + 1), (iy + 1), c, n)] * rx * ry)));
        }
    }
    /*==========================================================================*/

#undef POSITION

    template<typename T>
    std::vector<T> doScaleFast(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny) {
        auto xlut = std::make_unique<uint32_t[]>(nnx);
        auto ylut = std::make_unique<uint32_t[]>(nny);

        for (uint32_t i = 0; i < nnx; i++) {
            xlut[i] = (uint32_t) lround(i * (nx - 1) / (double) (nnx - 1));
        }
        for (uint32_t i = 0; i < nny; i++) {
            ylut[i] = (uint32_t) lround(i * (ny - 1) / (double) (nny - 1 ));
        }
        auto outbuf = std::vector<T>(nnx * nny * nc);
        for (uint32_t y = 0; y < nny; y++) {
            for (uint32_t x = 0; x < nnx; x++) {
                for (uint32_t k = 0; k < nc; k++) {
                    outbuf[nc * (y * nnx + x) + k] = inbuf[nc * (ylut[y] * nx + xlut[x]) + k];
                }
            }
        }
        return outbuf;
    }

    template<typename T>
    std::vector<T> doScaleMedium(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny) {
        auto xlut = std::make_unique<double[]>(nnx);
        auto ylut = std::make_unique<double[]>(nny);

        for (uint32_t i = 0; i < nnx; i++) {
            xlut[i] = (double) (i * (nx - 1)) / (double) (nnx - 1);
        }
        for (uint32_t j = 0; j < nny; j++) {
            ylut[j] = (double) (j * (ny - 1)) / (double) (nny - 1);
        }
        auto outbuf = std::vector<T>(nnx * nny * nc);
        double rx, ry;
        for (uint32_t j = 0; j < nny; j++) {
            ry = ylut[j];
            for (uint32_t i = 0; i < nnx; i++) {
                rx = xlut[i];
                for (uint32_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * nnx + i) + k] = bilinn<T>(inbuf, nx, rx, ry, k, nc);
                }
            }
        }
        return outbuf;
    }
    /*==========================================================================*/

    template<typename T>
    std::vector<T> doScale(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny) {
        uint32_t iix = 1, iiy = 1;
        uint32_t nnnx, nnny;

        //
        // if the scaling is less than 1 (that is, the image gets smaller), we first
        // expand it to a integer multiple of the desired size, and then we just
        // avarage the number of pixels. This is the "proper" way of downscale an
        // image...
        //
        if (nnx < nx) {
            while (nnx * iix < nx) iix++;
            nnnx = nnx * iix;
        } else {
            nnnx = nnx;
        }

        if (nny < ny) {
            while (nny * iiy < ny) iiy++;
            nnny = nny * iiy;
        } else {
            nnny = nny;
        }

        auto xlut = std::make_unique<double[]>(nnnx);
        auto ylut = std::make_unique<double[]>(nnny);

        for (uint32_t i = 0; i < nnnx; i++) {
            xlut[i] = (double) (i * (nx - 1)) / (double) (nnnx - 1);
        }
        for (uint32_t j = 0; j < nnny; j++) {
            ylut[j] = (double) (j * (ny - 1)) / (double) (nnny - 1);
        }

        auto outbuf = std::vector<T>(nnnx * nnny * nc);
        double rx, ry;

        for (uint32_t j = 0; j < nnny; j++) {
            ry = ylut[j];
            for (uint32_t i = 0; i < nnnx; i++) {
                rx = xlut[i];
                for (uint32_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * nnnx + i) + k] = bilinn<T>(inbuf, nx, rx, ry, k, nc);
                }
            }
        }


        if ((iix > 1) || (iiy > 1)) {
            auto outbuf2 = std::vector<T>(nnx * nny * nc);
            for (uint32_t j = 0; j < nny; j++) {
                for (uint32_t i = 0; i < nnx; i++) {
                    for (uint32_t k = 0; k < nc; k++) {
                        unsigned int accu = 0;

                        for (uint32_t jj = 0; jj < iiy; jj++) {
                            for (uint32_t ii = 0; ii < iix; ii++) {
                                accu += outbuf[nc * ((iiy * j + jj) * nnnx + (iix * i + ii)) + k];
                            }
                        }

                        outbuf2[nc * (j * nnx + i) + k] = accu / (iix * iiy);
                    }
                }
            }
            outbuf = std::move(outbuf2);
        }
        return outbuf;
    }

    template<typename T>
    std::vector<T> doRotate(std::vector<T> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror) {
        if (mirror) {
            auto outbuf = std::vector<T>(nx * ny * nc);
            for (uint32_t j = 0; j < ny; j++) {
                for (uint32_t i = 0; i < nx; i++) {
                    for (uint32_t k = 0; k < nc; k++) {
                        outbuf[nc * (j * nx + i) + k] = inbuf[nc * (j * nx + (nx - i - 1)) + k];
                    }
                }
            }
            inbuf = outbuf;
        }

        while (angle < 0.) angle += 360.;
        while (angle >= 360.) angle -= 360.;

        if (angle == 0.) {
            return inbuf;
        }

        if (angle == 90.) {
            //
            // abcdef     mga
            // ghijkl ==> nhb
            // mnopqr     oic
            //            pjd
            //            qke
            //            rlf
            //
            nnx = ny;
            nny = nx;
            auto outbuf = std::vector<T>(nx * ny * nc);
            for (uint32_t j = 0; j < nny; j++) {
                for (uint32_t i = 0; i < nnx; i++) {
                    for (uint32_t k = 0; k < nc; k++) {
                        outbuf[nc * (j * nnx + i) + k] = inbuf[nc * ((ny - i - 1) * nx + j) + k];
                    }
                }
            }
            return outbuf;
        } else if (angle == 180.) {
            //
            // abcdef     rqponm
            // ghijkl ==> lkjihg
            // mnopqr     fedcba
            //
            nnx = nx;
            nny = ny;
            auto outbuf = std::vector<T>(nx * ny * nc);
            for (uint32_t j = 0; j < nny; j++) {
                for (uint32_t i = 0; i < nnx; i++) {
                    for (uint32_t k = 0; k < nc; k++) {
                        outbuf[nc * (j * nnx + i) + k] = inbuf[nc * ((ny - j - 1) * nx + (nx - i - 1)) + k];
                    }
                }
            }
            return outbuf;
        } else if (angle == 270.) {
            //
            // abcdef     flr
            // ghijkl ==> ekq
            // mnopqr     djp
            //            cio
            //            bhn
            //            agm
            //
            nnx = ny;
            nny = nx;
            auto outbuf = std::vector<T>(nx * ny * nc);
            for (uint32_t j = 0; j < nny; j++) {
                for (uint32_t i = 0; i < nnx; i++) {
                    for (uint32_t k = 0; k < nc; k++) {
                        outbuf[nc * (j * nnx + i) + k] = inbuf[nc * (i * nx + (nx - j - 1)) + k];
                    }
                }
            }
            return outbuf;
        } else { // all other angles
            double phi = M_PI * angle / 180.0;
            double ptx = static_cast<double>(nx) / 2. - .5;
            double pty = static_cast<double>(ny) / 2. - .5;

            double si = sin(-phi);
            double co = cos(-phi);

            if ((angle > 0.) && (angle < 90.)) {
                nnx = floor((double) nx * cos(phi) + (double) ny * sin(phi) + .5);
                nny = floor((double) nx * sin(phi) + (double) ny * cos(phi) + .5);
            } else if ((angle > 90.) && (angle < 180.)) {
                nnx = floor(-((double) nx) * cos(phi) + (double) ny * sin(phi) + .5);
                nny = floor((double) nx * sin(phi) - (double) ny * cosf(phi) + .5);
            } else if ((angle > 180.) && (angle < 270.)) {
                nnx = floor(-((double) nx) * cos(phi) - (double) ny * sin(phi) + .5);
                nny = floor(-((double) nx) * sinf(phi) - (double) ny * cosf(phi) + .5);
            } else {
                nnx = floor((double) nx * cos(phi) - (double) ny * sin(phi) + .5);
                nny = floor(-((double) nx) * sin(phi) + (double) ny * cos(phi) + .5);
            }

            double pptx = ptx * (double) nnx / (double) nx;
            double ppty = pty * (double) nny / (double) ny;

            auto outbuf = std::vector<T>(nnx * nny * nc);
            byte bg = 0;
            for (uint32_t j = 0; j < nny; j++) {
                for (uint32_t i = 0; i < nnx; i++) {
                    double rx = ((double) i - pptx) * co - ((double) j - ppty) * si + ptx;
                    double ry = ((double) i - pptx) * si + ((double) j - ppty) * co + pty;

                    if ((rx < 0.0) || (rx >= (double) (nx - 1)) || (ry < 0.0) || (ry >= (double) (ny - 1))) {
                        for (uint32_t k = 0; k < nc; k++) {
                            outbuf[nc * (j * nnx + i) + k] = bg;
                        }
                    } else {
                        for (uint32_t k = 0; k < nc; k++) {
                            outbuf[nc * (j * nnx + i) + k] = bilinn<T>(inbuf, nx, rx, ry, k, nc);
                        }
                    }
                }
            }
            return outbuf;
        }
    }

    template std::unique_ptr<uint8_t[]> separateToContig<uint8_t[]>(std::unique_ptr<uint8_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);
    template std::unique_ptr<uint16_t[]> separateToContig<uint16_t[]>(std::unique_ptr<uint16_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    template std::vector<uint8_t> separateToContig<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);
    template std::vector<uint16_t> separateToContig<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t sll);

    template std::unique_ptr<uint8_t[]> doCrop<uint8_t[]>(std::unique_ptr<uint8_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);
    template std::unique_ptr<uint16_t[]> doCrop<uint16_t[]>(std::unique_ptr<uint16_t[]> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);

    template std::vector<uint8_t> doCrop<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);
    template std::vector<uint16_t> doCrop<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, const std::shared_ptr<IIIFRegion> &region);

    template uint8_t bilinn<uint8_t>(const std::vector<uint8_t> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n);
    template uint16_t bilinn<uint16_t>(const std::vector<uint16_t> &buf, uint32_t nx, double x, double y, uint32_t c, uint32_t n);

    template std::vector<uint8_t> doReduce<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t reduce, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny);
    template std::vector<uint16_t> doReduce<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t reduce, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t nny);

    template std::vector<uint8_t> doScaleFast<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    template std::vector<uint16_t> doScaleFast<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template std::vector<uint8_t> doScaleMedium<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    template std::vector<uint16_t> doScaleMedium<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template std::vector<uint8_t> doScale<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);
    template std::vector<uint16_t> doScale<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t nnx, uint32_t nny);

    template std::vector<uint8_t> doRotate<uint8_t>(std::vector<uint8_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror);
    template std::vector<uint16_t> doRotate<uint16_t>(std::vector<uint16_t> &&inbuf, uint32_t nx, uint32_t ny, uint32_t nc, uint32_t &nnx, uint32_t &nny, float angle, bool mirror);

}