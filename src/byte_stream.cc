#include "byte_stream.hh"
#include <iostream>
#include <string_view>

using namespace std;

ByteStream::ByteStream( uint64_t capacity )
  : capacity_( capacity )
  , _buffer()
  , _remaining_capacity( capacity )
  , _bytes_readed( 0 )
  , _bytes_written( 0 )
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
  for ( const auto& c : data ) {
    if ( _remaining_capacity > 0 ) {
      _buffer.push_back( c );
      _remaining_capacity -= 1;
      _bytes_written += 1;
    }
  }
}

void Writer::close()
{
  // Your code here.
  _input_ended = true;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return _remaining_capacity;
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return _bytes_written;
}

bool Reader::is_finished() const
{
  // Your code here.
  return _input_ended && ( _remaining_capacity == capacity_ );
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return _bytes_written - capacity_ + _remaining_capacity;
}

string_view Reader::peek() const
{
  // Your code here.
  if ( _buffer.empty() ) {
    return string_view();
  }
  return string_view( &_buffer.front(), 1 );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t pop_length = min( len, capacity_ - _remaining_capacity );
  _bytes_readed += pop_length;
  _remaining_capacity += pop_length;
  while ( pop_length ) {
    _buffer.pop_front();
    pop_length -= 1;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return capacity_ - _remaining_capacity;
}
