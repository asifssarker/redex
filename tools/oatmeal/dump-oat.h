/**
 * Copyright (c) 2017-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#pragma once

#include "util.h"

#include <memory>

class MemoryAccounter;

class OatFile {
 public:
  UNCOPYABLE(OatFile);
  virtual ~OatFile();

  // reads magic number, returns correct oat file implementation.
  static std::unique_ptr<OatFile> parse(ConstBuffer buf);

  virtual void print(bool dump_classes, bool dump_tables) = 0;

  MemoryAccounter* memory_accounter() { return memory_accounter_.get(); }

 protected:
  explicit OatFile(std::unique_ptr<MemoryAccounter> ma);

 private:
  std::unique_ptr<MemoryAccounter> memory_accounter_;
};
