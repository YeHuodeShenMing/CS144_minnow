#include "byte_stream.hh"
#include <iostream>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , _buffer()
  , _bytes_readed( 0 )
  , _bytes_written( 0 )
  , _bytes_buffer( 0 )
  , _input_ended( false )
{}

bool Writer::is_closed() const
{
  // Your code here.
  return _input_ended;
}

void Writer::push( string data )
{
  // Your code here.
  if ( available_capacity() == 0 || is_closed() || data.size() == 0 ) {
    return;
  }
  uint64_t push_len = min( data.size(), available_capacity() );

  if ( push_len < data.size() ) {
    data = data.substr( 0, push_len );
  }

  _buffer.push_back( data );

  _bytes_written += push_len;
  _bytes_buffer += push_len;
}

void Writer::close()
{
  // Your code here.
  _input_ended = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - _bytes_buffer;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return _bytes_written;
}

bool Reader::is_finished() const
{
  // Your code here.
  return _input_ended && ( _bytes_buffer == 0 );
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return _bytes_written - _bytes_buffer;
}

string_view Reader::peek() const
{
  // Your code here.
  if ( _buffer.empty() ) {
    return {};
  }
  return string_view( _buffer.front() );
}

void Reader::pop( uint64_t len )
{
  // 确保不超过缓冲区内可用数据量
  uint64_t pop_length = min( len, _bytes_buffer );

  // 循环处理要移除的数据
  while ( pop_length > 0 && !_buffer.empty() ) {
    auto& front_str = _buffer.front(); // 获取当前队列的前端元素
    auto sz = front_str.size();

    if ( pop_length < sz ) {
      // 只需要移除部分数据
      front_str.erase( 0, pop_length );
      _bytes_readed += pop_length;
      _bytes_buffer -= pop_length;
      return;
    }

    // 移除整个字符串
    _buffer.pop_front();

    _bytes_readed += sz;
    _bytes_buffer -= sz;

    pop_length -= sz; // 减少还需移除的长度
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return _bytes_buffer;
}
