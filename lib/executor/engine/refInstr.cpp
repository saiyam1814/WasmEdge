// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "executor/executor.h"

namespace WasmEdge {
namespace Executor {

namespace {
ValVariant packVal(const ValType &Type, const ValVariant &Val) {
  if (Type.isPackType()) {
    switch (Type.getCode()) {
    case TypeCode::I8:
      return ValVariant(uint32_t(Val.get<uint32_t>() & 0xFFU));
    case TypeCode::I16:
      return ValVariant(uint32_t(Val.get<uint32_t>() & 0xFFFFU));
    default:
      assumingUnreachable();
    }
  }
  return Val;
}

std::vector<ValVariant> packVals(const ValType &Type,
                                 std::vector<ValVariant> &&Vals) {
  uint32_t Mask = 0U;
  if (Type.isPackType()) {
    switch (Type.getCode()) {
    case TypeCode::I8:
      Mask = 0xFFU;
      break;
    case TypeCode::I16:
      Mask = 0xFFFFU;
      break;
    default:
      assumingUnreachable();
    }
    std::transform(
        Vals.cbegin(), Vals.cend(), Vals.begin(),
        [Mask](const ValVariant &Val) { return Val.get<uint32_t>() & Mask; });
  }
  return std::move(Vals);
}
} // namespace

Expect<void> Executor::runRefNullOp(Runtime::StackManager &StackMgr,
                                    const ValType &Type) const noexcept {
  StackMgr.push(RefVariant(Type));
  return {};
}

Expect<void> Executor::runRefIsNullOp(ValVariant &Val) const noexcept {
  Val.emplace<uint32_t>(Val.get<RefVariant>().isNull() ? 1U : 0U);
  return {};
}

Expect<void> Executor::runRefFuncOp(Runtime::StackManager &StackMgr,
                                    uint32_t Idx) const noexcept {
  const auto *FuncInst = getFuncInstByIdx(StackMgr, Idx);
  StackMgr.push(RefVariant(FuncInst));
  return {};
}

Expect<void> Executor::runRefEqOp(ValVariant &Val1,
                                  const ValVariant &Val2) const noexcept {
  Val1.emplace<uint32_t>(Val1.get<RefVariant>().asPtr<void>() ==
                                 Val2.get<RefVariant>().asPtr<void>()
                             ? 1U
                             : 0U);
  return {};
}

Expect<void>
Executor::runRefAsNonNullOp(RefVariant &Ref,
                            const AST::Instruction &Instr) const noexcept {
  if (Ref.isNull()) {
    spdlog::error(ErrCode::Value::CastNullToNonNull);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::CastNullToNonNull);
  }
  Ref = RefVariant(Ref.getType().toNonNullableRef(), Ref);
  return {};
}

Expect<void> Executor::runStructNewOp(Runtime::StackManager &StackMgr,
                                      const AST::CompositeType &CompType,
                                      bool IsDefault) const noexcept {
  if (IsDefault) {
    StackMgr.push(RefVariant(Runtime::HeapManager::newStruct(CompType)));
  } else {
    uint32_t N = CompType.getFieldTypes().size();
    auto Vals = StackMgr.pop(N);
    for (uint32_t I = 0; I < N; I++) {
      Vals[I] = packVal(CompType.getFieldTypes()[I].getStorageType(), Vals[I]);
    }
    StackMgr.push(
        RefVariant(Runtime::HeapManager::newStruct(CompType, std::move(Vals))));
  }
  return {};
}

Expect<void> Executor::runArrayNewOp(Runtime::StackManager &StackMgr,
                                     const AST::CompositeType &CompType,
                                     uint32_t InitCnt,
                                     uint32_t ValCnt) const noexcept {
  assuming(InitCnt == 0 || InitCnt == 1 || InitCnt == ValCnt);
  const auto &VType = CompType.getFieldTypes()[0].getStorageType();
  if (InitCnt == 0) {
    StackMgr.push(RefVariant(Runtime::HeapManager::newArray(CompType, ValCnt)));
  } else if (InitCnt == 1) {
    StackMgr.getTop() = RefVariant(Runtime::HeapManager::newArray(
        CompType, ValCnt, packVal(VType, StackMgr.getTop())));
  } else {
    StackMgr.push(RefVariant(Runtime::HeapManager::newArray(
        CompType, packVals(VType, StackMgr.pop(ValCnt)))));
  }
  return {};
}

Expect<void>
Executor::runArrayNewDataOp(Runtime::StackManager &StackMgr,
                            const AST::CompositeType &CompType,
                            const Runtime::Instance::DataInstance &DataInst,
                            const AST::Instruction &Instr) const noexcept {
  const uint32_t N = StackMgr.pop().get<uint32_t>();
  const uint32_t S = StackMgr.getTop().get<uint32_t>();
  const uint32_t BSize =
      CompType.getFieldTypes()[0].getStorageType().getBitWidth() / 8;
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) * BSize >=
      DataInst.getData().size()) {
    // TODO: GC - error log
    spdlog::error(ErrCode::Value::LengthOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::LengthOutOfBounds);
  }
  auto *Inst = Runtime::HeapManager::newArray(CompType, N);
  for (uint32_t Idx = 0; Idx < N; Idx++) {
    // The value has been packed.
    Inst->getData(Idx) = DataInst.loadValue(S + Idx * BSize, BSize);
  }
  StackMgr.getTop() = RefVariant(Inst);
  return {};
}

Expect<void>
Executor::runArrayNewElemOp(Runtime::StackManager &StackMgr,
                            const AST::CompositeType &CompType,
                            const Runtime::Instance::ElementInstance &ElemInst,
                            const AST::Instruction &Instr) const noexcept {
  const uint32_t N = StackMgr.pop().get<uint32_t>();
  const uint32_t S = StackMgr.getTop().get<uint32_t>();
  auto ElemSrc = ElemInst.getRefs();
  if (static_cast<uint64_t>(S) + static_cast<uint64_t>(N) >= ElemSrc.size()) {
    // TODO: GC - error log
    spdlog::error(ErrCode::Value::LengthOutOfBounds);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::LengthOutOfBounds);
  }
  // The value has been packed.
  std::vector<ValVariant> Refs(ElemSrc.begin() + S, ElemSrc.begin() + S + N);
  auto *Inst = Runtime::HeapManager::newArray(CompType, std::move(Refs));
  StackMgr.getTop() = RefVariant(Inst);
  return {};
}

Expect<void>
Executor::runArrayLen(ValVariant &Val,
                      const AST::Instruction &Instr) const noexcept {
  const auto *Inst =
      Val.get<RefVariant>().asPtr<Runtime::Instance::ArrayInstance>();
  if (Inst == nullptr) {
    spdlog::error(ErrCode::Value::CastNullToNonNull);
    spdlog::error(
        ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
    return Unexpect(ErrCode::Value::CastNullToNonNull);
  }
  Val.emplace<uint32_t>(Inst->getLength());
  return {};
}

Expect<void> Executor::runRefTestOp(Runtime::StackManager &StackMgr,
                                    RefVariant &Val,
                                    const AST::Instruction &Instr,
                                    bool IsCast) const noexcept {
  const auto &VT = Val.getType();
  auto TList = StackMgr.getModule()->getTypeList();
  if (AST::TypeMatcher::matchType(TList, Instr.getValType(), TList, VT)) {
    if (!IsCast) {
      StackMgr.getTop().emplace<uint32_t>(1U);
    }
  } else {
    if (IsCast) {
      // TODO: GC - error log
      spdlog::error(ErrCode::Value::CastNullToNonNull);
      spdlog::error(
          ErrInfo::InfoInstruction(Instr.getOpCode(), Instr.getOffset()));
      return Unexpect(ErrCode::Value::CastNullToNonNull);
    } else {
      StackMgr.getTop().emplace<uint32_t>(0U);
    }
  }
  return {};
}

Expect<void> Executor::runRefExternConvToOp(RefVariant &Ref,
                                            TypeCode TCode) const noexcept {
  if (Ref.isNull()) {
    Ref = RefVariant(ValType(TypeCode::RefNull, TCode));
  } else {
    Ref = RefVariant(ValType(TypeCode::Ref, TypeCode::ExternRef), Ref);
  }
  return {};
}

Expect<void> Executor::runRefI31(ValVariant &Val) const noexcept {
  Val = RefVariant(ValType(TypeCode::Ref, TypeCode::I31Ref),
                   reinterpret_cast<void *>(Val.get<uint32_t>() & 0x7FFFFFFFU));
  return {};
}

} // namespace Executor
} // namespace WasmEdge
