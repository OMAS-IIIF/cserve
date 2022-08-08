//
// Created by Lukas Rosenthaler on 04.08.22.
//
#include <filesystem>
#include <iostream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOTiff.h"

struct CommandResult {
    std::string output;
    int exitstatus;

    friend std::ostream &operator<<(std::ostream &os, const CommandResult &result) {
        os << "command exitstatus: " << result.exitstatus << " output: " << result.output;
        return os;
    }
    bool operator==(const CommandResult &rhs) const {
        return output == rhs.output &&
               exitstatus == rhs.exitstatus;
    }
    bool operator!=(const CommandResult &rhs) const {
        return !(rhs == *this);
    }
};

class Command {
public:
    static CommandResult exec(const std::string &command) {
        int exitcode = 255;
        std::array<char, 1048576> buffer{};
        std::string result;
        FILE *pipe = popen(command.c_str(), "r");
        if (pipe == nullptr) {
            throw std::runtime_error("popen() failed!");
        }
        try {
            std::size_t bytesread;
            while ((bytesread = fread(buffer.data(), sizeof(buffer.at(0)), sizeof(buffer), pipe)) != 0) {
                result += std::string(buffer.data(), bytesread);
            }
        } catch (...) {
            pclose(pipe);
            throw;
        }
        exitcode = pclose(pipe);
        return CommandResult{result, exitcode};
    }
};

void deleteDirectoryContents(const std::string& dir_path) {
    for (const auto& entry : std::filesystem::directory_iterator(dir_path))
        std::filesystem::remove_all(entry.path());
}

TEST_CASE("Image tests", "TIFF") {
    cserve::IIIFIOTiff tiffio;
    cserve::IIIFIOTiff::initLibrary();

    SECTION("RGB-8Bit-uncompressed") {
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
        auto res = Command::exec("compare -quiet -metric mae data/tiff_01_rgb_uncompressed.tif scratch/tiff_01_rgb_uncompressed.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_uncompressed.tif");
        std::filesystem::remove("scratch/out.png");

        compression[cserve::TIFF_COMPRESSION] = "COMPRESSION_LZW";
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_lzw.tif", compression));
        auto res2 = Command::exec("compare -quiet -metric mae data/tiff_01_rgb_uncompressed.tif scratch/tiff_01_rgb_lzw.tif scratch/out.png 2>&1");
        REQUIRE(res2 == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_uncompressed.tif");
        std::filesystem::remove("scratch/out.png");


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

    SECTION("EssentialMetadata") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_essential.tif",
                                            0,
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        std::cerr << img.essential_metadata() << std::endl;
        REQUIRE(img.essential_metadata().is_set());
        REQUIRE(img.essential_metadata().origname() == "tiff_01_rgb_uncompressed.tif");
        REQUIRE(img.essential_metadata().mimetype() == "image/tiff");
        REQUIRE(img.essential_metadata().hash_type() == cserve::sha256);
        REQUIRE(img.essential_metadata().data_chksum() == "011c7a3835c50c56cb45ee80ba3097e171a94e71f57362d2a252d8e5bb8e5aa3");
        REQUIRE_FALSE(img.essential_metadata().use_icc());
    }

    SECTION("RGBA") {
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
        auto res = Command::exec("compare -quiet -metric mae data/tiff_rgba.tif scratch/tiff_rgba.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/tiff_rgba.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("RGB-8Bit-lzw") {
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

    SECTION("RGB-8Bit-lzw-region") {
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

    SECTION("RGB-8Bit-jpg") {
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

    SECTION("RGB-8Bit-rrrgggbbb") {
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

    SECTION("RGB-16Bit-uncompressed") {
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
        auto res = Command::exec("compare -quiet -metric mae data/tiff_02_rgb_uncompressed_16bps.tif scratch/tiff_02_rgb_uncompressed_16bps.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/tiff_02_rgb_uncompressed_16bps.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("RGB-16Bit to 8 bit") {
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
        auto res = Command::exec("compare -quiet -metric mae data/smalltiff_group4.tif scratch/smalltiff_group4.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/smalltiff_group4.tif");
        std::filesystem::remove("scratch/out.png");
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
        auto res = Command::exec("compare -quiet -metric mae data/cmyk.tif scratch/cmyk.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/cmyk.tif");
        std::filesystem::remove("scratch/out.png");
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
        std::filesystem::remove("scratch/cielab.tif");
        std::filesystem::remove("scratch/out.png");
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
        std::filesystem::remove("scratch/CIELab16.tif");
        std::filesystem::remove("scratch/out.png");
    }
}

