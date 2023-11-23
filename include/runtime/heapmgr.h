// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/heapmgr.h - Heap Manager definition --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the definition of Heap Manager, in order to manage the
/// allocated structs and arrays.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/instance/array.h"
#include "runtime/instance/struct.h"

#include <memory>
#include <mutex>
#include <vector>

namespace WasmEdge {
namespace Runtime {

class HeapManager {
public:
  template <typename... Args>
  static Instance::ArrayInstance *newArray(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    ArrayStorage.push_back(std::make_unique<Instance::ArrayInstance>(
        std::forward<Args>(Values)...));
    return ArrayStorage.back().get();
  }

  template <typename... Args>
  static Instance::StructInstance *newStruct(Args &&...Values) {
    std::unique_lock Lock(Mutex);
    StructStorage.push_back(std::make_unique<Instance::StructInstance>(
        std::forward<Args>(Values)...));
    return StructStorage.back().get();
  }

private:
  /// \name Mutex for thread-safe.
  static std::mutex Mutex;
  static std::vector<std::unique_ptr<Instance::ArrayInstance>> ArrayStorage;
  static std::vector<std::unique_ptr<Instance::StructInstance>> StructStorage;
};

} // namespace Runtime
} // namespace WasmEdge
