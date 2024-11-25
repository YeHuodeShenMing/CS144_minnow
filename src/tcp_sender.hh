#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <queue>

class RetransmissionTimer
{
public:
  RetransmissionTimer( uint64_t initial_RTO_ms ) : RTO_( initial_RTO_ms ) {};
  bool is_expired() const noexcept { return is_active_ && time_passed_ >= RTO_; }
  bool is_active() const noexcept { return is_active_; }

  // 激活这个timer
  RetransmissionTimer& active() noexcept;

  // 超时翻倍
  RetransmissionTimer& timeout_double() noexcept;

  // 避免使用 stop() 函数，有需要就用直接使用 move 传递给新的对象
  // 这里只重置时间
  RetransmissionTimer& reset() noexcept;

  // 在timer中写好TCPSender的tick方法， TCPSender用的时候直接调
  RetransmissionTimer& tick( uint64_t initial_RTO_ms_ ) noexcept;

private:
  uint64_t RTO_;
  uint64_t time_passed_ { 0 };
  bool is_active_ {};
};

class TCPSender
{
public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( ByteStream&& input, Wrap32 isn, uint64_t initial_RTO_ms )
    : input_( std::move( input ) ), isn_( isn ), initial_RTO_ms_( initial_RTO_ms ), timer_( initial_RTO_ms ) {};

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage make_empty_message() const;

  /* Receive and process a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Type of the `transmit` function that the push and tick methods can use to send messages */
  using TransmitFunction = std::function<void( const TCPSenderMessage& )>;

  /* Push bytes from the outbound stream */
  void push( const TransmitFunction& transmit );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
  void tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit );

  // Accessors
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
  Writer& writer() { return input_.writer(); }
  const Writer& writer() const { return input_.writer(); }

  // Access input stream reader, but const-only (can't read from outside)
  const Reader& reader() const { return input_.reader(); }

private:
  // Variables initialized in constructor
  ByteStream input_;
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;

  // 新加的
  uint16_t wnd_size_ { 1 };       // 默认设置为 1
  uint64_t next_seqno_to_send {}; // 待发送的下一个字节
  uint64_t acked_seqno {};        // Sender 待确认的 第一个 ackno

  bool SYN_sent {}, FIN_sent {};

  RetransmissionTimer timer_;
  uint64_t retransmission_cnt_ { 0 };

  // 缓冲区 FIFO 算法
  std::queue<TCPSenderMessage> outstanding_messages_ {};
  uint64_t numbers_in_flight { 0 }; // ??? 缓冲区中字节长度
};
