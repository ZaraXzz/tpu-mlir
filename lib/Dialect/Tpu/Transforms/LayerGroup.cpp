//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Dialect/Tpu/Transforms/Passes.h"

#include "tpu_mlir/Dialect/Tpu/Transforms/LayerGroup/GroupOps.h"

using namespace llvm;

namespace tpu_mlir {
namespace tpu {

bool force_group_by_cores(const std::string &option) {
  if (option == "true") {
    return true;
  } else if (option == "auto" || option == "false") {
    // auto as false
    return false;
  }
  llvm_unreachable("Unknown layer group options");
  return true;
}

class LayerGroupPass : public LayerGroupBase<LayerGroupPass> {
public:
  LayerGroupPass() {}
  void runOnOperation() override {
    // init global options
    LgPass::OPTIONS.opt = opt;
    LgPass::OPTIONS.group_by_cores = force_group_by_cores(group_by_cores);

    // group pass by modules
    auto modules = module::getAllModules();
    for (auto s : *modules) {
      for (auto f : s.getOps<FuncOp>()) {
        if (f.getName() == "main") {
          continue;
        }
        GroupOps gOps(f);
        gOps.process();
      }
    }
  }
};

std::unique_ptr<OperationPass<ModuleOp>> createLayerGroupPass() {
  return std::make_unique<LayerGroupPass>();
}
} // namespace tpu
} // namespace tpu_mlir
