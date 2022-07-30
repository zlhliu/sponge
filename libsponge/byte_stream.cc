#include "byte_stream.hh"
#include<iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {this->ByteStream::max_len=capacity;}

size_t ByteStream::write(const string &data) {
    size_t new_len = byte_stream.size() + data.size();
    size_t byte_accepted = 0;
    if (new_len > max_len) {
        byte_accepted = max_len - byte_stream.size();
        string temp = data.substr(0, (max_len - byte_stream.size()));
        byte_stream += temp;
    } else {
        byte_stream += data;
        byte_accepted = data.size();
    }
    has_written += byte_accepted;
    return byte_accepted;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string ans;
    if (len > byte_stream.size()) {
        ans = byte_stream.substr(0, byte_stream.size());
    } else {
        ans = byte_stream.substr(0, len);
    }
    return ans;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    if (len > byte_stream.size()) {
        has_read += byte_stream.size();
        byte_stream.erase(0, byte_stream.size());
    } else {
        byte_stream.erase(0, len);
        has_read += len;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    string ans;
    if (len > byte_stream.size()) {
        ans=peek_output(byte_stream.size());
        pop_output(byte_stream.size());
    } else {
        ans=peek_output(len);
        pop_output(len);
    }
    return ans;
}

void ByteStream::end_input() { this->is_end = true; }

bool ByteStream::input_ended() const { return this->is_end; }

size_t ByteStream::buffer_size() const { return byte_stream.size(); }

bool ByteStream::buffer_empty() const { return byte_stream.size() == 0; }

bool ByteStream::eof() const { return is_end && (byte_stream.size() == 0); }

size_t ByteStream::bytes_written() const { return has_written; }

size_t ByteStream::bytes_read() const { return has_read; }

size_t ByteStream::remaining_capacity() const { return (max_len - byte_stream.size()); }

size_t ByteStream::size() const{
    return byte_stream.size();
}
