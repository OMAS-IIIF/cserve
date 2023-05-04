#ifndef __iiif_region_h
#define __iiif_region_h

#include <string>

namespace cserve {

    class IIIFRegion {
    public:
        typedef enum {
            FULL,        //!< no region, full image
            SQUARE,      //!< largest square
            COORDS,      //!< region given by x, y, width, height
            PERCENTS     //!< region (x,y,width height) given in percent of full image
        } CoordType;

    private:
        CoordType coord_type;
        float rx, ry, rw, rh;
        int32_t x, y;
        uint32_t w, h;
        bool canonical_ok;
        float reduce;

    public:
        inline IIIFRegion() : x(0), y(0), w(0), h(0), reduce(1.0F) {
            coord_type = FULL;
            rx = ry = rw = rh = 0.F;
            canonical_ok = false;
        }

        inline IIIFRegion(int32_t x, int32_t y, uint32_t w, uint32_t h) : x(0), y(0), w(0), h(0), reduce(1.0F) {
            coord_type = COORDS;
            rx = (float) x;
            ry = (float) y;
            rw = (float) w;
            rh = (float) h;
            canonical_ok = false;
        };

        explicit IIIFRegion(std::string str);

        inline void set_reduce(float reduce_p) { reduce = reduce_p; }

        [[nodiscard]]
        inline CoordType getType() const { return coord_type; };

        /*!
         * Get the region parameters to do the actual cropping. The parameters returned are
         * adjusted so that the returned region is within the bounds of the original image
         *
         * \param[in] nx Width of original image
         * \param[in] ny Height of original image
         * \param[out] p_x X position of region
         * \param[out] p_y Y position of region
         * \param[out] p_w Width of region
         * \param[out] p_h height of region
         *
         * \returns CoordType
         */
        CoordType crop_coords(uint32_t nx, uint32_t ny, int32_t &p_x, int32_t &p_y, uint32_t &p_w, uint32_t &p_h);

        /*!
         * Returns the canoncial IIIF string for the given region
         *
         * \param[in] Pointer to character buffer
         * \param[in] Length of the character buffer
         */
        void canonical(char *buf, int buflen);

        std::string canonical();

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFRegion &rhs);
    };

}

#endif
