
#ifndef __iiif_size_h
#define __iiif_size_h

#include <string>
#include <utility>

namespace cserve {

    /**
     * Error class for handling invalid IIIF size parameters that should result in returning a HTTP error code
     */
    class IIIFSizeError : std::exception {
    private:
        int http_code;
        std::string description;
    public:
        inline IIIFSizeError(int http_code, std::string description) : http_code(http_code), description(std::move(description)) {};

        [[nodiscard]]
        inline int getHttpCode() const { return http_code; }

        inline std::string getDescription() { return description; }

        [[nodiscard]]
        inline std::string to_string() const {
            return "SipiSizeError: " + description;
        };

        inline explicit operator std::string() const {
            return to_string();
        }

    };

    /*!
     * \class SipiSize
     * This class handles both the Size (or scale) parameter of a IIIF-request, but also deals
     * with the reduce parameter which can be used in reading J2K images
     * In order to support the reduce, the syntax "red:int" can be used, e.g.:
     * http://{url}/{prefix}/{identifier}/{region}/red:3/{rotation}/default.jpg
     * The class can be constructed in several ways and just stores the parameters.
     * The width/height that should be applied to the scaling is calculated by calling
     * the get_size() method. It is to note that get_size() may also return a reduce
     * factor. This may be used in case of the JPEG2000 where it es much more efficient
     * to have the reduce factor while reading the image (since using a reduce > 0 means
     * that only a fraction of the input file has to be read)
     *
     */
    class IIIFSize {
    public:
        typedef enum {
            UNDEFINED, //!< Is not initialized
            FULL,      //!< The full size of the image is served, IIIF: "full" or "max"
            PIXELS_XY, //!< Both X and Y is given, the image may be distorted, IIIF: "xxx,yyy"
            PIXELS_X,  //!< Only the X dimension is given, the Y dimension is calculated (no distortion), IIIF: "xxx,"
            PIXELS_Y,  //!< Only the Y dimension is given, the X dimension is calculated (no distortion), IIIF: ",yyy"
            MAXDIM,    //!< Either X or Y dimensions are taken, so that the resulting image fits in the rectangle (no distortion), IIIF: "!xxx,yyy"
            PERCENTS,  //!< The percentage of scaling is given, IIIF: "pct:ppp"
            REDUCE     //!< A reduce factor can be given, 0=no scaling, 1=0.5, 2=0.25, 3=0.125,...(Note: this is an extension to the IIIF standard) IIIF: "red:ii"
        } SizeType;

    private:
        static size_t limitdim; //!< maximal dimension of an image
        SizeType size_type; //!< Holds the type of size/scaling parameters given
        bool upscaling;
        float percent;      //!< if the scaling is given in percent, this holds the value
        int reduce;         //!< if the scaling is given by a reduce value
        bool redonly;       //!< we *only* have a reduce in the resulting size
        size_t nx, ny;      //!< the parameters given
        size_t w, h;        //!< the resulting width and height after processing
        size_t iiif_max_width, iiif_max_height, iiif_max_area;
        bool canonical_ok;

    public:
        /*!
         * Default constructor (full size)
         */
        IIIFSize() : size_type(SizeType::UNDEFINED), upscaling(false), percent(0.0F), reduce(0), redonly(false),
            nx(0), ny(0), w(0), h(0), iiif_max_height(0), iiif_max_width(0), iiif_max_area(0), canonical_ok(false) {}

        /*!
         * Constructor with reduce parameter (reduce=0: full image, reduce=1: 1/2, reduce=2: 1/4,…)
         *
         * \param[in] reduce_p Reduce parameter
         */
        explicit IIIFSize(int reduce_p, size_t iiif_max_width = 0, size_t iiif_max_height = 0, size_t iiif_max_area = 0)
                : size_type(SizeType::REDUCE), upscaling(false), percent(0.0F), reduce(reduce_p), redonly(false), nx(0),
                  ny(0), w(0), h(0), iiif_max_width(iiif_max_width), iiif_max_height(iiif_max_height),
                  iiif_max_area(iiif_max_area), canonical_ok(false) {}

        /*!
         * Constructor with percentage parameter
         *
         * \param[in] percent_p Percentage parameter
         */
        explicit IIIFSize(float percent_p, size_t iiif_max_width = 0, size_t iiif_max_height = 0, size_t iiif_max_area = 0)
                : size_type(
                SizeType::PERCENTS), upscaling(false), percent(percent_p), reduce(false), redonly(false), nx(0), ny(0),
                w(0), h(0), iiif_max_width(iiif_max_width), iiif_max_height(iiif_max_height),
                iiif_max_area(iiif_max_area), canonical_ok(false) {}

        /*!
         * Construcor taking size/scale part of IIIF url as parameter
         *
         * \param[in] str String with the IIIF url part containing the size/scaling information
         */
        explicit IIIFSize(std::string str, size_t iiif_max_width = 0, size_t iiif_max_height = 0, size_t iiif_max_area = 0);

        IIIFSize(const IIIFSize &other);

        IIIFSize(IIIFSize &&other) noexcept ;

        IIIFSize &operator=(const IIIFSize &other);

        IIIFSize &operator=(IIIFSize &&other);

        /*!
         * Comparison operator ">"
         */
        bool operator>(const IIIFSize &s) const;

        /*!
         * Comparison operator ">="
         */
        bool operator>=(const IIIFSize &s) const;

        /*!
         * Comparison operator "<"
         */
        bool operator<(const IIIFSize &s) const;

        /*!
         * Comparison operator "<="
         */
        bool operator<=(const IIIFSize &s) const;

        /*!
         * Get the coordinate type that has bee used for construction of the region
         *
         * \returns CoordType
         */
        inline SizeType get_type() { return size_type; };

        /*!
         * Test, if size object has been initialized with a IIIF string
         *
         * \returns true, if object has not been initialized by a IIIF string
         */
        [[nodiscard]]
        inline bool undefined() const { return (nx == 0) && (ny == 0) && (percent == 0.0F); }

        /*!
         * Get the size to which the image should be scaled
         *
         * \param[out] nx_p Width of scaled image
         * \param[out] nx_p Height of scaled image
         * \param[out] percent_p Percentage of scaling (if percentage was indicated)
         *
         * \returns enum SizeType which indicates how the size was specified
         */
        /*
         inline SizeType get_size(int &nx_p, int &ny_p, float &percent_p) {
             nx_p = nx; ny_p = ny; percent_p = percent;
             return size_type;
         };
         */
        /*!
         * Get the size to which the image should be scaled
         *
         * \param[in] nx Width of original images
         * \param[in] ny original height of image
         * \param[out] w_p Width of scaled image
         * \param[out] h_p Height of scaled image
         * \param[in,out] reduce_p Reduce parameter (especially for J2K images with resolution pyramid). As
         * input specifiy the maximal reduce factor that is allowed (0 if no limit), as output the optimal
         * reduce factor is returned.
         * \param[out] redonly_p True, if scaling can be made with the reduce parameter only
         *
         * \returns enum SizeType which indicates how the size was specified
         */
        IIIFSize::SizeType get_size(size_t nx, size_t ny, size_t &w_p, size_t &h_p, int &reduce_p, bool &redonly_p);

        /*!
         * Returns the canoncial IIIF string for the given size/scaling
         *
         * \param[in] Pointer to character buffer
         * \param[in] Length of the character buffer
         */
        void canonical(char *buf, int buflen);

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFSize &rhs);
    };

}

#endif
