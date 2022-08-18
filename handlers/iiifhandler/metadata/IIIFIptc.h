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
#ifndef __defined_iptc_h
#define __defined_iptc_h

#include <string>
#include <vector>
#include <exiv2/iptc.hpp>

namespace cserve {

    /*!
     * Handles IPTC data based on the exiv2 library
     */
    class IIIFIptc {
    private:
        Exiv2::IptcData iptcData; //!< Private member variable holding the exiv2 IPTC object

    public:
        IIIFIptc(const IIIFIptc &rhs);

        IIIFIptc(IIIFIptc &&rhs);

        /*!
         * Constructor
         *
         * \param[in] Buffer containing the IPTC data in native format
         * \param[in] Length of the buffer
         */
        IIIFIptc(const unsigned char *iptc, unsigned int len);

        IIIFIptc(const std::vector<unsigned char> &iptc);


        /*!
         * Destructor
         */
        ~IIIFIptc();

        IIIFIptc &operator=(const IIIFIptc &rhs);

        IIIFIptc &operator=(IIIFIptc &&rhs);

        /*!
        * Returns the bytes of the IPTC data. The buffer must be
        * deleted by the caller after it is no longer used!
        * \param[out] len Length of the data in bytes
        * \returns Chunk of chars holding the IPTC data
        */
        std::unique_ptr<unsigned char[]> iptcBytes(unsigned int &len);

        /*!
         * Returns the bytes of the IPTC data as std::vector
         * @return IPTC bytes as std::vector
         */
        std::vector<unsigned char> iptcBytes(void);

        inline Exiv2::Iptcdatum getValByKey(const std::string key_p) {
            return iptcData[key_p];
        }

        /*!
         * The overloaded << operator which is used to write the IPTC data formatted to the outstream
         *
         * \param[in] lhs The output stream
         * \param[in] rhs Reference to an instance of a SipiIptc
         * \returns Returns ostream object
         */
        friend std::ostream &operator<<(std::ostream &lhs, IIIFIptc &rhs);

    };

}

#endif
