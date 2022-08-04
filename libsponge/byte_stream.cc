#include "byte_stream.hh"
#include<iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {_max_len=capacity;}

size_t ByteStream::write(const string &data) {
    size_t new_len = _byte_stream.size() + data.size();
    size_t byte_accepted = 0;
    if (new_len > _max_len) {
        byte_accepted = _max_len - _byte_stream.size();
        string temp = data.substr(0, (_max_len - _byte_stream.size()));
        _byte_stream += temp;
    } else {
        _byte_stream += data;
        byte_accepted = data.size();
    }
    _has_written += byte_accepted;
    return byte_accepted;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ans;
    if (len > _byte_stream.size()) {
        ans = _byte_stream.substr(0, _byte_stream.size());
    } else {
        ans = _byte_stream.substr(0, len);
    }
    return ans;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > _byte_stream.size()) {
        _has_read += _byte_stream.size();
        _byte_stream.erase(0, _byte_stream.size());
    } else {
        _byte_stream.erase(0, len);
        _has_read += len;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ans;
    if (len > _byte_stream.size()) {
        ans=peek_output(_byte_stream.size());
        pop_output(_byte_stream.size());
    } else {
        ans=peek_output(len);
        pop_output(len);
    }
    return ans;
}

void ByteStream::end_input() { this->_is_end = true; }

bool ByteStream::input_ended() const { return this->_is_end; }

size_t ByteStream::buffer_size() const { return _byte_stream.size(); }

bool ByteStream::buffer_empty() const { return _byte_stream.size() == 0; }

bool ByteStream::eof() const { return _is_end && (_byte_stream.size() == 0); }

size_t ByteStream::bytes_written() const { return _has_written; }

size_t ByteStream::bytes_read() const { return _has_read; }

size_t ByteStream::remaining_capacity() const { return (_max_len - _byte_stream.size()); }

size_t ByteStream::size() const{
    return _byte_stream.size();
}
