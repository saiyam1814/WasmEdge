// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/runtime/instance/struct.h - Struct Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the struct instance definition in store manager.
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

class StructInstance {
public:
  StructInstance() = delete;
  StructInstance(const AST::CompositeType &CType) noexcept
      : RefCount(1), CompType(CType), Data(CType.getFieldTypes().size()) {}
  StructInstance(const AST::CompositeType &CType,
                 std::vector<ValVariant> &&Init) noexcept
      : RefCount(1), CompType(CType), Data(Init) {}

  /// Get field data in struct instance.
  ValVariant &getData(uint32_t Idx) noexcept { return Data[Idx]; }
  const ValVariant &getData(uint32_t Idx) const noexcept { return Data[Idx]; }

  /// Get field type in struct type.
  const ValType &getDataType(uint32_t Idx) const noexcept {
    return CompType.getFieldTypes()[Idx].getStorageType();
  }

  /// Get reference count.
  uint32_t getRefCount() const noexcept { return RefCount; }

private:
  /// \name Data of struct instance.
  /// @{
  uint32_t RefCount;
  const AST::CompositeType &CompType;
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
