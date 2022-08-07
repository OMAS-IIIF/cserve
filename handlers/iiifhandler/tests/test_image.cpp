//
// Created by Lukas Rosenthaler on 04.08.22.
//
#include <filesystem>
#include <iostream>
#include <fstream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOTiff.h"

TEST_CASE("Image tests", "TIFF") {
    cserve::IIIFIOTiff tiffio;
    cserve::IIIFIOTiff::initLibrary();

    SECTION("TIFF RGB 8-Bit uncompressed") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_uncompressed.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);

        cserve::IIIFImgInfo info = tiffio.getDim("data/tiff_01_rgb_uncompressed.tif", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 1200);
        REQUIRE(info.height == 900);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_uncompressed.tif", compression));

        compression[cserve::TIFF_COMPRESSION] = "COMPRESSION_LZW";
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_lzw.tif", compression));


        auto region1 = std::make_shared<cserve::IIIFRegion>("100,100,300,300");
        auto size1 = std::make_shared<cserve::IIIFSize>("100,100");
        cserve::IIIFImage img1 = tiffio.read("data/tiff_01_rgb_uncompressed.tif",
                                             0,
                                             region1,
                                             size1,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img1.getNx() == 100);
        REQUIRE(img1.getNy() == 100);
        REQUIRE(img1.getNc() == 3);
        REQUIRE(img1.getBps() == 8);
        REQUIRE(img1.getPhoto() == cserve::RGB);

        auto region2 = std::make_shared<cserve::IIIFRegion>("100,100,300,300");
        auto size2 = std::make_shared<cserve::IIIFSize>("^400,400");
        cserve::IIIFImage img2 = tiffio.read("data/tiff_01_rgb_uncompressed.tif",
                                             0,
                                             region2,
                                             size2,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img2.getNx() == 400);
        REQUIRE(img2.getNy() == 400);
        REQUIRE(img2.getNc() == 3);
        REQUIRE(img2.getBps() == 8);
        REQUIRE(img2.getPhoto() == cserve::RGB);
    }


    SECTION("TIFF RGBA") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_rgba.tif",
                                             0,
                                             region,
                                             size,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 800);
        REQUIRE(img.getNy() == 600);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_rgba.tif", compression));
    }

    SECTION("TIFF RGB 8-Bit lzw") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_lzw.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);
    }

    SECTION("TIFF RGB 8-Bit lzw with region") {
        auto region = std::make_shared<cserve::IIIFRegion>("100,150,400,300");
        auto size = std::make_shared<cserve::IIIFSize>("!250,250");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_lzw.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 250);
        REQUIRE(img.getNy() == 188);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);
    }

    SECTION("TIFF RGB 8-Bit jpg") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_jpg.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::YCBCR);
    }

    SECTION("TIFF RGB 8-Bit rrrgggbbb") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_rrrgggbbb.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);
    }

    SECTION("TIFF RGB 16-Bit uncompressed") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_02_rgb_uncompressed_16bps.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 800);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 16);
        REQUIRE(img.getPhoto() == cserve::RGB);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_02_rgb_uncompressed_16bps.tif", compression));

    }

    SECTION("TIFF RGB 16-Bit to 8 bit") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_02_rgb_uncompressed_16bps.tif",
                                            0,
                                            region,
                                            size,
                                            true,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 800);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);
    }

    SECTION("TIFF RGB palette") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/palette.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 2682);
        REQUIRE(img.getNy() == 1950);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);
    }

    SECTION("TIFF RGB CCITT4") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/smalltiff_group4.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 6);
        REQUIRE(img.getNy() == 4);
        REQUIRE(img.getNc() == 1);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::MINISBLACK);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/smalltiff_group4.tif", compression));
    }

    SECTION("TIFF RGB CMYK") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/cmyk.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 4872);
        REQUIRE(img.getNy() == 3864);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::SEPARATED);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/cmyk.tif", compression));
    }

    SECTION("TIFF RGB CIELab") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/cielab.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1280);
        REQUIRE(img.getNy() == 960);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::CIELAB);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/cielab.tif", compression));
    }

    SECTION("TIFF RGB CIELab16") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/CIELab16.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 2000);
        REQUIRE(img.getNy() == 2709);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 16);
        REQUIRE(img.getPhoto() == cserve::CIELAB);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/CIELab16.tif", compression));
    }

}
