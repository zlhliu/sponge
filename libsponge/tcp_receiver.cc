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
    WrappingInt32 true_seqno=WrappingInt32(head.seqno);
    // 是否为syn包
    if(head.syn){_isn=head.seqno+1;_isn_set=true;true_seqno=true_seqno+1;}
    // syn包来之前，为无效输入
    if(!_isn_set)return;
    // 判断是否为最后的segment
    bool eof=head.fin;
    uint64_t index=unwrap(true_seqno,_isn,_checkpoint);
    _reassembler.push_substring(seg.payload().copy(),index,eof);
    _checkpoint=_reassembler.stream_out().buffer_size();
    cout<<"capacity:"<<_capacity<<endl;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(_isn_set==true){
        // isn和fin都是占序列号的
        uint64_t absolute_seq=_reassembler.stream_out().bytes_written();
        // 加上fin
        if(_reassembler.stream_out().input_ended()){
            return wrap(absolute_seq,_isn)+1;
        }
        else{
            return wrap(absolute_seq,_isn);
        }
    }
    else return {};
 }

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
