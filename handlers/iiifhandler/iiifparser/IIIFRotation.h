#ifndef IIIF_SIPIROTATION_H
#define IIIF_SIPIROTATION_H

#include <string>

namespace cserve {

//-------------------------------------------------------------------------
// local class to handle IIIF Rotation parameters
//
    class IIIFRotation {
    private:
        bool mirror = false;
        float rotation = 0.F;

    public:
        IIIFRotation();

        IIIFRotation(std::string str);

        inline bool get_rotation(float &rot) {
            rot = rotation;
            return mirror;
        };

        friend std::ostream &operator<<(std::ostream &lhs, const IIIFRotation &rhs);
    };

}
#endif //IIIF_SIPIROTATION_H
