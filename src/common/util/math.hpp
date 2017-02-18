/*
 math.hpp
 Katsuki Ohto
 */

#ifndef UTIL_MATH_HPP_
#define UTIL_MATH_HPP_

#include <cmath>
#include <cassert>

// 数学的定数
constexpr unsigned int FactorialTable[12] = {
    1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880, 3628800, 39916800,
};

static double dFactorial(int n){
    assert(n >= 0);
    
    double ans;
    if(n < 12){
        ans = (double)FactorialTable[n];
    }else{
        ans = 1.0;
        while(n > 1){
            ans *= (double)n;
            n--;
        }
    }
    return ans;
}

static double dPermutation(int n, int r){
    double ans = 1.0;
    while(r--){
        ans *= (n - r);
    }
    return ans;
}

static double dCombination(int n, int r){
    if(n >= r){
        return dPermutation(n, r) / dFactorial(r);
    }else{
        return 0;
    }
}

/*double dCombination4(int n, int r1, int r2, int r3, int r4){
    if(n >= (r1 + r2 + r3 + r4)){
        return dFactorial(n) / dFactorial(r1) / dFactorial(r2) / dFactorial(r3) / dFactorial(r4);
    }else{
        return 0;
    }
}*/

static double dCombination3(int r1, int r2, int r3){
    if(r1 + r2 + r3 >= 0){
        return dFactorial(r1 + r2 + r3) / dFactorial(r1) / dFactorial(r2) / dFactorial(r3);
    }else{
        return 0;
    }
}

static double dCombination4(int r1, int r2, int r3, int r4){
    if(r1 + r2 + r3 + r4 >= 0){
        return dFactorial(r1 + r2 + r3 + r4) / dFactorial(r1) / dFactorial(r2) / dFactorial(r3) / dFactorial(r4);
    }else{
        return 0;
    }
}

static double dCombination5( int r1, int r2, int r3, int r4, int r5){
    if(r1 + r2 + r3 + r4 + r5 >= 0){
        return dFactorial(r1 + r2 + r3 + r4 + r5) / dFactorial(r1) / dFactorial(r2) / dFactorial(r3) / dFactorial(r4) / dFactorial(r5);
    }else{
        return 0;
    }
}

static double dCombination(const int r[], const int n){
    switch(n){
        case 0: break;
        case 1: break;
        case 2: break;
        case 3: return dCombination3(r[0], r[1], r[2]); break;
        case 4: return dCombination4(r[0], r[1], r[2], r[3]); break;
        case 5: return dCombination5(r[0], r[1], r[2], r[3], r[4]); break;
        default: break;
    }
    return 0;
}

static int nC2(const int n){
    int res = 1;
    int nn = n;
    while(nn > 2){
        res *= nn;
        --nn;
    }
    nn = n - 2;
    while(nn){
        res /= nn;
        --nn;
    }
    return res;
}

constexpr static int ipow(int m, int n)noexcept{
    return (n <= 0) ? 1 : (m * ipow(m, n - 1));
}

// シグモイド変換、逆変換
static double sigmoid(double x, double alpha = 1){
    return 1 / (1 + exp(-x / alpha));
}
static double logit(double s, double alpha = 1){
    return -log((1.0 / s) - 1.0) * alpha;
}

// ベータ関数
static double beta(double x, double y){
    return tgamma(x) * tgamma(y) / tgamma(x + y);
}
static double log_beta(double x, double y){
    //DERR << x << " " << y << " " << tgamma(x) << " " << tgamma(y) << endl;
    return lgamma(x) + lgamma(y) - lgamma(x + y);
}

// 多変数ベータ関数
template<std::size_t N>
static double multivariate_beta(const std::array<double, N>& x){
    double ret = 1;
    double sum = 0;
    for(double i : x){
        ret *= tgamma(i);
        sum += i;
    }
    return ret / tgamma(sum);
}
template<std::size_t N>
static double multivariate_beta(const std::array<double, N>& x, double sum){
    double ret = 1;
    for(double i : x){
        ret *= tgamma(i);
    }
    return ret / tgamma(sum);
}
template<std::size_t N>
static double log_multivariate_beta(const std::array<double, N>& x){
    double ret = 0;
    double sum = 0;
    for(double i : x){
        ret += lgamma(i);
        sum += i;
    }
    return ret - lgamma(sum);
}
template<std::size_t N>
static double log_multivariate_beta(const std::array<double, N>& x, double sum){
    double ret = 0;
    for(double i : x){
        ret += lgamma(i);
    }
    return ret - lgamma(sum);
}

/**************************高次元座標の一列化**************************/

unsigned int cartesian(unsigned int x, unsigned int y){
    unsigned int sum = x + y;
    unsigned int base = sum * (sum + 1) / 2;
    //return (sum & 1) ? (base + x) : (base + sum - x);
    return base + x;
}

#endif // UTIL_MATH_HPP_




