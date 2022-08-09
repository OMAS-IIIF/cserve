//
// Created by Lukas Rosenthaler on 08.08.22.
//
#include <filesystem>
#include <iostream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOPng.h"

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

void deleteDirectoryContents(const std::string &dir_path) {
    for (const auto &entry: std::filesystem::directory_iterator(dir_path))
        std::filesystem::remove_all(entry.path());
}

TEST_CASE("Image tests", "PNG") {
    cserve::IIIFIOPng pngio;

    SECTION("8Bit") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = pngio.read("data/png_rgb8.png",
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

        auto info = pngio.getDim("data/png_rgb8.png", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 1200);
        REQUIRE(info.height == 900);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(pngio.write(img, "scratch/png_rgb8.png", compression));
        auto res = Command::exec("compare -quiet -metric mae data/png_rgb8.png scratch/png_rgb8.png scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/png_rgb8.png");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("8BitAlpha") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = pngio.read("data/png_rgb8_alpha.png",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);

        auto info = pngio.getDim("data/png_rgb8_alpha.png", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 1200);
        REQUIRE(info.height == 900);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(pngio.write(img, "scratch/png_rgb8_alpha.png", compression));
        auto res = Command::exec("compare -quiet -metric mae data/png_rgb8_alpha.png scratch/png_rgb8_alpha.png scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/png_rgb8_alpha.png");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("16Bit") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = pngio.read("data/png_rgb16.png",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 600);
        REQUIRE(img.getNy() == 600);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 16);
        REQUIRE(img.getPhoto() == cserve::RGB);

        auto info = pngio.getDim("data/png_rgb16.png", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 600);
        REQUIRE(info.height == 600);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(pngio.write(img, "scratch/png_rgb16.png", compression));
        auto res = Command::exec("compare -quiet -metric mae data/png_rgb16.png scratch/png_rgb16.png scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/png_rgb16.png");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("16BitAlpha") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = pngio.read("data/png_rgb16_alpha.png",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 600);
        REQUIRE(img.getNy() == 600);
        REQUIRE(img.getNc() == 4);
        REQUIRE(img.getBps() == 16);
        REQUIRE(img.getPhoto() == cserve::RGB);

        auto info = pngio.getDim("data/png_rgb16_alpha.png", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 600);
        REQUIRE(info.height == 600);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(pngio.write(img, "scratch/png_rgb16_alpha.png", compression));
        auto res = Command::exec("compare -quiet -metric mae data/png_rgb16_alpha.png scratch/png_rgb16_alpha.png scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/png_rgb16_alpha.png");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("8BitPalette") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = pngio.read("data/png_palette8.png",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 150);
        REQUIRE(img.getNy() == 200);
        REQUIRE(img.getNc() == 1);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::MINISBLACK);

        auto info = pngio.getDim("data/png_palette8.png", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 150);
        REQUIRE(info.height == 200);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(pngio.write(img, "scratch/png_palette8.png", compression));
        auto res = Command::exec("compare -quiet -metric mae data/png_palette8.png scratch/png_palette8.png scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0 (0)", 0});
        std::filesystem::remove("scratch/png_palette8.png");
        std::filesystem::remove("scratch/out.png");
    }

}