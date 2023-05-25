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
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img.getNx() == 1200);
        REQUIRE(img.getNy() == 900);
        REQUIRE(img.getNc() == 3);
        REQUIRE(img.getBps() == 8);
        REQUIRE(img.getPhoto() == cserve::RGB);

        cserve::IIIFImgInfo info = tiffio.getDim("data/tiff_01_rgb_uncompressed.tif");
        REQUIRE(info.success == cserve::IIIFImgInfo::DIMS);
        REQUIRE(info.width == 1200);
        REQUIRE(info.height == 900);

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_uncompressed.tif", compression));
        auto res = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_uncompressed.tif scratch/tiff_01_rgb_uncompressed.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_uncompressed.tif");
        std::filesystem::remove("scratch/out.png");

        compression[cserve::TIFF_COMPRESSION] = "COMPRESSION_LZW";
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_lzw.tif", compression));
        auto res2 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_uncompressed.tif scratch/tiff_01_rgb_lzw.tif scratch/out.png 2>&1");
        REQUIRE(res2 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_uncompressed.tif");
        std::filesystem::remove("scratch/out.png");


        auto region1 = std::make_shared<cserve::IIIFRegion>("100,100,300,300");
        auto size1 = std::make_shared<cserve::IIIFSize>("100,100");
        cserve::IIIFImage img1 = tiffio.read("data/tiff_01_rgb_uncompressed.tif",
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
                                             region2,
                                             size2,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        REQUIRE(img2.getNx() == 400);
        REQUIRE(img2.getNy() == 400);
        REQUIRE(img2.getNc() == 3);
        REQUIRE(img2.getBps() == 8);
        REQUIRE(img2.getPhoto() == cserve::RGB);

        cserve::IIIFImage img3 = tiffio.read("data/tiff_01_rgb_uncompressed.tif",
                                             region,
                                             size,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        compression[cserve::TIFF_PYRAMID] = "1:512 2:256 4:128 8:64";
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_pyramid.tif", compression));

        size_t w, h;
        cserve::IIIFImage::getDim("scratch/tiff_01_rgb_pyramid.tif");
    }

    SECTION("Metadata") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/IMG_8207.tiff",
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
        std::string pbuf( ws.begin(), ws.end());
        REQUIRE(pbuf == "sRGB IEC61966-2.1");
    }

    SECTION("EssentialMetadata") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_essential.tif",
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
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
        auto res = Command::exec("compare -quiet -metric ae data/tiff_rgba.tif scratch/tiff_rgba.tif scratch/outXXX.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_rgba.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("RGB-8Bit-lzw") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_lzw.tif",
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
        auto res = Command::exec("compare -quiet -metric ae data/tiff_02_rgb_uncompressed_16bps.tif scratch/tiff_02_rgb_uncompressed_16bps.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_02_rgb_uncompressed_16bps.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("RGB-16Bit to 8 bit") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_02_rgb_uncompressed_16bps.tif",
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

    SECTION("TIFF 4-BIT") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/money-4bit.tif",
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/money-4bit.tif", compression));
        auto res = Command::exec("compare -quiet -metric ae data/money-4bit-reference.tif scratch/money-4bit.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/money-4bit.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("TIFF 12-BIT") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_12bit_1chan.tif",
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_12bit_1chan.tif", compression));
        auto res = Command::exec("compare -quiet -metric ae data/tiff_12bit_1chan_reference.tif scratch/tiff_12bit_1chan.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/money-4bit.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("TIFF RGB CCITT4") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/smalltiff_group4.tif",
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
        auto res = Command::exec("compare -quiet -metric ae data/smalltiff_group4.tif scratch/smalltiff_group4.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/smalltiff_group4.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("TIFF RGB CMYK") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/cmyk.tif",
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
        auto res = Command::exec("compare -quiet -metric ae data/cmyk.tif scratch/cmyk.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/cmyk.tif");
        std::filesystem::remove("scratch/out.png");
    }

    SECTION("TIFF RGB CIELab") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/cielab.tif",
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

    SECTION("TIFF Pyramid with tiles") {
        auto region = std::make_shared<cserve::IIIFRegion>("full");
        auto size = std::make_shared<cserve::IIIFSize>("max");
        cserve::IIIFImage img = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                            region,
                                            size,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        cserve::IIIFCompressionParams compression;
        REQUIRE_NOTHROW(tiffio.write(img, "scratch/tiff_01_rgb_pyramid_res01.tif", compression));
        auto res = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_pyramid_res01.tif scratch/tiff_01_rgb_pyramid_res01.tif scratch/out.png 2>&1");
        REQUIRE(res == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_pyramid_res01.tif");
        std::filesystem::remove("scratch/out.png");


        auto region2 = std::make_shared<cserve::IIIFRegion>("full");
        auto size2 = std::make_shared<cserve::IIIFSize>("600,450");
        cserve::IIIFImage img2 = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                            region2,
                                            size2,
                                            false,
                                            {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        cserve::IIIFCompressionParams compression2;
        REQUIRE_NOTHROW(tiffio.write(img2, "scratch/tiff_01_rgb_pyramid_res02.tif", compression2));
        auto res2 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_pyramid_res02.tif scratch/tiff_01_rgb_pyramid_res02.tif scratch/out.png 2>&1");
        REQUIRE(res2 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_pyramid_res02.tif");
        std::filesystem::remove("scratch/out.png");

        auto region3 = std::make_shared<cserve::IIIFRegion>("350,300,600,400");
        auto size3 = std::make_shared<cserve::IIIFSize>("300,200");
        cserve::IIIFImage img3 = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                             region3,
                                             size3,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        cserve::IIIFCompressionParams compression3;
        REQUIRE_NOTHROW(tiffio.write(img3, "scratch/tiff_01_rgb_pyramid_res03.tif", compression3));
        auto res3 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_pyramid_res03.tif scratch/tiff_01_rgb_pyramid_res03.tif scratch/out.png 2>&1");
        REQUIRE(res3 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_pyramid_res03.tif");
        std::filesystem::remove("scratch/out.png");

        auto region4 = std::make_shared<cserve::IIIFRegion>("0,0,512,512");
        auto size4 = std::make_shared<cserve::IIIFSize>("256,256");
        cserve::IIIFImage img4 = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                             region4,
                                             size4,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});

        cserve::IIIFCompressionParams compression4;
        REQUIRE_NOTHROW(tiffio.write(img4, "scratch/tiff_01_rgb_pyramid_res04.tif", compression4));
        auto res4 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_pyramid_res04.tif scratch/tiff_01_rgb_pyramid_res04.tif scratch/out.png 2>&1");
        REQUIRE(res4 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_pyramid_res04.tif");
        std::filesystem::remove("scratch/out.png");

        auto region5 = std::make_shared<cserve::IIIFRegion>("1024,0,176,512");
        auto size5 = std::make_shared<cserve::IIIFSize>("88,256");
        cserve::IIIFImage img5 = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                             region5,
                                             size5,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5;
        REQUIRE_NOTHROW(tiffio.write(img5, "scratch/tiff_01_rgb_pyramid_res05.tif", compression5));
        auto res5 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_pyramid_res05.tif scratch/tiff_01_rgb_pyramid_res05.tif scratch/out.png 2>&1");
        REQUIRE(res5 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/tiff_01_rgb_pyramid_res05.tif");
        std::filesystem::remove("scratch/out.png");

        //
        // Get all tiles in a reduced size, compose the tiles to a complete image
        // and compare to reference image...
        //
        auto region5a = std::make_shared<cserve::IIIFRegion>("0,0,512,512");
        auto size5a = std::make_shared<cserve::IIIFSize>("256,256");
        cserve::IIIFImage img5a = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                             region5a,
                                             size5a,
                                             false,
                                             {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5a;
        REQUIRE_NOTHROW(tiffio.write(img5a, "scratch/res_a.tif", compression5a));

        auto region5b = std::make_shared<cserve::IIIFRegion>("512,0,512,512");
        auto size5b = std::make_shared<cserve::IIIFSize>("256,256");
        cserve::IIIFImage img5b = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                              region5b,
                                              size5b,
                                              false,
                                              {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5b;
        REQUIRE_NOTHROW(tiffio.write(img5b, "scratch/res_b.tif", compression5b));

        auto region5c = std::make_shared<cserve::IIIFRegion>("1024,0,176,512");
        auto size5c = std::make_shared<cserve::IIIFSize>("88,256");
        cserve::IIIFImage img5c = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                              region5c,
                                              size5c,
                                              false,
                                              {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5c;
        REQUIRE_NOTHROW(tiffio.write(img5c, "scratch/res_c.tif", compression5c));

        auto region5d = std::make_shared<cserve::IIIFRegion>("0,512,512,388");
        auto size5d = std::make_shared<cserve::IIIFSize>("256,194");
        cserve::IIIFImage img5d = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                              region5d,
                                              size5d,
                                              false,
                                              {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5d;
        REQUIRE_NOTHROW(tiffio.write(img5d, "scratch/res_d.tif", compression5d));

        auto region5e = std::make_shared<cserve::IIIFRegion>("512,512,512,388");
        auto size5e = std::make_shared<cserve::IIIFSize>("256,194");
        cserve::IIIFImage img5e = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                              region5e,
                                              size5e,
                                              false,
                                              {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5e;
        REQUIRE_NOTHROW(tiffio.write(img5e, "scratch/res_e.tif", compression5e));

        auto region5f = std::make_shared<cserve::IIIFRegion>("1024,512,176,388");
        auto size5f = std::make_shared<cserve::IIIFSize>("88,194");
        cserve::IIIFImage img5f = tiffio.read("data/tiff_01_rgb_pyramid.tif",
                                              region5f,
                                              size5f,
                                              false,
                                              {cserve::HIGH, cserve::HIGH, cserve::HIGH, cserve::HIGH});
        cserve::IIIFCompressionParams compression5f;
        REQUIRE_NOTHROW(tiffio.write(img5f, "scratch/res_f.tif", compression5f));

        auto res6 = Command::exec(R"(convert \( scratch/res_a.tif scratch/res_b.tif scratch/res_c.tif +append \) \( scratch/res_d.tif scratch/res_e.tif scratch/res_f.tif +append \) -append scratch/res_comp.tif)");

        auto res7 = Command::exec("compare -quiet -metric ae data/tiff_01_rgb_halfsize.tif scratch/res_comp.tif scratch/out.png 2>&1");
        REQUIRE(res5 == CommandResult{"0", 0});
        std::filesystem::remove("scratch/res_a.tif");
        std::filesystem::remove("scratch/res_b.tif");
        std::filesystem::remove("scratch/res_c.tif");
        std::filesystem::remove("scratch/res_d.tif");
        std::filesystem::remove("scratch/res_e.tif");
        std::filesystem::remove("scratch/res_f.tif");
        std::filesystem::remove("scratch/res_comp.tif");
        std::filesystem::remove("scratch/out.png");

    }
}

