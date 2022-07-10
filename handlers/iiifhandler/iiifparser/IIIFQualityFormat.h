#ifndef SIPI_SIPIQUALITYFORMAT_H
#define SIPI_SIPIQUALITYFORMAT_H

#include <string>


namespace cserve {

    class IIIFQualityFormat {
    public:
        typedef enum {
            DEFAULT, COLOR, GRAY, BITONAL
        } QualityType;
        typedef enum {
            UNSUPPORTED, JPG, TIF, PNG, GIF, JP2, PDF, WEBP
        } FormatType;

    private:
        QualityType quality_type;
        FormatType format_type;

    public:
        inline IIIFQualityFormat() {
            quality_type = IIIFQualityFormat::DEFAULT;
            format_type = IIIFQualityFormat::JPG;
        }

        IIIFQualityFormat(std::string str);

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFQualityFormat &rhs);

        inline QualityType quality() { return quality_type; };

        inline FormatType format() { return format_type; };
    };

}

#endif //SIPI_SIPIQUALITYFORMAT_H
