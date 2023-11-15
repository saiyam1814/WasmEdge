// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

#include "loader/loader.h"

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Loader {

Expect<std::variant<AST::Component, AST::Module>> Loader::loadUnit() {
  // component ::= <preamble> s*:<section>* => (component flatten(s*))
  // preamble  ::= <magic> <version> <layer>
  // magic     ::= 0x00 0x61 0x73 0x6D
  // version   ::= 0x0a 0x00
  // layer     ::= 0x01 0x00

  // The combination of version and layer is corresponding to the version of
  // core wasm.
  // The core module has same magic but the different version:
  // 0x01 0x00 0x00 0x00
  auto Magic = FMgr.readBytes(4);
  if (!Magic) {
    return logLoadError(Magic.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> WasmMagic = {0x00, 0x61, 0x73, 0x6D};
  if (*Magic != WasmMagic) {
    return logLoadError(ErrCode::Value::MalformedMagic, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  auto Ver = FMgr.readBytes(4);
  if (!Ver) {
    return logLoadError(Ver.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
  std::vector<Byte> ModuleVersion = {0x01, 0x00, 0x00, 0x00};
  // spec says 0x0a, but it's actually 0x0d, where cargo component compiled out
  std::vector<Byte> ComponentVersion = {0x0d, 0x00, 0x01, 0x00};
  if (*Ver == ModuleVersion) {
    auto Mod = std::make_unique<AST::Module>();
    Mod->getMagic() = WasmMagic;
    Mod->getVersion() = *Ver;
    if (!Conf.getRuntimeConfigure().isForceInterpreter()) {

      if (auto Res = loadModuleAOT(Mod->getAOTSection()); !Res) {
        return Unexpect(Res);
      }
    }
    // Seek to the position after the binary header.
    FMgr.seek(8);
    if (auto Res = loadModule(*Mod); !Res) {
      return Unexpect(Res);
    }

    // Load library from AOT Section for the universal WASM case.
    // For the force interpreter mode, skip this.
    if (!Conf.getRuntimeConfigure().isForceInterpreter() &&
        WASMType == InputType::UniversalWASM) {
      if (auto Res = loadUniversalWASM(*Mod); !Res) {
        return Unexpect(Res);
      }
    }
    return *Mod;
  } else if (*Ver == ComponentVersion) {
    auto Comp = std::make_unique<AST::Component>();
    Comp->getMagic() = WasmMagic;
    Comp->getVersion() = {(*Ver)[0], (*Ver)[1]};
    Comp->getLayer() = {(*Ver)[2], (*Ver)[3]};
    if (auto Res = loadCompnent(*Comp); !Res) {
      return Unexpect(Res);
    }
    return *Comp;
  } else {
    return logLoadError(ErrCode::Value::MalformedVersion, FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }
}

Expect<void> Loader::loadCompnent(AST::Component &Comp) {
  Expect<Byte> Res;
  while (auto Res = FMgr.readByte()) {
    // only keep going if we have new section ID
    uint8_t NewSectionId = *Res;

    switch (NewSectionId) {
    case 0x00:
      Comp.getCustomSections().emplace_back();
      if (auto Res = loadSection(Comp.getCustomSections().back()); !Res) {
        spdlog::error(ErrInfo::InfoAST(ASTNodeAttr::Component));
        return Unexpect(Res);
      }
      break;
    case 0x01:
      spdlog::error(
          "Component model is not fully parsed yet! core:module section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x02:
      spdlog::error(
          "Component model is not fully parsed yet! core:instance section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x03:
      spdlog::error(
          "Component model is not fully parsed yet! core:type section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x04:
      spdlog::error(
          "Component model is not fully parsed yet! component section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x05:
      spdlog::error(
          "Component model is not fully parsed yet! instance section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x06:
      spdlog::error("Component model is not fully parsed yet! alias section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x07:
      spdlog::error("Component model is not fully parsed yet! type section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x08:
      spdlog::error("Component model is not fully parsed yet! canon section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x09:
      spdlog::error("Component model is not fully parsed yet! start section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x0A:
      spdlog::error("Component model is not fully parsed yet! import section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);
    case 0x0B:
      spdlog::error("Component model is not fully parsed yet! export section");
      return logLoadError(ErrCode::Value::Terminated, FMgr.getLastOffset(),
                          ASTNodeAttr::Component);

    default:
      return logLoadError(ErrCode::Value::MalformedSection,
                          FMgr.getLastOffset(), ASTNodeAttr::Component);
    }
  }

  if (!Res) {
    return logLoadError(Res.error(), FMgr.getLastOffset(),
                        ASTNodeAttr::Component);
  }

  return {};
}

} // namespace Loader
} // namespace WasmEdge
