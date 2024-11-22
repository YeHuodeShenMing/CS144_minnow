#include "reassembler.hh"
// #include <iostream>

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  seg segment(first_index, data);
  first_unacceptable_ = first_unassembled_ + output_.writer().available_capacity(); 

  // 不接受新的stream
  if ( output_.writer().available_capacity() == 0 || output_.writer().is_closed()) {
    return;
  }

  if (is_last_substring) {
    end_index_ = segment.last_index_;
  }

  // 数据完全在区间外
  if (segment.first_index_ > first_unacceptable_ || segment.last_index_ < first_unassembled_) {
    return;
  }

  // 裁剪
  cut_seg(segment);
  // 插入
  insert_seg(segment);
  // 写出
  write_seg();
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return unassembled_byte_;
}

void Reassembler::write_seg() {
  // cout <<"DEBUG: Write"  <<endl;
  auto it = segments_.begin();

  // if (segments_.empty()) {
  //   cout <<"DEBUG: empty"  <<endl;
  // }

  // 没有能输出的从0开始连续stream
  if (segments_.empty() || it->first_index_ != first_unassembled_) return;

  // cout <<"DEBUG: " << output_.writer().bytes_pushed() <<endl;

  output_.writer().push(move(it->data_));
  // cout <<"DEBUG: After" << output_.writer().bytes_pushed() <<endl;

  unassembled_byte_ -= it->data_.size();
  first_unassembled_ = it->last_index_;
  segments_.erase(it);

  // 若最后一个segment
  if (first_unassembled_ == end_index_) {
    output_.writer().close();
  } 
}

void Reassembler::cut_seg(seg& segment) {
  // cout <<"DEBUG: CUT"  <<endl;
  // cout <<"DEBUG: segment.last_index_"  << segment.last_index_ <<endl;
  // cout <<"DEBUG: unassembled_byte_"  << unassembled_byte_ <<endl;
  if (segment.first_index_ < first_unassembled_) {
    

    // substr是取 参数 到 结尾 的字串
    segment.data_ = segment.data_.substr(first_unassembled_ - segment.first_index_);
    segment.first_index_ = first_unassembled_;
  }

  if (segment.last_index_ > first_unacceptable_) {
    // cout <<"DEBUG: CUT larger"  <<endl;
    // erase是取 开头 到 参数 的字串 ，删除后面的
    segment.data_ = segment.data_.erase(first_unacceptable_ - segment.first_index_);
    segment.last_index_ = first_unacceptable_;
  }
}

void Reassembler::insert_seg(seg& segment) {
  // cout <<"DEBUG: Insert : " <<segment.data_ <<endl;
  // lower_bound 返回第一个匹配上segment的segments_元素的 指针
  auto it = segments_.lower_bound(segment);

  // 向前移动一个区间，检查前一个区间是否也有重叠
  // 主要是针对 segment 的位置判断
  if (it != segments_.begin() && prev(it)->last_index_ >= segment.first_index_) {
    it --;
  }

  while (it != segments_.end() && it->first_index_ <= segment.last_index_) {
    // cout <<"DEBUG: In case it = : " <<segment.data_ <<endl;
    // Case 1 : 新区间与起点在自身之前的旧区间部分重叠
    if (it->first_index_ < segment.first_index_) {
      // cout <<"DEBUG: In case 1 : " <<segment.data_ <<endl;
      segment.data_ = it->data_.substr(0, segment.first_index_ - it->first_index_) + segment.data_;
      segment.first_index_ = it->first_index_;
    }

    // Case 3: 新区间与起点在自身之后的旧区间部分重叠
    if (it->last_index_ > segment.last_index_) {
      // cout <<"DEBUG: In case 2: " <<segment.data_ <<endl;
      segment.data_ += it->data_.substr(segment.last_index_ - it->first_index_);
      segment.last_index_ = it->last_index_;
    }

    // Case 2: 新区间完全覆盖已有区间
    // 进行后处理, 删除覆盖的区间
    unassembled_byte_ -= it->data_.size();
    it = segments_.erase(it);
  }

  // cout <<"DEBUG: After segment : " <<segment.data_ <<endl;
  segments_.insert(segment);
  unassembled_byte_ += segment.data_.size();

  // cout <<"DEBUG: unassembled_byte_ " << unassembled_byte_ <<endl;
  // // cout <<"DEBUG: After " << output_.writer().bytes_pushed() <<endl;
  // cout <<"DEBUG: segment " << segment.data_ <<endl;
  // cout <<"DEBUG: segment_.size " << segments_.size() <<endl;
  // cout <<"DEBUG: segments_.begin()->data_ " << segments_.begin()->data_ <<endl;
}