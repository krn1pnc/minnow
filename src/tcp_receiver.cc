#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if ( message.SYN ) {
    message.seqno = message.seqno + 1;
    byte_start_no_ = message.seqno;
  }
  if ( message.RST ) {
    byte_start_no_.reset();
  }
  if ( !byte_start_no_.has_value() ) {
    reassembler_.reader().set_error();
    return;
  }
  reassembler_.insert( message.seqno.unwrap( byte_start_no_.value(), reassembler_.writer().bytes_pushed() ),
                       message.payload,
                       message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  auto w = reassembler_.writer();
  TCPReceiverMessage res;
  if ( w.has_error() ) {
    res.RST = true;
  }
  if ( byte_start_no_.has_value() ) {
    res.ackno = Wrap32::wrap( w.bytes_pushed() + w.is_closed(), byte_start_no_.value() );
  }
  res.window_size = min<uint64_t>( w.available_capacity(), UINT16_MAX );
  return res;
}
