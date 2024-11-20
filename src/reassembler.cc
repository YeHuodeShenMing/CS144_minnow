#include "reassembler.hh"

using namespace std;

// Reassembler::Reassembler(uint64_t capacity) : capacity_(capacity), _input_ended(false), unassembled_byte(0) {};

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // Your code here.
  uint64_t _first_unassembled = output_.bytes_pushed();
  uint64_t _first_unacceptable = _first_unassembled + capacity_;

  // 超出范围的数据直接丢弃
  if (first_index >= _first_unacceptable || first_index + data.length() < _first_unassembled) {
    return;
  }

  // 裁剪
  uint64_t begin = max(_first_unassembled, first_index);
  uint64_t end = min(_first_unacceptable, first_index + data.length());

  for (uint64_t i = begin; i < end; i++) {
    if (!_flag[i - _first_unassembled]) {
      _flag[i - _first_unassembled] = true;
      _buffer[i - _first_unassembled] = data[i - first_index];
      unassembled_byte += 1;
    }
  }

  // 输出连续流
  string wait_str = "";

  while (_flag.front()) {
    wait_str += _buffer.front();
    _buffer.pop_front();
    _flag.pop_front();

    _buffer.push_back("");
    _flag.push_back(false);
  }

  // 如果有连续字节流，写入输出流
  if (wait_str.length() > 0) {
    output_.push(wait_str);
    unassembled_byte -= wait_str.length();
  }

  // 标记输入结束
  if (is_last_substring) {
    output_.close();
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unassembled_byte;
}


