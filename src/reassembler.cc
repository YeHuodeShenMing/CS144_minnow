#include "reassembler.hh"
// #include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  if (is_last_substring) {
    eof_ = true;
    last_index_ = max(last_index_, first_index + data.length());
  }

  uint64_t _first_unassembled = output_.writer().bytes_pushed();
  uint64_t _first_unacceptable = _first_unassembled + output_.writer().available_capacity();

  // 初始化缓冲区的大小
  if ( _buffer.size() < _first_unacceptable - _first_unassembled ) {
    _buffer.resize( _first_unacceptable - _first_unassembled, "" );
    flag_.resize( _first_unacceptable - _first_unassembled, false );
  }

  // 超出范围的数据直接丢弃
  if ( first_index >= _first_unacceptable || first_index + data.length() < _first_unassembled ) {
    return;
  }

  // 裁剪
  uint64_t begin = max( _first_unassembled, first_index );
  uint64_t end = min( _first_unacceptable, first_index + data.length() );

  // cout << "DEBUG: initialization = " << first_index << " " << data << " " << is_last_substring << endl;
  // cout << "DEBUG: _first_unassembled = " << _first_unassembled << endl;
  // cout << "DEBUG: last_index_ = " << last_index_ << endl;

  for ( uint64_t i = begin; i < end; i++ ) {
    if ( !flag_[i - _first_unassembled] ) {
      // cout << "DEBUG: end = " << end << endl;
      flag_[i - _first_unassembled] = true;
      _buffer[i - _first_unassembled] = data[i - first_index];
      unassembled_byte_ += 1;
      // cout << "DEBUG: 111 bytes_pending = " << bytes_pending() << endl;
      // cout << "DEBUG: 111 bytes_pushed = " << output_.writer().bytes_pushed() << endl;
    }
  }

  // 输出连续流
  string wait_str = "";

  while ( !flag_.empty() && flag_.front() ) {
    wait_str += _buffer.front();

    // cout << "DEBUG: wait_str = " << wait_str << endl;

    _buffer.pop_front();
    flag_.pop_front();

    _buffer.push_back( "" );
    flag_.push_back( false );
  }

  // 如果有连续字节流，写入输出流
  if ( !wait_str.empty() ) {
    output_.writer().push( wait_str );
    if ( unassembled_byte_ >= wait_str.length() ) {
      unassembled_byte_ -= wait_str.length();
    } else {
      unassembled_byte_ = 0;
    }
    // cout << "DEBUG: 222 bytes_pending = " << bytes_pending() << endl;
    // cout << "DEBUG: 222 bytes_pushed = " << output_.writer().bytes_pushed() << endl;
  }

  // cout << "DEBUG: 3333 _eof = " << _eof << endl;


  // 标记输入结束, 放到这一轮输入的最后
  if ( eof_ && unassembled_byte_ == 0 && output_.writer().bytes_pushed() >= last_index_) {
    // cout<<"INININ";
    output_.writer().close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unassembled_byte_;
}
