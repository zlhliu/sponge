#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity)
    , _rto(retx_timeout) {}
uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }


void TCPSender::send_segment(TCPSegment &seg) {
    // TCP报文头部根据ask写入待发送的首位序列号
    seg.header().seqno = wrap(_next_seqno, _isn);
    // std::cout<<seg.payload().str();
    _next_seqno += seg.length_in_sequence_space();
    _bytes_in_flight += seg.length_in_sequence_space();

    // 如果不是sym包，则修改发送方剩余容量
    if (_syn_sent)
         _receiver_free_space -= seg.length_in_sequence_space();

    // 待发送seg
    _segments_out.push(seg);
    // 待发送seg，未接受ack?
    _segments_outstanding.push(seg);
    if (!_timer_running) {
        _timer_running = true;
        _time_elapsed = 0;
    }
}

void TCPSender::fill_window() {
    // 还没发syn时
    if (!_syn_sent) {
        _syn_sent = true;
        TCPSegment seg;
        seg.header().syn = true;
        send_segment(seg);
        return;
    }

    // 有将发送的seg，但syn未收到ack
    if (!_segments_outstanding.empty() && _segments_outstanding.front().header().syn)
        return;
    
    // 没有需要发送的文字
    if (!_stream.buffer_size() && !_stream.eof())
        return;
        
    // fin已经发出
    if (_fin_sent)
        return;

    
    if (_receiver_window_size) {
        // 如果滑动窗口不为0
        while (_receiver_free_space) {
            TCPSegment seg;
            size_t payload_size = min({_stream.buffer_size(),
                                    static_cast<size_t>(_receiver_free_space),
                                    static_cast<size_t>(TCPConfig::MAX_PAYLOAD_SIZE)});
            seg.payload() = _stream.read(payload_size);

            // eof未发射且可以把全部的内容全部放入负载，所以fin置1
            if (_stream.eof() && static_cast<size_t>(_receiver_free_space) > payload_size) {
                seg.header().fin = true;
                _fin_sent = true;
            }
            send_segment(seg);

            // 内容全部发送完
            if (_stream.buffer_empty())
                break;
        }
    } else if (_receiver_free_space == 0) {
        // The zero-window-detect-segment should only be sent once (retransmition excute by tick function).
        // Before it is sent, _receiver_free_space is zero. Then it will be -1.
        TCPSegment seg;
        if (_stream.eof()) {
            seg.header().fin = true;
            _fin_sent = true;
            send_segment(seg);
        } else if (!_stream.buffer_empty()) {
            seg.payload() = _stream.read(1);
            send_segment(seg);
        }
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size


void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _next_seqno);

    _receiver_window_size = window_size;
    _receiver_free_space = window_size;

    // 有发送但未接受ack的seg，则检查是否有需要pop的(因为此时接收到ack了)
    while (!_segments_outstanding.empty()) {
        // 等待ack的首位
        TCPSegment seg = _segments_outstanding.front();
        TCPSegment seg_back= _segments_outstanding.back();
        // 如果接收到的ack大于等于首位seg的seqno则确认接受，并将首位pop
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno &&
        abs_ackno<=(unwrap(seg_back.header().seqno, _isn, _next_seqno) + seg_back.length_in_sequence_space())) {
            // 传输中的字节减少
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
            // Do not do the following operations outside while loop.
            // Because if the ack is not corresponding to any segment in the segment_outstanding,
            // we should not restart the timer.
            _time_elapsed = 0;
            _rto = _initial_retransmission_timeout;
            _consecutive_retransmissions = 0;
        } else {
            break;
        }
    }

    // 仍有剩余未收到
    if (!_segments_outstanding.empty()) {
        // 接收方剩余空间为：未发的-(队列中首个未确认的序列号-需要发送的序列号)-传输中的字节数
        // 这是因为syn和fin也占空间，所以要做调整
        _receiver_free_space = static_cast<uint16_t>(
            abs_ackno + static_cast<uint64_t>(window_size) -
            unwrap(_segments_outstanding.front().header().seqno, _isn, _next_seqno) - _bytes_in_flight);
    }

    // 没有传输中的字节了，则关闭计时器
    if (!_bytes_in_flight)
        _timer_running = false;
    // Note that test code will call it again.
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // 计时器没开时，结束
    if (!_timer_running){
        // _rto=_initial_retransmission_timeout;
        return;
    }
    
    _time_elapsed += ms_since_last_tick;
    // 时间大于rto时，触发重传
    if (_time_elapsed >= _rto) {
        // 放入待发送队列
        _segments_out.push(_segments_outstanding.front());
        
        // syn未收到ack或者
        if (_receiver_window_size || _segments_outstanding.front().header().syn) {
            ++_consecutive_retransmissions;
            _rto <<= 1;
        }
        // 已经重传，所以等待时间重置
        _time_elapsed = 0;
    }
 }

// unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

// void TCPSender::send_empty_segment() {}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
