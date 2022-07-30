#include "wrapping_integers.hh"

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
    uint32_t ans=0;
    ans=static_cast<uint32_t>(n%4294967296UL)+isn.raw_value();
    return WrappingInt32{ans};
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
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    uint64_t range1=checkpoint/4294967296UL;
    uint64_t range2=range1+1;
    uint64_t ret=1;
    uint64_t ans1=range1*4294967296UL-isn.raw_value();
    uint64_t ans2=range2*4294967296UL-isn.raw_value();

    ans1+=n.raw_value();

    ans2+=n.raw_value();

    if(ans1>=checkpoint)ret=ans1;
    else if(checkpoint-ans1<ans2-checkpoint)ret=ans1;
    else if(checkpoint-ans1>=ans2-checkpoint) ret=ans2;

    return ret;
}
