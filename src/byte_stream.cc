#include "byte_stream.hh"
#include <string_view>
#include <iostream>

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
  for ( char c : data ) {
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
  char top_elem = '\0';
  top_elem = *_buffer.begin();
  cout << "this is top_elem" << top_elem;
  return string_view( &top_elem, 1 );
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  uint64_t pop_length = min( len, bytes_buffered() );
  while ( pop_length ) {
    _bytes_readed += 1;
    _buffer.pop_front();
    _remaining_capacity += 1;
    pop_length -= 1;
  }
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return capacity_ - _remaining_capacity;
}

