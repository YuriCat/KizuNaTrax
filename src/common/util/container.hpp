/*
 container.hpp
 Katsuki Ohto
 */

#ifndef UTIL_CONTAINER_HPP_
#define UTIL_CONTAINER_HPP_

// 集合を扱う時のユーティリティ的な何か

// 基準位置をnずらす
// 不可逆で良い
template<class container_t>
container_t* splitHorizon(container_t *const c, const int n)noexcept{
    return c + n; // 配列ならn番目のポインタを返す
}

template<class container_t>
const container_t* splitHorizon(const container_t c[], const int n)noexcept{
    return (const container_t*)((&(c[0])) + n); // 配列ならn番目のポインタを返す
}

/*template<class container_t>
const container_t* splitHorizon(const container_t *const c, const int n)noexcept{
    return c + n; // 配列ならn番目のポインタを返す
}

template<class container_t>
const container_t* splitHorizon(const container_t c[], const int n)noexcept{
    return (container_t*)((&(c[0])) + n); // 配列ならn番目のポインタを返す
}*/

#endif // UTIL_CONTAINER_HPP_
