//
// Created by Lukas Rosenthaler on 22.07.22.
//

#include "IIIFImgTools.h"
#include "fmt/format.h"
#include "IIIFPhotometricInterpretation.h"

static const char file_[] = __FILE__;

namespace cserve {

    std::unique_ptr<uint8_t[]> separateToContig(std::unique_ptr<uint8_t[]> inbuf, size_t nx, size_t ny, size_t nc, size_t sll) {
        auto tmpptr = std::make_unique<uint8_t[]>(nc * ny * nx);
        for (size_t k = 0; k < nc; k++) {
            for (size_t j = 0; j < ny; j++) {
                for (size_t i = 0; i < nx; i++) {
                    tmpptr[nc * (j * nx + i) + k] = inbuf[k * ny * sll + j * nx + i];
                }
            }
        }
        return std::move(tmpptr);
    }

    std::unique_ptr<uint16_t[]> separateToContig(std::unique_ptr<uint16_t[]> inbuf, size_t nx, size_t ny, size_t nc, size_t sll) {
        auto tmpptr = std::make_unique<uint16_t[]>(nc * ny * nx);
        for (unsigned int k = 0; k < nc; k++) {
            for (unsigned int j = 0; j < ny; j++) {
                for (unsigned int i = 0; i < nx; i++) {
                    tmpptr[nc * (j * nx + i) + k] = inbuf[k * ny * sll + j * nx + i];
                }
            }
        }
        return std::move(tmpptr);
    }

    IIIFImage separateToContig(IIIFImage img, unsigned int sll) {
        //
        // rearrange RRRRRR...GGGGG...BBBBB data  to RGBRGBRGB…RGB
        //
        if (img.bps == 8) {
            auto tmpptr = std::make_unique<uint8_t[]>(img.nc * img.ny * img.nx);
            for (unsigned int k = 0; k < img.nc; k++) {
                for (unsigned int j = 0; j < img.ny; j++) {
                    for (unsigned int i = 0; i < img.nx; i++) {
                        tmpptr[img.nc * (j * img.nx + i) + k] = img.bpixels[k * img.ny * sll + j * img.nx + i];
                    }
                }
            }
            img.bpixels = std::move(tmpptr);
        } else if (img.bps == 16) {
            auto tmpptr = std::make_unique<uint16_t[]>(img.nc * img.ny * img.nx);

            for (unsigned int k = 0; k < img.nc; k++) {
                for (unsigned int j = 0; j < img.ny; j++) {
                    for (unsigned int i = 0; i < img.nx; i++) {
                        tmpptr[img.nc * (j * img.nx + i) + k] = img.wpixels[k * img.ny * sll + j * img.nx + i];
                    }
                }
            }
            img.wpixels = std::move(tmpptr);
        } else {
            throw IIIFImageError(file_, __LINE__, fmt::format("Bits per sample not supported: (bps={})", img.bps));
        }
        return img;
    }

    std::unique_ptr<byte[]> crop(std::unique_ptr<byte[]> inbuf, size_t nx, size_t ny, size_t nc, const std::shared_ptr<IIIFRegion> &region) {
        if (region->getType() == IIIFRegion::FULL) {
            return std::move(inbuf);
        }
        int x, y;
        size_t width, height;
        region->crop_coords(nx, ny, x, y, width, height);
        auto outbuf = std::make_unique<byte[]>(width * height * nc);

        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
                for (size_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * width + i) + k] = inbuf[nc * ((j + y) * nx + (i + x)) + k];
                }
            }
        }
        return std::move(outbuf);
    }

    std::unique_ptr<word[]> crop(std::unique_ptr<word[]> inbuf, size_t nx, size_t ny, size_t nc, const std::shared_ptr<IIIFRegion> &region) {
        if (region->getType() == IIIFRegion::FULL) {
            return std::move(inbuf);
        }
        int x, y;
        size_t width, height;
        region->crop_coords(nx, ny, x, y, width, height);
        auto outbuf = std::make_unique<word[]>(width * height * nc);

        for (size_t j = 0; j < height; j++) {
            for (size_t i = 0; i < width; i++) {
                for (size_t k = 0; k < nc; k++) {
                    outbuf[nc * (j * width + i) + k] = inbuf[nc * ((j + y) * nx + (i + x)) + k];
                }
            }
        }
        return std::move(outbuf);
    }



    std::unique_ptr<byte[]> cvrt1BitTo8Bit(const IIIFImage &img, unsigned int sll, unsigned int black, unsigned int white) {
        byte *raw_inbuf = img.bpixels.get();
        byte *in_byte, *out_byte, *in_off, *out_off, *inbuf_high;

        static unsigned char mask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
        unsigned int x, y, k;

        if ((img.photo != PhotometricInterpretation::MINISWHITE) &&
            (img.photo != PhotometricInterpretation::MINISBLACK)) {
            throw IIIFImageError(file_, __LINE__,
                                 "Photometric interpretation is not MINISWHITE or  MINISBLACK");
        }

        if (img.bps != 1) {
            std::string msg = "Bits per sample is not 1 but: " + std::to_string(img.bps);
            throw IIIFImageError(file_, __LINE__, msg);
        }

        auto outbuf = std::make_unique<byte[]>(img.nx * img.ny);
        byte *raw_outbuf = outbuf.get();
        inbuf_high = raw_inbuf + img.ny * sll;

        if ((8 * sll) == img.nx) {
            in_byte = raw_inbuf;
            out_byte = raw_outbuf;

            for (; in_byte < inbuf_high; in_byte++, out_byte += 8) {
                for (k = 0; k < 8; k++) {
                    *(out_byte + k) = (*(mask + k) & *in_byte) ? white : black;
                }
            }
        } else {
            out_off = raw_outbuf;
            in_off = raw_inbuf;

            for (y = 0; y < img.ny; y++, out_off += img.nx, in_off += sll) {
                x = 0;
                for (in_byte = in_off; in_byte < in_off + sll; in_byte++, x += 8) {
                    out_byte = out_off + x;

                    if ((x + 8) <= img.nx) {
                        for (k = 0; k < 8; k++) {
                            *(out_byte + k) = (*(mask + k) & *in_byte) ? white : black;
                        }
                    } else {
                        for (k = 0; (x + k) < img.nx; k++) {
                            *(out_byte + k) = (*(mask + k) & *in_byte) ? white : black;
                        }
                    }
                }
            }
        }

        return std::move(outbuf);
    }

    std::unique_ptr<byte[]> cvrt8BitTo1bit(const IIIFImage &img, unsigned int &sll) {
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
        auto outbuf = std::make_unique<uint8_t[]>(sll * img.ny);
        byte *raw_outbuf = outbuf.get();
        //unsigned char *outbuf = new unsigned char[sll * img.ny];

        if (img.photo == PhotometricInterpretation::MINISBLACK) {
            for (y = 0; y < img.ny; y++) {
                for (x = 0; x < img.nx; x++) {
                    raw_outbuf[y * sll + (x / 8)] |= (img.bpixels[y * img.nx + x] > 128) ? mask[x % 8] : !mask[x % 8];
                }
            }
        } else { // must be MINISWHITE
            for (y = 0; y < img.ny; y++) {
                for (x = 0; x < img.nx; x++) {
                    raw_outbuf[y * sll + (x / 8)] |= (img.bpixels[y * img.nx + x] > 128) ? !mask[x % 8] : mask[x % 8];
                }
            }
        }
        return std::move(outbuf);
    }

}