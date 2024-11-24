#include "tcp_sender.hh"
#include "tcp_config.hh"

// #include <iostream>

using namespace std;

// Timer
RetransmissionTimer& RetransmissionTimer::active() noexcept
{
  this->is_active_ = true;
  return *this;
};

RetransmissionTimer& RetransmissionTimer::timeout_double() noexcept
{
  RTO_ *= 2;
  return *this;
};

RetransmissionTimer& RetransmissionTimer::reset() noexcept
{
  time_passed_ = 0;
  return *this;
};

// 计时函数
RetransmissionTimer& RetransmissionTimer::tick( uint64_t initial_RTO_ms_ ) noexcept
{
  time_passed_ += is_active_ ? initial_RTO_ms_ : 0;
  return *this;
};

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return numbers_in_flight;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retransmission_cnt_;
}

void TCPSender::push( const TransmitFunction& transmit )
{
  // Your code here.

  // 窗口够就一直发
  while ( ( wnd_size_ == 0 ? 1 : wnd_size_ ) > numbers_in_flight ) {
    // 1. 新建长度为1的连接请求报文,在已有的num_in_flight之后
    TCPSenderMessage msg { make_empty_message() };
    if ( !SYN_sent ) {
      SYN_sent = true;
      msg.SYN = true;
    }

    if ( FIN_sent ) {
      break;
    }
    const uint64_t remaining { ( wnd_size_ == 0 ? 1 : wnd_size_ ) - numbers_in_flight };
    // cout << "remaining : " << (remaining ) << endl;
    // len表示 payload没满，剩下的字节可以继续填后面的数据
    const uint64_t len = min( TCPConfig::MAX_PAYLOAD_SIZE, remaining - msg.sequence_length() );

    // 将msg往payload填充
    auto&& new_payload { msg.payload };

    while ( this->reader().bytes_buffered() != 0 && new_payload.size() < len ) {
      string_view new_payload_view = this->reader().peek(); // peek观星下一段
      // len 每次填充的报文段的最大长度
      // new_payload.size() 是已经填充到 msg.payload 中的数据量。

      // 相减得到还能再插入的新的 字节长度
      new_payload_view = new_payload_view.substr( 0, len - new_payload.size() );
      msg.payload += new_payload_view;
      input_.reader().pop( new_payload_view.size() );
    }
    
    // cout << "msg.sequence_length() : " << (msg.sequence_length() ) << endl;
    // cout << "remaining > msg.sequence_length() ??? " << (remaining > msg.sequence_length()) << endl;
    // 窗口有空余位置再加FIN，否则不加
    if ( !msg.FIN && this->reader().is_finished() && remaining > msg.sequence_length()) {
      // cout << "I am here" << endl;
      msg.FIN = true;
      FIN_sent = true;
    }

    // 发送
    if ( msg.sequence_length() == 0 ) {
      return;
    }

    if ( !timer_.is_active() ) {
      timer_.active();
    }
    transmit( msg );
    next_seqno_to_send += msg.sequence_length();
    // cout << "Before : numbers_in_flight : " << numbers_in_flight << endl;
    numbers_in_flight += msg.sequence_length();
    // cout << "After : numbers_in_flight : " << numbers_in_flight << endl;
    // emplace: 在 outstanding_message_ 中 直接构造 一个 MessageType 对象
    outstanding_messages_.emplace( move( msg ) );
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  // Your code here.
  return { Wrap32::wrap( next_seqno_to_send, isn_ ), false, {}, false, input_.has_error() };
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.

  if ( input_.has_error() ) {
    return;
  }
  // cout << "RECEIVE" <<endl;

  if ( msg.RST ) {
    input_.set_error();
  }

  wnd_size_ = msg.window_size;
  // 没有ackno也得接收 窗口大小，等待下一次传输
    if ( !msg.ackno.has_value() ) {
    return;
  }
  const uint64_t recv_abs_seqno = msg.ackno->unwrap( isn_, next_seqno_to_send );

  // 返回的ackno过大， 超出Sender预期的seqno
  if ( recv_abs_seqno > next_seqno_to_send ) {
    return;
  }

  bool has_acknowledge { false };
  // cout << "outstanding_messages_.size : " << outstanding_messages_.size() << endl;
  while ( !outstanding_messages_.empty() ) {

    auto& buffered_msg = outstanding_messages_.front(); // 取待确认的第一个segment
    // cout << " buffered_msg: " << buffered_msg.sequence_length() << endl;
    // cout << " buffered_msg syn: " << buffered_msg.SYN << endl;

    if ( recv_abs_seqno < acked_seqno + buffered_msg.sequence_length() ) {
      break;
    }

    // cout << "IN" << endl;
    // 正常确认 buffered_msg
    has_acknowledge = true;
    acked_seqno += buffered_msg.sequence_length();

    // cout << "outstanding_messages_.size() : " << outstanding_messages_.size() << endl;
    // cout << "receive before : numbers_in_flight : " << numbers_in_flight << endl;
    // cout << "oops" << endl;
    numbers_in_flight -= buffered_msg.sequence_length();
    outstanding_messages_.pop();
    // cout << "receive after : numbers_in_flight : " << numbers_in_flight << endl;
  }

  // 处理timer
  if ( has_acknowledge ) {
    retransmission_cnt_ = 0;
    // 如果缓冲区被清空，停止计时器( 即为不active() )，否则只重置计时器；
    if ( outstanding_messages_.empty() ) {
      timer_ = RetransmissionTimer( initial_RTO_ms_ );
    } else {
      timer_ = RetransmissionTimer( initial_RTO_ms_ ).active();
    }
  }
}

// 处理超时重传
void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  // Your code here.
  if ( timer_.tick( ms_since_last_tick ).is_expired() ) {
    if ( !outstanding_messages_.empty() ) {
      // 1. 重传队首元素
      transmit( outstanding_messages_.front() );

      // 2. RTO_翻倍
      if ( wnd_size_ != 0 ) {
        timer_.timeout_double();
      }

      // 3. 保证timer_被释放，不会内存泄露
      timer_.reset();
      retransmission_cnt_ += 1;
    }
  }
}
