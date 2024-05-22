//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Support/Module.h"

int64_t top::GroupNormTrainOp::getFLOPs() {
  return module::getNumElements(getOutput()) * 2;
}

LogicalResult top::GroupNormTrainOp::init(InferenceParameter &p) {
  return success();
}
void top::GroupNormTrainOp::deinit(InferenceParameter &p) {}

LogicalResult top::GroupNormTrainOp::inference(InferenceParameter &p) {
  llvm_unreachable("Not Implemented");
  return success();
}

void top::GroupNormTrainOp::shape_inference() {

}
