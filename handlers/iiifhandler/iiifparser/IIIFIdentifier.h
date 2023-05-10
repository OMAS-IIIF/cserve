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
        void parse(const std::string &str);
    public:
        inline IIIFIdentifier() : identifier() { }

        explicit IIIFIdentifier(const std::string &str);

        IIIFIdentifier(const IIIFIdentifier &other);

        IIIFIdentifier(IIIFIdentifier &&other) noexcept;

        [[nodiscard]]
        const std::string &get_identifier() const {
            return identifier;
        }

        IIIFIdentifier &operator=(const IIIFIdentifier &other);

        IIIFIdentifier &operator=(IIIFIdentifier &&other);

    };
}
#endif //IIIF_SIPIIDENTIFIER_H
