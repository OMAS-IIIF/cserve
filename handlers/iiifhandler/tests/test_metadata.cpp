//
// Created by Lukas Rosenthaler on 02.08.22.
//
#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include <filesystem>
#include <iostream>
#include <fstream>

#include <lcms2.h>

#include "catch2/catch_all.hpp"
#include "../metadata/IIIFEssentials.h"
#include "../metadata/IIIFExif.h"
#include "../metadata/IIIFIptc.h"
#include "../metadata/IIIFIcc.h"
#include "../metadata//IIIFXmp.h"

#include "tiffio.h"

std::shared_ptr<unsigned char[]> iptc_from_tiff(const std::string &filename, unsigned int &len) {
    unsigned char *iptc_content = nullptr;

    TIFF *tif = TIFFOpen(filename.c_str(), "r");
    if (tif == nullptr) {
        UNSCOPED_INFO("TIFFOpen failed on: " << filename);
        return nullptr;
    }
    if (TIFFGetField(tif, TIFFTAG_RICHTIFFIPTC, &len, &iptc_content) != 0) {
        auto iptc = std::make_unique<unsigned char[]>(len);
        memcpy(iptc.get(), iptc_content, len);
        TIFFClose(tif);
        return std::move(iptc);
    }
    TIFFClose(tif);
    len = 0;
    return nullptr;
}

std::shared_ptr<unsigned char[]> icc_from_tiff(const std::string &filename, unsigned int &len) {
    unsigned char *icc_content = nullptr;

    TIFF *tif = TIFFOpen(filename.c_str(), "r");
    if (tif == nullptr) {
        UNSCOPED_INFO("TIFFOpen failed on: " << filename);
        return nullptr;
    }
    if (TIFFGetField(tif, TIFFTAG_ICCPROFILE, &len, &icc_content) == 1) {
        std::shared_ptr<unsigned char[]> icc(new unsigned char[len]);
        //auto icc = std::make_shared<unsigned char>(len);
        memcpy(icc.get(), icc_content, len);
        TIFFClose(tif);
        return std::move(icc);
    }
    TIFFClose(tif);
    len = 0;
    return nullptr;
}

std::shared_ptr<char[]> xmp_from_tiff(const std::string &filename, unsigned int &len) {
    char *xmp_content = nullptr;

    TIFF *tif = TIFFOpen(filename.c_str(), "r");
    if (tif == nullptr) {
        UNSCOPED_INFO("TIFFOpen failed on: " << filename);
        return nullptr;
    }
    if (TIFFGetField(tif, TIFFTAG_XMLPACKET, &len, &xmp_content) == 1) {
        std::shared_ptr<char[]> xmp(new char[len]);
        memcpy(xmp.get(), xmp_content, len);
        TIFFClose(tif);
        return std::move(xmp);
    }
    TIFFClose(tif);
    len = 0;
    return nullptr;
}


inline std::vector<uint8_t> read_file(const std::string& file_path) {
    std::ifstream instream(file_path, std::ios::in | std::ios::binary);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(instream)), std::istreambuf_iterator<char>());
    return data;
}

TEST_CASE("Testing IIIFEssentials class", "[IIIFEssentials]") {
    SECTION("IIIFEssentials constructors 1") {
        cserve::IIIFEssentials essentials;
        REQUIRE_FALSE(essentials.is_set());
    }

    std::string origname{"test/testfile.jpg"};
    std::string mimetype{"image/jpeg"};
    cserve::HashType htype = cserve::sha256;
    std::string checksum{"abcdefghijklmnopqrstuvwxyz"};
    std::vector<unsigned char> icc_profile{'I', 'C', 'C', 'p', 'r', 'o', 'f', 'i', 'l', 'e'};

    SECTION("IIIFEssentials constructors 2") {
        cserve::IIIFEssentials essentials1(origname, mimetype, htype, checksum, icc_profile);
        REQUIRE(essentials1.is_set());
        REQUIRE(essentials1.origname() == origname);
        REQUIRE(essentials1.mimetype() == mimetype);
        REQUIRE(essentials1.hash_type() == htype);
        REQUIRE(essentials1.data_chksum() == checksum);
        REQUIRE(essentials1.use_icc());
        REQUIRE(essentials1.icc_profile() == icc_profile);
    }

    SECTION("IIIFEssentials constructors 3") {
        cserve::IIIFEssentials essentials1(origname, mimetype, htype, checksum);
        REQUIRE(essentials1.is_set());
        REQUIRE(essentials1.origname() == origname);
        REQUIRE(essentials1.mimetype() == mimetype);
        REQUIRE(essentials1.hash_type() == htype);
        REQUIRE(essentials1.data_chksum() == checksum);
        REQUIRE_FALSE(essentials1.use_icc());
    }

    SECTION("IIIFEssentials constructors 3") {
        cserve::IIIFEssentials essentials(origname, mimetype, htype, checksum, icc_profile);
        std::string serialized = std::string(essentials);
        REQUIRE(serialized == "test/testfile.jpg|image/jpeg|sha256|abcdefghijklmnopqrstuvwxyz|USE_ICC|SUNDcHJvZmlsZQ==");
        cserve::IIIFEssentials essentials1{serialized};
        REQUIRE(essentials1.is_set());
        REQUIRE(essentials1.origname() == origname);
        REQUIRE(essentials1.mimetype() == mimetype);
        REQUIRE(essentials1.hash_type() == htype);
        REQUIRE(essentials1.data_chksum() == checksum);
        REQUIRE(essentials1.use_icc());
        REQUIRE(essentials1.icc_profile() == icc_profile);
    }

    SECTION("IIIFEssentials constructors 4") {
        cserve::IIIFEssentials essentials(origname, mimetype, htype, checksum);
        std::string serialized = std::string(essentials);
        REQUIRE(serialized == "test/testfile.jpg|image/jpeg|sha256|abcdefghijklmnopqrstuvwxyz|IGNORE_ICC|NULL");
        cserve::IIIFEssentials essentials1{serialized};
        REQUIRE(essentials1.is_set());
        REQUIRE(essentials1.origname() == origname);
        REQUIRE(essentials1.mimetype() == mimetype);
        REQUIRE(essentials1.hash_type() == htype);
        REQUIRE(essentials1.data_chksum() == checksum);
        REQUIRE_FALSE(essentials1.use_icc());
    }
}

TEST_CASE("Testing IIIFExif class", "[IIIFExif]") {
    SECTION("IIIFExif constructors") {
        cserve::IIIFExif exif;
        unsigned int w{1500};
        exif.addKeyVal("Exif.Image.ImageWidth", w);
        unsigned int h{1000};
        exif.addKeyVal("Exif.Image.ImageLength", h);
        unsigned short bps{8};
        exif.addKeyVal("Exif.Image.BitsPerSample", bps);
        unsigned short compression{1};
        exif.addKeyVal("Exif.Image.Compression", compression);
        std::string description("Eine Beschreibung...");
        exif.addKeyVal("Exif.Image.ImageDescription", description);

        unsigned int len{0};
        auto binbuf = exif.exifBytes(len);

        cserve::IIIFExif exif2(binbuf.get(), len);
        unsigned int w2;
        REQUIRE(exif2.getValByKey("Exif.Image.ImageWidth", w2));
        REQUIRE(w2 == w);
        unsigned int h2;
        REQUIRE(exif2.getValByKey("Exif.Image.ImageLength", h2));
        REQUIRE(h2 == h);
        unsigned short bps2;
        REQUIRE(exif2.getValByKey("Exif.Image.BitsPerSample", bps2));
        REQUIRE(bps2 == bps);
        unsigned short compression2;
        REQUIRE(exif2.getValByKey("Exif.Image.Compression", compression2));
        REQUIRE(compression2 == compression);
        std::string description2;
        REQUIRE(exif2.getValByKey("Exif.Image.ImageDescription", description2));
        REQUIRE(description2 == description);
    }
}


TEST_CASE("Testing IIIFIptc class", "[IIIFIptc]") {
    SECTION("IIIFIptc basic") {
        unsigned int len{};
        auto buf = iptc_from_tiff("/Users/rosenth/ProgDev/OMAS/cserve/handlers/iiifhandler/tests/data/IPTC-PhotometadataRef-Std2014_large.tiff",len);
        REQUIRE_NOTHROW(cserve::IIIFIptc(buf.get(), len));
        std::vector<unsigned char> vbuf(buf.get(), buf.get() + len);
        REQUIRE_NOTHROW(cserve::IIIFIptc(vbuf));
        cserve::IIIFIptc iptc(vbuf);
        unsigned int len2 = 0;
        std::unique_ptr<unsigned char[]> nbuf = iptc.iptcBytes(len2);
        REQUIRE(len == len2);
    }
}


TEST_CASE("Testing IIIFIcc class", "[IIIFIcc]") {
    SECTION("IIIFIcc basic") {
        unsigned int len{};
        auto iccbuf = icc_from_tiff("/Users/rosenth/ProgDev/OMAS/cserve/handlers/iiifhandler/tests/data/image_with_icc_profile.tif",len);
        REQUIRE_NOTHROW(cserve::IIIFIcc(iccbuf.get(), len));
        REQUIRE_NOTHROW(cserve::IIIFIcc(cserve::icc_AdobeRGB));
        cserve::IIIFIcc icc(cserve::icc_AdobeRGB);
         auto nicc_buf = icc.iccBytes(len);
        REQUIRE(len == 560);
        std::vector<unsigned char> vicc_buf = icc.iccBytes();
        REQUIRE(vicc_buf.size() == 560);
        cmsHPROFILE profile = icc.getIccProfile();
        REQUIRE(profile != nullptr);
        unsigned int formatter = icc.iccFormatter(16);
        REQUIRE(formatter == 262170);
    }
}


TEST_CASE("Testing IIIFXmp class", "[IIIFXmp]") {
    SECTION("IIIFXmp basic") {
        unsigned int len;
        auto xmpbuf = xmp_from_tiff("/Users/rosenth/ProgDev/OMAS/cserve/handlers/iiifhandler/tests/data/IMG_8067.tiff",len);
        REQUIRE(len == 18845);
        REQUIRE_NOTHROW(cserve::IIIFXmp(xmpbuf.get(), len));
        cserve::IIIFXmp xmp(xmpbuf.get(), len);
        unsigned int len2;
        auto xmp2 = xmp.xmpBytes(len2);
        REQUIRE(xmp2 != nullptr);
        std::string xmp_str = xmp.xmpBytes();
        REQUIRE(xmp_str.size() == 18845);
    }
}
