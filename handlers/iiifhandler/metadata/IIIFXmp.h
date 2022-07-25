/*
 * Copyright © 2022 Lukas Rosenthaler
 * This file is part of OMAS/cserve
 * OMAS/cserve is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * OMAS/cserve is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef __defined_xmp_h
#define __defined_xmp_h


#include <string>
#include <mutex>
#include <exiv2/xmp_exiv2.hpp> //!< Import xmp from the exiv2 library!
#include <exiv2/error.hpp>

namespace cserve {

    typedef struct {
        std::mutex lock;
    } XmpMutex;

    extern XmpMutex xmp_mutex;

    extern void xmplock_func(void *pLockData, bool lockUnlock);

    /*!
    * This class handles XMP metadata. It uses the Exiv2 library
    */
    class IIIFXmp {
    private:
        Exiv2::XmpData xmpData; //!< Private member variable holding the exiv2 XMP data
        std::string __xmpstr;

    public:

        IIIFXmp(const IIIFXmp &rhs);

        IIIFXmp(IIIFXmp &&rhs);

        /*!
         * Constructor
         *
         * \param[in] xmp A std::string containing RDF/XML with XMP data
         */
        IIIFXmp(const std::string &xmp);


        /*!
         * Constructor
         *
         * \param[in] xmp A C-string (char *)containing RDF/XML with XMP data
         */
        IIIFXmp(const char *xmp);

        /*!
         * Constructor
         *
         * \param[in] xmp A string containing RDF/XML with XMP data
         * \param[in] len Length of the string
         */
        IIIFXmp(const char *xmp, int len);


        /*!
         * Destructor
         */
        ~IIIFXmp();

        IIIFXmp &operator=(const IIIFXmp &rhs);

        IIIFXmp &operator=(IIIFXmp &&rhs);

        /*!
        * Returns the bytes of the RDF/XML data
        * \param[out] len Length of the data on bytes
        * \returns Chunk of chars holding the xmp data
        */
        char *xmpBytes(unsigned int &len);

        /*!
         * Returns the bytes of the RDF/XML data as std::string
         *
         * @return String holding the xmp data
         */
        std::string xmpBytes(void);

        /*!
         * The overloaded << operator which is used to write the xmp formatted to the outstream
         *
         * \param[in] lhs The output stream
         * \param[in] rhs Reference to an instance of a SipiXmp
         * \returns Returns ostream object
         */
        friend std::ostream &operator<<(std::ostream &lhs, const IIIFXmp &rhs);

    };

}

#endif
