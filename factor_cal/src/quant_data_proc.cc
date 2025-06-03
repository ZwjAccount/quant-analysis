#include "quant_data_proc.hh"

QuantDataProc& QuantDataProc::GetInstance() {
  static QuantDataProc instance;
  return instance;
}

QuantDataProc::~QuantDataProc() {}
QuantDataProc::QuantDataProc()
 : thread_flag_(true) {}

void QuantDataProc::InitSecurityMap(std::string codes, size_t queue_size) {
  std::stringstream ss(codes);
  std::string code;

  while (std::getline(ss, code, ',')) {
      // 去除可能的空白
      code.erase(0, code.find_first_not_of(" \t\n\r"));
      code.erase(code.find_last_not_of(" \t\n\r") + 1);

      if (!code.empty()) {
        // 创建 unique_ptr 实例

        auto queue_ptr = std::make_unique<SPSCQueue<message::Snap>>(queue_size);

        // 如果你想使用原始指针做其他事情（例如传给线程），可以在这里拿到地址
        QueueWithKlineIns queue_kline(queue_ptr.get());
        kline_proc_->queue_location_.push_back(std::move(queue_kline));
        // 插入到 map 中
        code_queue_.emplace(std::move(code), std::move(queue_ptr));
      }
  }
}

void QuantDataProc::DataClassify() {
  while (thread_flag_) {
    Snap* snap = snap_spsc_queue.front();
    if (snap != nullptr) {
      std::string code(snap->security_id);
      auto it = code_queue_.find(code);
      if (it != code_queue_.end()) {
        // 找到对应的 SPSCQueue，尝试入队
        if (it->second->try_push(*snap) == false) {
          std::cout << "code is: " << it->first << " spsc is full" << std::endl;
        }
      }
      snap_spsc_queue.pop();  // 无论是否成功分类，都要从原始队列中移除
    }
  }
}