//
// Created by Lukas Rosenthaler on 10.08.22.
//
#include <filesystem>
#include <iostream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOJpeg.h"

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

TEST_CASE("Image tests", "JPEG") {
    cserve::IIIFIOJpeg jpegio;


    SECTION("8BitEs") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = jpegio.read("data/jpeg_with_icc_es.jpg",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 300);
        REQUIRE(img.getNy() == 200);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);

        REQUIRE(img.essential_metadata().is_set());
        REQUIRE(img.essential_metadata().origname() == "image_with_icc_profile.tif");
        REQUIRE(img.essential_metadata().mimetype() == "image/tiff");
        REQUIRE(img.essential_metadata().hash_type() == cserve::sha256);
        REQUIRE(img.essential_metadata().data_chksum() == "aa9bfcb3a8686a9a700f5cc6b16035844935c536ae91f14ef47061bfe18e6862");
        REQUIRE_FALSE(img.essential_metadata().use_icc());

        auto info = jpegio.getDim("data/jpeg_with_icc_es.jpg", 0);
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 300);
        REQUIRE(info.height == 200);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(jpegio.write(img, "scratch/jpeg_with_icc_es.jpg", compression));
        auto res = Command::exec("compare -quiet -metric mae data/jpeg_with_icc_es.jpg scratch/jpeg_with_icc_es.jpg scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"145.995 (0.00222773)", 256});
        std::filesystem::remove("scratch/jpeg_with_icc_es.jpg");
        std::filesystem::remove("scratch/out.png");
    }
}
