#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  return Wrap32 { static_cast<uint32_t>( zero_point.raw_value_ + n ) };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t rem = raw_value_ - zero_point.raw_value_;
  if ( rem > checkpoint ) {
    return rem;
  }
  uint64_t k = ( checkpoint - rem ) >> 32;
  uint64_t l = ( k << 32 ) + rem, r = ( ( k + 1 ) << 32 ) + rem;
  return checkpoint - l < r - checkpoint ? l : r;
}
