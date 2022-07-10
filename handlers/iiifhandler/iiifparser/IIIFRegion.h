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
        int x, y;
        size_t w, h;
        bool canonical_ok;

    public:
        inline IIIFRegion() {
            coord_type = FULL;
            rx = ry = rw = rh = 0.F;
            canonical_ok = false;
        }

        inline IIIFRegion(int x, int y, size_t w, size_t h) {
            coord_type = COORDS;
            rx = (float) x;
            ry = (float) y;
            rw = (float) w;
            rh = (float) h;
            canonical_ok = false;
        };

        IIIFRegion(std::string str);

        /*!
         * Get the coordinate type that has bee used for construction of the region
         *
         * \returns CoordType
         */
        inline CoordType getType() { return coord_type; };

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
        CoordType crop_coords(size_t nx, size_t ny, int &p_x, int &p_y, size_t &p_w, size_t &p_h);

        /*!
         * Returns the canoncial IIIF string for the given region
         *
         * \param[in] Pointer to character buffer
         * \param[in] Length of the character buffer
         */
        void canonical(char *buf, int buflen);

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFRegion &rhs);
    };

}

#endif
