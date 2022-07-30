//
// Created by Lukas Rosenthaler on 27.07.22.
//

#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch_all.hpp"
#include "../IIIFError.h"
#include "../iiifparser/IIIFRegion.h"
#include "../iiifparser/IIIFSize.h"

TEST_CASE("Testing IIIFError class", "[IIIFError]") {
    std::string msg("test message");
    cserve::IIIFError err(__FILE__, __LINE__, msg); int l = __LINE__;
    REQUIRE(err.getMessage() == msg);
    REQUIRE(err.getLine() == l);

    cserve::IIIFError err2(__FILE__, __LINE__, "gagaga"); int l2 = __LINE__;
    REQUIRE(err2.getMessage() == "gagaga");
    REQUIRE(err2.getLine() == l2);
}

TEST_CASE("Testing IIIFRegion class" "[IIIFRegion]") {

    SECTION("Default constructor") {
        cserve::IIIFRegion reg;
        REQUIRE(reg.getType() == cserve::IIIFRegion::FULL);
    }

    SECTION("FULL constructor") {
        cserve::IIIFRegion reg("full");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        REQUIRE(t == cserve::IIIFRegion::FULL);
        REQUIRE(rx == 0);
        REQUIRE(ry == 0);
        REQUIRE(rw == 1000);
        REQUIRE(rh == 1500);
        char can[1024];
        reg.canonical(can, 1024);
        REQUIRE(strcmp(can, "full") == 0);
    }

    SECTION("SQUARE constructor") {
        cserve::IIIFRegion reg("square");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        REQUIRE(t == cserve::IIIFRegion::SQUARE);
        REQUIRE(rx == 0);
        REQUIRE(ry == 250);
        REQUIRE(rw == 1000);
        REQUIRE(rh == 1000);
    }

    SECTION("Coords constructor") {
        cserve::IIIFRegion reg("10,20,100,200");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        REQUIRE(t == cserve::IIIFRegion::COORDS);
        REQUIRE(rx == 10);
        REQUIRE(ry == 20);
        REQUIRE(rw == 100);
        REQUIRE(rh == 200);
    }

    SECTION("Coords constructor too big") {
        cserve::IIIFRegion reg("800,1200,400,600");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        REQUIRE(t == cserve::IIIFRegion::COORDS);
        REQUIRE(rx == 800);
        REQUIRE(ry == 1200);
        REQUIRE(rw == 200);
        REQUIRE(rh == 300);
    }

    SECTION("Coords constructor outside x") {
        cserve::IIIFRegion reg("1200,1200,400,600");
        int rx, ry;
        size_t rw, rh;
        REQUIRE_THROWS_AS(reg.crop_coords(1000, 1500, rx, ry, rw, rh), cserve::IIIFError);
    }

    SECTION("Coords constructor outside y") {
        cserve::IIIFRegion reg("1200,1501,400,600");
        int rx, ry;
        size_t rw, rh;
        REQUIRE_THROWS_AS(reg.crop_coords(1000, 1500, rx, ry, rw, rh), cserve::IIIFError);
    }

    SECTION("Percents constructor") {
        cserve::IIIFRegion reg("pct:10,20,30,40");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        REQUIRE(t == cserve::IIIFRegion::PERCENTS);
        REQUIRE(rx == 100);
        REQUIRE(ry == 300);
        REQUIRE(rw == 300);
        REQUIRE(rh == 600);
    }

    SECTION("Canonical size") {
        cserve::IIIFRegion reg("pct:10,20,30,40");
        int rx, ry;
        size_t rw, rh;
        cserve::IIIFRegion::CoordType t = reg.crop_coords(1000, 1500, rx, ry, rw, rh);
        std::string canonical = reg.canonical();
        REQUIRE(canonical == "100,300,300,600");
    }
}

TEST_CASE("Testing IIIFSize class" "[IIIFSize]") {
    SECTION("IIIFSize empty") {
        cserve::IIIFSize siz;
        REQUIRE(siz.get_type() == cserve::IIIFSize::UNDEFINED);
        REQUIRE(siz.undefined());
    }

    SECTION("IIIFSize 'reduce'") {
        cserve::IIIFSize siz(2);
        REQUIRE(siz.get_type() == cserve::IIIFSize::REDUCE);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(1200, 2000, w, h, reduce, redonly);
        REQUIRE(reduce == 2);
        REQUIRE(redonly);
        REQUIRE(w == 300);
        REQUIRE(h == 500);

        cserve::IIIFSize siz2(2, 100, 200);
        REQUIRE(siz2.get_type() == cserve::IIIFSize::REDUCE);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(1200, 2000, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);

        cserve::IIIFSize siz3(2, 0, 0, 120 * 200);
        REQUIRE(siz3.get_type() == cserve::IIIFSize::REDUCE);
        size_t w3, h3;
        int reduce3 = -1;
        bool redonly3;
        REQUIRE_THROWS_AS(siz3.get_size(484, 800, w3, h3, reduce3, redonly3), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize 'max'") {
        cserve::IIIFSize siz("max", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(1200, 1800, w, h, reduce, redonly);
        REQUIRE(reduce == 0);
        REQUIRE(w == 1200);
        REQUIRE(h == 1800);

        cserve::IIIFSize siz2("max", 120, 180);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(484, 800, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize '^max'") {
        cserve::IIIFSize siz("^max", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(600, 800, w, h, reduce, redonly);
        REQUIRE(reduce == 0);
        REQUIRE(redonly);
        REQUIRE(w == 3000);
        REQUIRE(h == 4000);

        cserve::IIIFSize siz2("^max", 3000, 4000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        siz2.get_size(300, 300, w2, h2, reduce2, redonly2);
        REQUIRE(reduce == 0);
        REQUIRE(redonly);
        REQUIRE(w2 == 3000);
        REQUIRE(h2 == 3000);

        cserve::IIIFSize siz3("^max", 4000, 3000);
        size_t w3, h3;
        int reduce3 = -1;
        bool redonly3;
        siz3.get_size(300, 300, w3, h3, reduce3, redonly3);
        REQUIRE(reduce == 0);
        REQUIRE(redonly);
        REQUIRE(w3 == 3000);
        REQUIRE(h3 == 3000);

        cserve::IIIFSize siz4("^max", 0, 0, 1000 * 1000);
        size_t w4, h4;
        int reduce4 = -1;
        bool redonly4;
        siz4.get_size(250, 500, w4, h4, reduce4, redonly4);
        REQUIRE(reduce == 0);
        REQUIRE(redonly);
        REQUIRE(w4 == 707);
        REQUIRE(h4 == 1414);
    }

    SECTION("IIIFSize 'w,'") {
        cserve::IIIFSize siz("1000,", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(3000, 6000, w, h, reduce, redonly);
        REQUIRE(w == 1000);
        REQUIRE(h == 2000);

        cserve::IIIFSize siz1("1000,", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        REQUIRE_THROWS_AS(siz1.get_size(500, 1000, w1, h1, reduce1, redonly1), cserve::IIIFSizeError);

        cserve::IIIFSize siz2("1000,", 1000, 1000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(2000, 3000, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize '^w,'") {
        cserve::IIIFSize siz("^1000,", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(3000, 6000, w, h, reduce, redonly);
        REQUIRE(reduce == 1);
        REQUIRE_FALSE(redonly);
        REQUIRE(w == 1000);
        REQUIRE(h == 2000);

        cserve::IIIFSize siz1("^1000,", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        siz1.get_size(500, 1000, w1, h1, reduce1, redonly1);
        REQUIRE(w == 1000);
        REQUIRE(h == 2000);

        cserve::IIIFSize siz2("^1000,", 1000, 1000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(500, 800, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize ',h'") {
        cserve::IIIFSize siz(",1000", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(6000, 4000, w, h, reduce, redonly);
        REQUIRE(reduce == 2);
        REQUIRE(redonly);
        REQUIRE(w == 1500);
        REQUIRE(h == 1000);

        cserve::IIIFSize siz1(",1000", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        REQUIRE_THROWS_AS(siz1.get_size(1000, 500, w1, h1, reduce1, redonly1), cserve::IIIFSizeError);

        cserve::IIIFSize siz2(",1000", 1000, 1000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(3000, 2000, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize '^,h'") {
        cserve::IIIFSize siz("^,1000", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(600, 500, w, h, reduce, redonly);
        REQUIRE(reduce == 0);
        REQUIRE_FALSE(redonly);
        REQUIRE(w == 1200);
        REQUIRE(h == 1000);

        cserve::IIIFSize siz1("^,1000", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        siz1.get_size(1000, 500, w1, h1, reduce1, redonly1);
        REQUIRE(reduce == 0);
        REQUIRE_FALSE(redonly);
        REQUIRE(w1 == 2000);
        REQUIRE(h1 == 1000);

        cserve::IIIFSize siz2("^,1000", 1000, 1000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        REQUIRE_THROWS_AS(siz2.get_size(3000, 2000, w2, h2, reduce2, redonly2), cserve::IIIFSizeError);
    }

    SECTION("IIIFSize 'pct:n") {
        cserve::IIIFSize siz("pct:50", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(3000, 4000, w, h, reduce, redonly);
        REQUIRE(reduce == 1);
        REQUIRE(redonly);
        REQUIRE(w == 1500);
        REQUIRE(h == 2000);

        cserve::IIIFSize siz1("pct:101", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        REQUIRE_THROWS_AS(siz1.get_size(3000, 4000, w1, h1, reduce1, redonly1), cserve::IIIFSizeError);

        cserve::IIIFSize siz2("pct:33.333", 3000, 4000);
        size_t w2, h2;
        int reduce2 = -1;
        bool redonly2;
        siz2.get_size(3000, 4000, w2, h2, reduce2, redonly2);
        REQUIRE(reduce2 == 1);
        REQUIRE_FALSE(redonly2);
        REQUIRE(w2 == 1000);
        REQUIRE(h2 == 1334);
    }

    SECTION("IIIFSize '^pct:n") {
        cserve::IIIFSize siz("^pct:200", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(1000, 2000, w, h, reduce, redonly);
        REQUIRE(reduce == 0);
        REQUIRE_FALSE(redonly);
        REQUIRE(w == 2000);
        REQUIRE(h == 4000);
    }

    SECTION("IIIFSize 'w,h'") {
        cserve::IIIFSize siz("125,125", 3000, 4000);
        size_t w, h;
        int reduce = -1;
        bool redonly;
        siz.get_size(1000, 1000, w, h, reduce, redonly);
        REQUIRE(reduce == 3);
        REQUIRE(redonly);
        REQUIRE(w == 125);
        REQUIRE(h == 125);

        cserve::IIIFSize siz1("125,125", 3000, 4000);
        size_t w1, h1;
        int reduce1 = -1;
        bool redonly1;
        siz1.get_size(1000, 2000, w1, h1, reduce1, redonly1);
        REQUIRE(reduce1 == 3);
        REQUIRE_FALSE(redonly1);
        REQUIRE(w1 == 125);
        REQUIRE(h1 == 125);

    }
}