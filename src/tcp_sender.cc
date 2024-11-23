#include "tcp_sender.hh"
#include "tcp_config.hh"

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
  // 1. 新建长度为1的连接请求报文,在已有的num_in_flight之后
  TCPSenderMessage msg { make_empty_message() };
  if ( !SYN_sent ) {
    SYN_sent = true;
    msg.SYN = true;
  }

  // 窗口够就一直发
  while ( ( wnd_size_ == 0 ? 1 : wnd_size_ ) > numbers_in_flight
          || msg.payload.size() <= TCPConfig::MAX_PAYLOAD_SIZE ) {
    if ( FIN_sent ) {
      break;
    }
    const uint64_t remaining { ( wnd_size_ == 0 ? 1 : wnd_size_ ) - numbers_in_flight };
    // len表示 payload没满，剩下的字节可以继续填后面的数据
    const uint64_t len = min( TCPConfig::MAX_PAYLOAD_SIZE, remaining - msg.sequence_length() );

    // 将msg往payload填充
    auto&& new_payload {msg.payload};

    while (this->reader().bytes_buffered() != 0 && new_payload.size() < len) {
      string_view new_payload_view = this->reader().peek(); // peek观星下一段
      //len 表示这个报文段允许的最大有效负载大小。
      //new_payload.size() 是已经填充到 msg.payload 中的数据量。

      // 相减得到还能再插入的新的 字节长度
      new_payload_view = new_payload_view.substr(0, len - new_payload.size());
      msg.payload += new_payload_view;
      input_.reader().pop(new_payload_view.size());
    }

    if (!msg.FIN && this->reader().is_finished()) {
      msg.FIN = true;
      FIN_sent = true;
    }

    // 发送
    if (msg.sequence_length() == 0) {
      return;
    }

    if (!timer_.is_active()) {
      timer_.active();
    }
    transmit(msg);
    next_seqno_to_send += msg.sequence_length();
    numbers_in_flight += msg.sequence_length();
    // emplace: 在 outstanding_message_ 中 直接构造 一个 MessageType 对象
    outstanding_messages_.emplace(move(msg));
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
  if ( !msg.ackno.has_value() || input_.has_error() ) {
    return;
  }

  if ( msg.RST ) {
    input_.set_error();
  }

  wnd_size_ = msg.window_size;
  const uint64_t recv_abs_seqno = msg.ackno->unwrap( isn_, next_seqno_to_send );

  // 返回的ackno过大， 超出Sender预期的seqno
  if ( recv_abs_seqno > next_seqno_to_send ) {
    return;
  }

  bool has_acknowledge { false };

  while ( !outstanding_messages_.empty() ) {
    auto& buffered_msg = outstanding_messages_.front(); // 取待确认的第一个segment

    if ( recv_abs_seqno <= acked_seqno + buffered_msg.sequence_length() ) {
      break;
    }

    // 正常确认 buffered_msg
    has_acknowledge = true;
    acked_seqno += buffered_msg.sequence_length();
    outstanding_messages_.pop();
    numbers_in_flight -= buffered_msg.sequence_length();
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
    }
  }
}
