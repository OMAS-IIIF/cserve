//
// Created by Lukas Rosenthaler on 05.08.21.
//

#ifndef CSERVER_LIB_REQUESTHANDLERDATA_H_
#define CSERVER_LIB_REQUESTHANDLERDATA_H_

#include <string>

namespace cserve {
    class RequestHandlerData {
    public:
        inline RequestHandlerData() = default;

        inline virtual ~RequestHandlerData(){}

    };

}

#endif //CSERVER_LIB_REQUESTHANDLERDATA_H_
