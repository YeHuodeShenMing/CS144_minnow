#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  return Wrap32 { static_cast<uint32_t>( n ) + zero_point.raw_value_ };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  Wrap32 checkpoint_wrap32 = wrap( checkpoint, zero_point );
  uint32_t delta = raw_value_ - checkpoint_wrap32.raw_value_;
  uint64_t result = checkpoint + static_cast<uint64_t>( delta );

  // offset > (1u << 31) 检查 offset 是否大于 2^31（即，检查是否跨越了 32 位整数的中点，导致环绕）。
  // result >= (1ul << 32) 检查解包后的结果是否大于或等于 2^32，即检查它是否超出了 32 位无符号整数的最大值。
  if ( ( delta > ( 1U << 31 ) ) && ( result >= ( 1UL << 32 ) ) ) {
    result -= ( 1UL << 32 );
  }
  return result;
}
