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
    public:
        inline IIIFIdentifier() {
            page = 0;
        }

        IIIFIdentifier(const std::string &str);

        const std::string &get_identifier() const {
            return identifier;
        }

        int get_page() const {
            return page;
        }
    };
}
#endif //IIIF_SIPIIDENTIFIER_H
