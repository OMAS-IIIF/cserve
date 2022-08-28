//
// Created by Lukas Rosenthaler on 28.08.22.
//
#include <filesystem>
#include <iostream>

#include "catch2/catch_all.hpp"
#include "../IIIFImage.h"
#include "../imgformats/IIIFIOJ2k.h"

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

TEST_CASE("Image tests", "J2K") {
    cserve::IIIFIOJ2k j2kio;
    SECTION("tmp") {
        auto  region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");

        cserve::IIIFImage im = cserve::IIIFImage::readOriginal("data/IMG_8207.tiff",
                                                               0,
                                                               region,
                                                               size,
                                                               "IMG_8207.tiff",
                                                               cserve::HashType::sha256);
        cserve::IIIFCompressionParams params;
        j2kio.write(im, "data/IMG_8207.jpx", params);

    }

    SECTION("metadata") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = j2kio.read("data/IMG_8207.jpx",
                                           0,
                                           region,
                                           size,
                                           false,
                                           {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        auto exif = img.getExif();
        REQUIRE(exif != nullptr);
        std::string artist;
        REQUIRE(exif->getValByKey("Exif.Image.Artist", artist));
        REQUIRE(artist == "Lukas Rosenthaler");

        auto iptc = img.getIptc();
        REQUIRE(iptc != nullptr);
        auto val = iptc->getValByKey("Iptc.Application2.Caption");
        REQUIRE(val.toString() == "Storage tracks at Bastia station");

        auto icc = img.getIcc();
        REQUIRE(icc != nullptr);
        auto profile = icc->getIccProfile();
        cmsUInt32Number psiz = cmsGetProfileInfo(profile, cmsInfoDescription, "en", "US", nullptr, 0);
        auto pbufw = std::make_unique<wchar_t[]>(psiz);
        cmsGetProfileInfo(profile, cmsInfoDescription, "en", "US", pbufw.get(), psiz);
        std::wstring ws(pbufw.get());
        std::string pbuf(ws.begin(), ws.end());
        REQUIRE(pbuf == "sRGB built-in");

        auto essential = img.essential_metadata();
        REQUIRE(essential.origname() == "IMG_8207.tiff");
        REQUIRE(essential.mimetype() == "image/tiff");
        REQUIRE(essential.hash_type() == cserve::HashType::sha256);
        REQUIRE(essential.data_chksum() == "bc8eb26df171005e7019a449c0442964be26b74561d506322db55bf151ec673e");
    }
}