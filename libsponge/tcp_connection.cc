#include "tcp_connection.hh"

#include <iostream>

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

bool TCPConnection::active() const { return _active; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    /**
    cout << "received A =" << seg.header().ack  << " S = " << seg.header().syn << \
         " F =" << seg.header().fin << " R = " << seg.header().rst << " seqno = " << \
         seg.header().seqno << " ackno = " << seg.header().ackno << endl;
    **/
    if (!active()) {
        return;
    }
    _time_since_last_segment_received = 0;

    // rst置位
    if (seg.header().rst) {
        // if (in_listen_state()) {
        //     return;
        // }
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        _active = false;
        return;
    }

    // 当seg没有syn并且没有ack时
    if (!seg.header().syn && !_receiver.ackno().has_value()) {
        return;
    }

    // 收到ack
    if (seg.header().ack) {
        // 接收方确认ackno和修改自己的win
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    _receiver.segment_received(seg);

    // fill_window的目的是得到几个可发送的报文段让后续可以捎带上ack, 顺便一提fill_window的操作还顺便把syn给发了
    // 提高效率的步骤
    _sender.fill_window(); 

    // 没有可发送的报文段，但依然需要及时ack
    // seg.length_in_sequence_space() > 0保证了不会对纯ack回应ack
    if (seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    // 发送应答，捎带上ackno(在这个函数里捎带)
    send_segments();
    try_clean_shutdown();
}

size_t TCPConnection::write(const string &data) {
    if (!active()) {
        return 0;
    }
    size_t len = _sender.stream_in().write(data);
    send_segments();
    try_clean_shutdown();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    // cout << "time pass " << ms_since_last_tick << "ms" << endl; 
    if (!active()) {
        return;
    }
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);

    if (_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
       unclean_shutdown();
    } else {
        send_segments();
        try_clean_shutdown();
    }
}


bool TCPConnection::try_clean_shutdown() {
    if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
        _linger_after_streams_finish = false;
    }
    // 确保对等层收到ack，不会一直发送seg
    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) {
        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false; // 就把自己关了
        }
    }
    return !_active;
}

void TCPConnection::unclean_shutdown() {
    // When this being called, _sender.stream_out() should not be empty.
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _active = false;

    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();
    seg.header().ack = true;
    if (_receiver.ackno().has_value())
        seg.header().ackno = _receiver.ackno().value();
    seg.header().win = _receiver.window_size();
    seg.header().rst = true;
    _segments_out.push(seg);
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();//发送
    _sender.fill_window();
    send_segments();
}

void TCPConnection::connect() {
    _sender.fill_window(); // 第一次fill_window的时候发SYN，第一次握手
    send_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            unclean_shutdown();
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

// 把segment从sender的发送队列中取出，扔到网络里(也就是放到_segments_out里)
void TCPConnection::send_segments() {
    TCPSegment seg;
    // 发送端队列非空时发送
    while (!_sender.segments_out().empty()) {
        // 取出第一个等待发送的消息
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();

        // 注意：为全双工，所以在发送内容的同时要反馈对方发送的内容
        // 加上ack和win_size
        // 当有接收端有信息时
        // 注意，不对ack回应ack
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }

        // unclean shutdown
        if (_sender.stream_in().error() || _receiver.stream_out().error()) {
            seg.header().rst = true;
        }

        _segments_out.push(seg);
    }
}