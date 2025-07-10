#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), buffer_( make_shared<char[]>( capacity ) ) {}

void Writer::push( string data )
{
  if ( data.size() > available_capacity() ) {
    data.resize( available_capacity() );
  }

  uint64_t tail_len = ( writep_ < readp_ ? readp_ : capacity_ ) - writep_;
  if ( data.size() <= tail_len ) {
    copy( data.begin(), data.end(), buffer_.get() + writep_ );
  } else {
    copy( data.begin(), data.begin() + tail_len, buffer_.get() + writep_ );
    copy( data.begin() + tail_len, data.end(), buffer_.get() );
  }

  writep_ += data.size();
  if ( writep_ >= capacity_ ) {
    writep_ -= capacity_;
  }
  writec_ += data.size();
}

void Writer::close()
{
  closed_ = true;
}

bool Writer::is_closed() const
{
  return closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_ - ( writec_ - readc_ );
}

uint64_t Writer::bytes_pushed() const
{
  return writec_;
}

string_view Reader::peek() const
{
  if ( !bytes_buffered() ) {
    return string_view();
  } else {
    int tail_len = ( readp_ < writep_ ? writep_ : capacity_ ) - readp_;
    return string_view( buffer_.get(), capacity_ ).substr( readp_, tail_len );
  }
}

void Reader::pop( uint64_t len )
{
  len = min( len, bytes_buffered() );

  readp_ += len;
  if ( readp_ >= capacity_ ) {
    readp_ -= capacity_;
  }
  readc_ += len;
}

bool Reader::is_finished() const
{
  return closed_ && !bytes_buffered();
}

uint64_t Reader::bytes_buffered() const
{
  return writec_ - readc_;
}

uint64_t Reader::bytes_popped() const
{
  return readc_;
}
