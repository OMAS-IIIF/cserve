//
// Created by Lukas Rosenthaler on 2019-05-23.
//

#ifndef IIIF_SIPIIDENTIFIER_H
#define IIIF_SIPIIDENTIFIER_H

#include <string>

namespace cserve {

    class IIIFIdentifier {
    private:
        std::string identifier;
        int page;
        void parse(const std::string &str);
    public:
        inline IIIFIdentifier() : identifier(), page(0) { }

        explicit IIIFIdentifier(const std::string &str);

        IIIFIdentifier(const IIIFIdentifier &other);

        IIIFIdentifier(IIIFIdentifier &&other) noexcept;

        [[nodiscard]]
        const std::string &get_identifier() const {
            return identifier;
        }

        IIIFIdentifier &operator=(const IIIFIdentifier &other);

        IIIFIdentifier &operator=(IIIFIdentifier &&other);

        [[nodiscard]]
        int get_page() const {
            return page;
        }
    };
}
#endif //IIIF_SIPIIDENTIFIER_H
