#ifndef BACKTRACE_H_
#define BACKTRACE_H_

#include <inttypes.h>
#include <stdio.h>

#include <string>
#include <vector>

enum BacktraceUnwindError : uint32_t {
  BACKTRACE_UNWIND_NO_ERROR,
  // Something failed while trying to perform the setup to begin the unwind.
  BACKTRACE_UNWIND_ERROR_SETUP_FAILED,
  // There is no map information to use with the unwind.
  BACKTRACE_UNWIND_ERROR_MAP_MISSING,
  // An error occurred that indicates a programming error.
  BACKTRACE_UNWIND_ERROR_INTERNAL,
  // The thread to unwind has disappeared before the unwind can begin.
  BACKTRACE_UNWIND_ERROR_THREAD_DOESNT_EXIST,
  // The thread to unwind has not responded to a signal in a timely manner.
  BACKTRACE_UNWIND_ERROR_THREAD_TIMEOUT,
  // Attempt to do an unsupported operation.
  BACKTRACE_UNWIND_ERROR_UNSUPPORTED_OPERATION,
  // Attempt to do an offline unwind without a context.
  BACKTRACE_UNWIND_ERROR_NO_CONTEXT,
};

struct backtrace_map_t {
  uintptr_t start = 0;
  uintptr_t end = 0;
  uintptr_t offset = 0;
  uintptr_t load_base = 0;
  int flags = 0;
  std::string name;
};

struct backtrace_frame_data_t {
  size_t num;             // The current fame number.
  uintptr_t pc;           // The absolute pc.
  uintptr_t sp;           // The top of the stack.
  size_t stack_size;      // The size of the stack, zero indicate an unknown stack size.
  backtrace_map_t map;    // The map associated with the given pc.
  std::string func_name;  // The function name associated with this pc, NULL if not found.
  uintptr_t func_offset;  // pc relative to the start of the function, only valid if func_name is not NULL.
};

using word_t = unsigned long;

class BacktraceMap;

class BacktraceStub {
public:
  virtual ~BacktraceStub() {}
  virtual bool Unwind(size_t num_ignore_frames, void* context) = 0;
  virtual std::string GetFunctionName(uint64_t pc, uint64_t* offset,
                                      const backtrace_map_t* map = NULL) = 0;
  virtual void FillInMap(uint64_t pc, backtrace_map_t* map) = 0;
  virtual bool ReadWord(uint64_t ptr, word_t* out_value) = 0;
  virtual size_t Read(uint64_t addr, uint8_t* buffer, size_t bytes) = 0;
  virtual std::string FormatFrameData(size_t frame_num) = 0;

  size_t NumFrames() const { return frames_.size(); }

protected:
    pid_t pid_;
    pid_t tid_;
  BacktraceMap* map_;
  bool map_shared_;
  std::vector<backtrace_frame_data_t> frames_;
  BacktraceUnwindError error_;
};

#endif //BACKTRACE_H_
