#include "wrapping_integers.hh"
#include <cmath>
#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    //ban type cast.
    uint32_t offset = n % (1ll << 32);
    return isn + offset;
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.

uint64_t private_abs(uint64_t a, uint64_t b) {
    if (a >= b) {
        return a - b;
    }
    return b - a;
}

uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {

    uint32_t inter = (n - isn);
    uint64_t res = inter;
    if (res > checkpoint) {
        return res;
    }
    uint64_t multiple = checkpoint >> 32;
    uint64_t lhs = res + ((multiple - 1) << 32);
    uint64_t mhs = res + (multiple << 32);
    uint64_t rhs = res + ((multiple + 1) << 32);
    if (private_abs(lhs, checkpoint) < private_abs(mhs, checkpoint) && private_abs(lhs, checkpoint) < private_abs(rhs, checkpoint)) {
        return lhs;
    }
    if (private_abs(mhs, checkpoint) < private_abs(lhs, checkpoint) && private_abs(mhs, checkpoint) < private_abs(rhs, checkpoint)) {
        return mhs;
    }
    if (private_abs(rhs, checkpoint) < private_abs(lhs, checkpoint) && private_abs(rhs, checkpoint) < private_abs(mhs, checkpoint)) {
        return rhs;
    }
    return res;
}
