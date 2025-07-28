#include "tcp_sender.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return outstanding_count_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmission_count_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  auto& r = reader();
  uint64_t window_size = !window_size_ ? 1 : window_size_;
  while ( outstanding_count_ < window_size ) {
    if ( FIN_sent_ ) {
      break;
    }

    TCPSenderMessage msg;

    msg.seqno = Wrap32::wrap( bytes_sent_, isn_ );

    if ( !r.has_error() ) {
      if ( !SYN_sent_ ) {
        msg.SYN = true;
        SYN_sent_ = true;
      }

      string payload;
      uint64_t max_size = min( window_size - outstanding_count_ - msg.SYN, TCPConfig::MAX_PAYLOAD_SIZE );
      while ( r.bytes_buffered() && payload.size() < max_size ) {
        auto view = r.peek();
        view = view.substr( 0, max_size - payload.size() );
        payload += view;
        r.pop( view.size() );
      }
      msg.payload = move( payload );

      if ( r.is_finished() && outstanding_count_ + msg.sequence_length() + 1 <= window_size ) {
        msg.FIN = true;
        FIN_sent_ = true;
      }

      if ( !msg.sequence_length() ) {
        break;
      }

      if ( !timer_active_ ) {
        timer_active_ = true;
      }
      transmit( msg );
      bytes_sent_ += msg.sequence_length();
      outstanding_count_ += msg.sequence_length();
      outstanding_message_.emplace( move( msg ) );
    } else {
      msg.RST = true;
      transmit( msg );
      break;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  return {
    Wrap32::wrap( bytes_sent_, isn_ ),
    false,
    "",
    false,
    reader().has_error(),
  };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  if ( msg.RST ) {
    reader().set_error();
    return;
  }

  window_size_ = msg.window_size;

  if ( !msg.ackno.has_value() ) {
    return;
  }

  bool acked_any = false;
  uint64_t acked_seqno = msg.ackno.value().unwrap( isn_, bytes_sent_ );
  if ( acked_seqno > bytes_sent_ ) {
    return;
  }
  while ( !outstanding_message_.empty() ) {
    auto& oldest_message = outstanding_message_.front();
    if ( oldest_message.seqno.unwrap( isn_, bytes_sent_ ) + oldest_message.sequence_length() <= acked_seqno ) {
      acked_any = true;
      outstanding_count_ -= oldest_message.sequence_length();
      outstanding_message_.pop();
    } else {
      break;
    }
  }
  if ( acked_any ) {
    time_oldest_message_ = 0;
    current_RTO_ms_ = initial_RTO_ms_;
    retransmission_count_ = 0;
    if ( outstanding_message_.empty() ) {
      timer_active_ = false;
    }
  }
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if ( !timer_active_ ) {
    return;
  }
  time_oldest_message_ += ms_since_last_tick;
  if ( time_oldest_message_ >= current_RTO_ms_ ) {
    transmit( outstanding_message_.front() );
    time_oldest_message_ = 0;
    if ( window_size_ ) {
      current_RTO_ms_ *= 2;
      retransmission_count_ += 1;
    }
  }
}
