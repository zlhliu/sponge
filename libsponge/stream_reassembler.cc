#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // the map should be clear after output having be read
    if(_output.buffer_empty()){
        for(size_t i=_first_unread;i<_first_unassembled;i++){
            if(_map.count(i))_map.erase(i);
        }
        _first_unread=_cur_index;
        _first_unassembled=_first_unread;
    }
    if(eof==true){
        _total_cnt=index+data.size();
        _can_be_eof=true;
    }
    // i should be less than the num of data
    // i should be greater than the first_unread index
    // _map should be less than the capacity
    for(size_t i=max(index,_first_unread);i<index+data.size()&&_map.size()<=_capacity&&i<_first_unread+_capacity;i++){
        _map[i]=data[i-index];
    }
    while(_output.remaining_capacity()>0&&_map.count(_cur_index)){
        _output.write(_map[_cur_index]);
        _first_unassembled++;
        _cur_index++;
    }
    if(_can_be_eof&&_cur_index==_total_cnt){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    // because map has {-1,"-1"},this is useless
    return _map.size()-(_first_unassembled-_first_unread)-1; 
}

bool StreamReassembler::empty() const { return _map.size()-(_first_unassembled-_first_unread)==1; }
