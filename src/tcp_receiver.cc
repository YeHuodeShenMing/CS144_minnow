#include "tcp_receiver.hh"

#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // Your code here.
  // RST 报异常
  if ( message.RST ) {
    reassembler_.reader().set_error();
    return;
  }

  // SYN 位置被占了，不能写入
  if ( message.seqno == ISN_ ) {
    return;
  }

  // 1 : SYN
  if ( !ISN_.has_value() && message.SYN ) {
    ISN_ = message.seqno;
  }

  // 2 : PayLoad
  uint64_t checkpoint = reassembler_.writer().bytes_pushed();
  uint64_t abs_seqno = message.seqno.unwrap( *ISN_, checkpoint );

  // cout << "FIN" << endl;
  reassembler_.insert( abs_seqno > 0 ? abs_seqno - 1 : 0, message.payload, message.FIN );
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  uint64_t checkpoint = ISN_.has_value() + reassembler_.writer().bytes_pushed() + reassembler_.writer().is_closed();
  uint64_t capacity = reassembler_.writer().available_capacity();
  TCPReceiverMessage result;

  if ( ISN_.has_value() ) {
    result.ackno = Wrap32::wrap( checkpoint, *ISN_ );
  }

  else {
    result.ackno = {};
  }

  result.window_size = capacity > UINT16_MAX ? UINT16_MAX : capacity;

  if ( reassembler_.reader().has_error() ) {
    result.RST = true;
  }

  return result;
}
