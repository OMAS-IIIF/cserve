// An implementation of the C++14 make_unique function, copied from the standards document
// <http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2013/n3656.htm>.

#ifndef SIPI_MAKEUNIQUE_H
#define SIPI_MAKEUNIQUE_H

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace shttps {
    template<class T>
    struct Unique_if {
        [[maybe_unused]] typedef std::unique_ptr<T> Single_object;
    };

    template<class T>
    struct Unique_if<T[]> {
        [[maybe_unused]] typedef std::unique_ptr<T[]> Unknown_bound;
    };

    template<class T, size_t N>
    struct Unique_if<T[N]> {
        [[maybe_unused]] typedef void Known_bound;
    };

    template<class T, class... Args>
    [[maybe_unused]] typename Unique_if<T>::_Single_object make_unique(Args &&... args) {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }

    template<class T>
    typename Unique_if<T>::_Unknown_bound make_unique(size_t n) {
        typedef typename std::remove_extent<T>::type U;
        return std::unique_ptr<T>(new U[n]());
    }

    template<class T, class... Args>
    [[maybe_unused]] typename Unique_if<T>::_Known_bound make_unique(Args &&...) = delete;
}

#endif //SIPI_MAKEUNIQUE_H
