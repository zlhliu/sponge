#include "tcp_receiver.hh"
#include <iostream>
// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    TCPHeader head=seg.header();
    // 是否为syn包
    if(head.syn){_isn=head.seqno+1;_isn_set=true;}
    // syn包来之前，为无效输入
    if(!_isn_set)return;
    // 判断是否为最后的segment
    bool eof=head.fin;
    uint64_t index=unwrap(head.seqno,_isn,_checkpoint);
    _reassembler.push_substring(seg.payload().copy(),index,eof);
    _checkpoint=_reassembler.stream_out().buffer_size();
    cout<<_capacity;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(_isn_set==true)return wrap(_reassembler.stream_out().bytes_written(),_isn);
    else return {};
 }

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
