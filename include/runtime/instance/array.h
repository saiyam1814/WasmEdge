// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/instance/array.h - Array Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the array instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"
#include "common/span.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ArrayInstance {
public:
  ArrayInstance() = delete;
  ArrayInstance(const AST::CompositeType &CType, const uint32_t Size) noexcept
      : RefCount(1), CompType(CType), Data(Size) {}
  ArrayInstance(const AST::CompositeType &CType, const uint32_t Size,
                const ValVariant &Init) noexcept
      : RefCount(1), CompType(CType), Data(Size, Init) {}
  ArrayInstance(const AST::CompositeType &CType,
                std::vector<ValVariant> &&Init) noexcept
      : RefCount(1), CompType(CType), Data(Init) {}

  /// Get field data in array instance.
  ValVariant &getData(uint32_t Idx) noexcept { return Data[Idx]; }
  const ValVariant &getData(uint32_t Idx) const noexcept { return Data[Idx]; }

  /// Get field type in array type.
  const ValType &getDataType() const noexcept {
    return CompType.getFieldTypes()[0].getStorageType();
  }

  /// Get array length.
  uint32_t getLength() const noexcept { return Data.size(); }

  /// Get reference count.
  uint32_t getRefCount() const noexcept { return RefCount; }

private:
  /// \name Data of array instance.
  /// @{
  uint32_t RefCount;
  const AST::CompositeType &CompType;
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
