// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/ast/type.h - type class definitions ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the type classes: Limit, FunctionType,
/// MemoryType, TableType, and GlobalType.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/span.h"
#include "common/symbol.h"
#include "common/types.h"

#include <vector>

namespace WasmEdge {
namespace AST {

/// AST Limit node.
class Limit {
public:
  /// Limit type enumeration class.
  enum class LimitType : uint8_t {
    HasMin = 0x00,
    HasMinMax = 0x01,
    SharedNoMax = 0x02,
    Shared = 0x03
  };

  /// Constructors.
  Limit() noexcept : Type(LimitType::HasMin), Min(0U), Max(0U) {}
  Limit(uint32_t MinVal) noexcept
      : Type(LimitType::HasMin), Min(MinVal), Max(MinVal) {}
  Limit(uint32_t MinVal, uint32_t MaxVal, bool Shared = false) noexcept
      : Min(MinVal), Max(MaxVal) {
    if (Shared) {
      Type = LimitType::Shared;
    } else {
      Type = LimitType::HasMinMax;
    }
  }

  /// Getter and setter of limit mode.
  bool hasMax() const noexcept {
    return Type == LimitType::HasMinMax || Type == LimitType::Shared;
  }
  bool isShared() const noexcept { return Type == LimitType::Shared; }
  void setType(LimitType TargetType) noexcept { Type = TargetType; }

  /// Getter and setter of min value.
  uint32_t getMin() const noexcept { return Min; }
  void setMin(uint32_t Val) noexcept { Min = Val; }

  /// Getter and setter of max value.
  uint32_t getMax() const noexcept { return Max; }
  void setMax(uint32_t Val) noexcept { Max = Val; }

private:
  /// \name Data of Limit.
  /// @{
  LimitType Type;
  uint32_t Min;
  uint32_t Max;
  /// @}
};

/// AST FunctionType node.
class FunctionType {
public:
  /// Function type wrapper for symbols.
  using Wrapper = void(void *ExecCtx, void *Function, const ValVariant *Args,
                       ValVariant *Rets);

  /// Constructors.
  FunctionType() noexcept = default;
  FunctionType(Span<const ValType> P, Span<const ValType> R) noexcept
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()) {}
  FunctionType(Span<const ValType> P, Span<const ValType> R,
               Symbol<Wrapper> S) noexcept
      : ParamTypes(P.begin(), P.end()), ReturnTypes(R.begin(), R.end()),
        WrapSymbol(std::move(S)) {}

  /// `==` and `!=` operator overloadings.
  friend bool operator==(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return LHS.ParamTypes == RHS.ParamTypes &&
           LHS.ReturnTypes == RHS.ReturnTypes;
  }

  friend bool operator!=(const FunctionType &LHS,
                         const FunctionType &RHS) noexcept {
    return !(LHS == RHS);
  }

  /// Getter of param types.
  const std::vector<ValType> &getParamTypes() const noexcept {
    return ParamTypes;
  }
  std::vector<ValType> &getParamTypes() noexcept { return ParamTypes; }

  /// Getter of return types.
  const std::vector<ValType> &getReturnTypes() const noexcept {
    return ReturnTypes;
  }
  std::vector<ValType> &getReturnTypes() noexcept { return ReturnTypes; }

  /// Getter and setter of symbol.
  const auto &getSymbol() const noexcept { return WrapSymbol; }
  void setSymbol(Symbol<Wrapper> S) noexcept { WrapSymbol = std::move(S); }

private:
  /// \name Data of FunctionType.
  /// @{
  std::vector<ValType> ParamTypes;
  std::vector<ValType> ReturnTypes;
  Symbol<Wrapper> WrapSymbol;
  /// @}
};

/// AST FieldType node for GC proposal.
class FieldType {
public:
  /// Constructors.
  FieldType() noexcept = default;
  FieldType(const ValType &Type, ValMut Mut) noexcept : Type(Type), Mut(Mut) {}

  /// Getter and setter of storage type.
  const ValType &getStorageType() const noexcept { return Type; }
  void setStorageType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter of value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  ValType Type;
  ValMut Mut;
};

/// AST CompositeType node for GC proposal.
class CompositeType {
public:
  /// Constructors.
  CompositeType() noexcept = default;
  CompositeType(const FunctionType &FT) noexcept
      : Type(TypeCode::Func), FType(FT) {}

  /// Getter of content.
  const FunctionType &getFuncType() const noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  FunctionType &getFuncType() noexcept {
    return *std::get_if<FunctionType>(&FType);
  }
  const std::vector<FieldType> &getFieldTypes() const noexcept {
    return *std::get_if<std::vector<FieldType>>(&FType);
  }

  /// Setter of content.
  void setArrayType(FieldType &&FT) noexcept {
    Type = TypeCode::Array;
    FType = std::vector<FieldType>{std::move(FT)};
  }
  void setStructType(std::vector<FieldType> &&VFT) noexcept {
    Type = TypeCode::Struct;
    FType = std::move(VFT);
  }
  void setFunctionType(FunctionType &&FT) noexcept {
    Type = TypeCode::Func;
    FType = std::move(FT);
  }

  /// Getter of content type.
  TypeCode getContentTypeCode() const noexcept { return Type; }

  /// Checker if is a function type.
  bool isFunc() const noexcept { return (Type == TypeCode::Func); }

  /// Expand the composite type to its reference.
  TypeCode expand() const noexcept {
    switch (Type) {
    case TypeCode::Func:
      return TypeCode::FuncRef;
    case TypeCode::Struct:
      return TypeCode::StructRef;
    case TypeCode::Array:
      return TypeCode::ArrayRef;
    default:
      assumingUnreachable();
    }
  }

private:
  TypeCode Type;
  std::variant<std::vector<FieldType>, FunctionType> FType;
};

/// AST SubType node for GC proposal.
class SubType {
public:
  /// Constructors.
  SubType() noexcept = default;
  SubType(const FunctionType &FT) noexcept : IsFinal(true), CompType(FT) {}

  /// Getter and setter of final flag.
  bool isFinal() const noexcept { return IsFinal; }
  void setFinal(bool F) noexcept { IsFinal = F; }

  /// Getter of type index vector.
  Span<const uint32_t> getTypeIndices() const noexcept { return TypeIndex; }
  std::vector<uint32_t> &getTypeIndices() noexcept { return TypeIndex; }

  /// Getter of composite type.
  const CompositeType &getCompositeType() const noexcept { return CompType; }
  CompositeType &getCompositeType() noexcept { return CompType; }

private:
  bool IsFinal;
  std::vector<uint32_t> TypeIndex;
  CompositeType CompType;
};

/// AST Type match helper class.
class TypeMatcher {
public:
  static bool matchType(Span<const SubType> ExpTypeList, uint32_t ExpIdx,
                        Span<const SubType> GotTypeList,
                        uint32_t GotIdx) noexcept {
    const auto &ExpType = ExpTypeList[ExpIdx];
    const auto &GotType = GotTypeList[GotIdx];
    if (ExpIdx == GotIdx) {
      return true;
    }
    for (auto TIdx : GotType.getTypeIndices()) {
      if (matchType(ExpTypeList, ExpIdx, GotTypeList, TIdx)) {
        return true;
      }
    }
    return matchType(ExpTypeList, ExpType.getCompositeType(), GotTypeList,
                     GotType.getCompositeType());
  }

  static bool matchType(Span<const SubType> ExpTypeList,
                        const AST::CompositeType &Exp,
                        Span<const SubType> GotTypeList,
                        const AST::CompositeType &Got) noexcept {
    if (Exp.getContentTypeCode() != Got.getContentTypeCode()) {
      return false;
    }
    switch (Exp.getContentTypeCode()) {
    case TypeCode::Func: {
      const auto &ExpFType = Exp.getFuncType();
      const auto &GotFType = Got.getFuncType();
      return matchTypes(ExpTypeList, ExpFType.getParamTypes(), GotTypeList,
                        GotFType.getReturnTypes()) &&
             matchTypes(ExpTypeList, ExpFType.getReturnTypes(), GotTypeList,
                        GotFType.getReturnTypes());
    }
    case TypeCode::Struct: {
      const auto &ExpFType = Exp.getFieldTypes();
      const auto &GotFType = Got.getFieldTypes();
      if (GotFType.size() < ExpFType.size()) {
        return false;
      }
      for (uint32_t I = 0; I < ExpFType.size(); I++) {
        if (!matchType(ExpTypeList, ExpFType[I], GotTypeList, GotFType[I])) {
          return false;
        }
      }
      return true;
    }
    case TypeCode::Array: {
      const auto &ExpFType = Exp.getFieldTypes();
      const auto &GotFType = Got.getFieldTypes();
      return matchType(ExpTypeList, ExpFType[0], GotTypeList, GotFType[0]);
    }
    default:
      return false;
    }
  }

  static bool matchType(Span<const SubType> ExpTypeList,
                        const AST::FieldType &Exp,
                        Span<const SubType> GotTypeList,
                        const AST::FieldType &Got) noexcept {
    bool IsMatch = false;
    if (Exp.getValMut() == Got.getValMut()) {
      // For both const or both var: Got storage type should match the expected
      // storage type.
      IsMatch = matchType(ExpTypeList, Exp.getStorageType(), GotTypeList,
                          Got.getStorageType());
      if (Exp.getValMut() == ValMut::Var) {
        // If both var: and vice versa.
        IsMatch &= matchType(GotTypeList, Got.getStorageType(), ExpTypeList,
                             Exp.getStorageType());
      }
    }
    return IsMatch;
  }

  static bool matchType(Span<const SubType> ExpTypeList, const ValType &Exp,
                        Span<const SubType> GotTypeList,
                        const ValType &Got) noexcept {
    if (!Exp.isRefType() && !Got.isRefType() &&
        Exp.getCode() == Got.getCode()) {
      // Match for the non-reference type case.
      return true;
    }
    if (Exp.isRefType() && Got.isRefType()) {
      // Nullable matching.
      if (!(Exp.isNullableRefType() || !Got.isNullableRefType())) {
        return false;
      }

      // Match heap type.
      if (Exp.isAbsHeapType() && Got.isAbsHeapType()) {
        // Case 1: Both abstract heap type.
        return matchType(Exp.getHeapTypeCode(), Got.getHeapTypeCode());
      } else if (Exp.isAbsHeapType()) {
        // Case 2: Match a type index to abstract heap type.
        return matchType(
            Exp.getHeapTypeCode(),
            GotTypeList[Got.getTypeIndex()].getCompositeType().expand());
      } else if (Got.isAbsHeapType()) {
        // Case 3: Match abstract heap type to a type index.
        TypeCode ExpandGotType =
            ExpTypeList[Exp.getTypeIndex()].getCompositeType().expand();
        switch (Got.getHeapTypeCode()) {
        case TypeCode::NullRef:
          return matchType(TypeCode::AnyRef, ExpandGotType);
        case TypeCode::NullFunc:
          return matchType(TypeCode::FuncRef, ExpandGotType);
        case TypeCode::NullExtern:
          return matchType(TypeCode::ExternRef, ExpandGotType);
        default:
          return false;
        }
      } else {
        // Case 4: Match defined types.
        return matchType(ExpTypeList, Exp.getTypeIndex(), GotTypeList,
                         Got.getTypeIndex());
      }
    }
    return false;
  }

  static bool matchType(TypeCode Exp, TypeCode Got) noexcept {
    // Handle the equal cases first.
    if (Exp == Got) {
      return true;
    }

    // Match the func types: nofunc <= func
    if (Exp == TypeCode::FuncRef || Exp == TypeCode::NullFunc) {
      return Got == TypeCode::NullFunc;
    }
    if (Got == TypeCode::FuncRef || Got == TypeCode::NullFunc) {
      return false;
    }

    // Match the extern types: noextern <= extern
    if (Exp == TypeCode::ExternRef || Exp == TypeCode::NullExtern) {
      return Got == TypeCode::NullExtern;
    }
    if (Got == TypeCode::ExternRef || Got == TypeCode::NullExtern) {
      return false;
    }

    // Match the other types: none <= i31 | struct | array <= eq <= any
    switch (Exp) {
    case TypeCode::I31Ref:
    case TypeCode::StructRef:
    case TypeCode::ArrayRef:
      // This will filter out the i31/struct/array unmatch cases.
      return Got == TypeCode::NullRef;
    case TypeCode::EqRef:
      return Got != TypeCode::AnyRef;
    case TypeCode::AnyRef:
      return true;
    default:
      break;
    }
    return false;
  }

  static bool matchTypes(Span<const SubType> ExpTypeList,
                         Span<const ValType> Exp,
                         Span<const SubType> GotTypeList,
                         Span<const ValType> Got) noexcept {
    if (Exp.size() != Got.size()) {
      return false;
    }
    for (uint32_t I = 0; I < Exp.size(); I++) {
      if (!matchType(ExpTypeList, Exp[I], GotTypeList, Got[I])) {
        return false;
      }
    }
    return true;
  }
};

/// AST MemoryType node.
class MemoryType {
public:
  /// Constructors.
  MemoryType() noexcept = default;
  MemoryType(uint32_t MinVal) noexcept : Lim(MinVal) {}
  MemoryType(uint32_t MinVal, uint32_t MaxVal, bool Shared = false) noexcept
      : Lim(MinVal, MaxVal, Shared) {}
  MemoryType(const Limit &L) noexcept : Lim(L) {}

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }

private:
  /// \name Data of MemoryType.
  /// @{
  Limit Lim;
  /// @}
};

/// AST TableType node.
class TableType {
public:
  /// Constructors.
  TableType() noexcept : Type(TypeCode::FuncRef), Lim() {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint32_t MinVal) noexcept
      : Type(RType), Lim(MinVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, uint32_t MinVal, uint32_t MaxVal) noexcept
      : Type(RType), Lim(MinVal, MaxVal) {
    assuming(Type.isRefType());
  }
  TableType(const ValType &RType, const Limit &L) noexcept
      : Type(RType), Lim(L) {
    assuming(Type.isRefType());
  }

  /// Getter of reference type.
  const ValType &getRefType() const noexcept { return Type; }
  void setRefType(const ValType &RType) noexcept {
    assuming(RType.isRefType());
    Type = RType;
  }

  /// Getter of limit.
  const Limit &getLimit() const noexcept { return Lim; }
  Limit &getLimit() noexcept { return Lim; }

private:
  /// \name Data of TableType.
  /// @{
  ValType Type;
  Limit Lim;
  /// @}
};

/// AST GlobalType node.
class GlobalType {
public:
  /// Constructors.
  GlobalType() noexcept : Type(TypeCode::I32), Mut(ValMut::Const) {}
  GlobalType(const ValType &VType, ValMut VMut) noexcept
      : Type(VType), Mut(VMut) {}

  /// Getter and setter of value type.
  const ValType &getValType() const noexcept { return Type; }
  void setValType(const ValType &VType) noexcept { Type = VType; }

  /// Getter and setter of value mutation.
  ValMut getValMut() const noexcept { return Mut; }
  void setValMut(ValMut VMut) noexcept { Mut = VMut; }

private:
  /// \name Data of GlobalType.
  /// @{
  ValType Type;
  ValMut Mut;
  /// @}
};

} // namespace AST
} // namespace WasmEdge
