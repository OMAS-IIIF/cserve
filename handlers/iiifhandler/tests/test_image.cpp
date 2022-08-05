//
// Created by Lukas Rosenthaler on 04.08.22.
//
#include <filesystem>
#include <iostream>
#include <fstream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOTiff.h"

TEST_CASE("Image tests", "image") {
    SECTION("TIFF RGB 8-Bit uncompressed") {
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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

    SECTION("TIFF RGB 8-Bit lzw") {
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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

    SECTION("TIFF RGB 8-Bit jpg") {
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
    }

    SECTION("TIFF RGB 16-Bit to 8 bit") {
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
        cserve::IIIFIOTiff tiffio;
        cserve::IIIFIOTiff::initLibrary();
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
    }

}