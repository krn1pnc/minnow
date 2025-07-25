#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  auto& w = output_.writer();

  uint64_t reasm_begin = w.bytes_pushed(), reasm_end = reasm_begin + w.available_capacity();
  if ( first_index + data.size() < reasm_begin || first_index >= reasm_end ) {
    return;
  }

  if ( is_last_substring && first_index + data.size() <= reasm_end ) {
    received_last = true;
  }

  auto data_begin = max( first_index, reasm_begin );
  auto data_end = min( first_index + data.size(), reasm_end );
  auto it = buffer_.lower_bound( data_begin );
  if ( it != buffer_.begin() ) {
    auto pit = prev( it );
    data_begin = max( data_begin, pit->first + pit->second.size() );
  }

  while ( it != buffer_.end() && it->first + it->second.size() <= data_end ) {
    buffer_.erase( it++ );
  }
  if ( it != buffer_.end() && it->first < data_end ) {
    buffer_[data_end] = it->second.substr( data_end - it->first );
    buffer_.erase( it );
  }
  if ( data_begin - first_index < data.size() ) {
    buffer_[data_begin] = data.substr( data_begin - first_index, data_end - data_begin );
  }

  while ( !buffer_.empty() && w.bytes_pushed() == buffer_.begin()->first ) {
    w.push( buffer_.begin()->second );
    buffer_.erase( buffer_.begin() );
  }

  if ( received_last && buffer_.empty() ) {
    w.close();
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  auto res = 0;
  for ( auto [k, v] : buffer_ ) {
    res += v.size();
  }
  return res;
}
