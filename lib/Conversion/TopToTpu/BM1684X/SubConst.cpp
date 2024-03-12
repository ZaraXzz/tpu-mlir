//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#include "tpu_mlir/Conversion/TopToTpu/LoweringBM1684X.h"
#include "tpu_mlir/Support/Float8.h"

namespace tpu_mlir {
namespace bm1684x {

void SubConstLowering::LoweringF32(PatternRewriter &rewriter,
                                   top::SubConstOp op) const {
  lowering_common_f32<tpu::SubConstOp>(rewriter, op);
}

void SubConstLowering::LoweringINT8(PatternRewriter &rewriter,
                                    top::SubConstOp op, bool asymmetric) const {
  if (asymmetric)
    return LoweringF16(rewriter, op);

  auto in = op.getInput();
  auto out = op.getOutput();
  double in_scale, out_scale;

  int64_t in_zp, out_zp;
  module::getScaleAndZeroPoint(in, in_scale, in_zp, asymmetric);
  module::getScaleAndZeroPoint(out, out_scale, out_zp, asymmetric);
  int multiplier, rshift;
  get_scale_and_shift_positive(in_scale / out_scale, multiplier, rshift, 8);

  double const_b = op.getConstVal().convertToDouble();
  const_b = static_cast<int>(round(const_b / out_scale)) << rshift;

  std::vector<NamedAttribute> attrs;
  for (auto &attr : op->getAttrs()) {
    if (attr.getName() == "const_val") {
      attrs.push_back(rewriter.getNamedAttr("const_val",
                                            rewriter.getF64FloatAttr(const_b)));
    } else {
      attrs.push_back(attr);
    }
  }

  attrs.push_back(
    rewriter.getNamedAttr("multiplier", rewriter.getSI32IntegerAttr(multiplier)));
  attrs.push_back(
    rewriter.getNamedAttr("rshift", rewriter.getSI32IntegerAttr(rshift)));


  auto newType = getQuantInt8Type(op.getOutput(), asymmetric);
  rewriter.replaceOpWithNewOp<tpu::SubConstOp>(op, newType, op.getInput(),
                                               attrs);
}

void SubConstLowering::LoweringINT4(PatternRewriter &rewriter,
                                    top::SubConstOp op, bool asymmetric) const {
  LoweringINT8(rewriter, op, asymmetric);
}

void SubConstLowering::LoweringBF16(PatternRewriter &rewriter,
                                    top::SubConstOp op) const {
  lowering_common_bf16<tpu::SubConstOp>(rewriter, op);
}

void SubConstLowering::LoweringF16(PatternRewriter &rewriter,
                                   top::SubConstOp op) const {
  lowering_common_f16<tpu::SubConstOp>(rewriter, op);
}

void SubConstLowering::LoweringF8(PatternRewriter &rewriter,
                                   top::SubConstOp op) const {
  std::vector<NamedAttribute> attrs;
  double const_v = op.getConstVal().convertToDouble();
  auto qtype_in = module::getCalibratedType(op.getInput());
  auto qtype_out = module::getCalibratedType(op.getOutput());

  bool isE4 = module::getMode() == module::Mode::F8E4M3;
  double scale = 1.0;
  if (isE4) {
    double in_scale = qtype_in.getMax() / get_f8e4m3_max();
    double out_scale = qtype_out.getMax() / get_f8e4m3_max();
    const_v = const_v / out_scale;
    scale = in_scale / out_scale;
  }
  for (auto &attr : op->getAttrs()) {
    if (attr.getName() == "const_val") {
      attrs.push_back(rewriter.getNamedAttr("const_val", rewriter.getF64FloatAttr(const_v)));
    } else {
      attrs.push_back(attr);
    }
  }

  attrs.push_back(rewriter.getNamedAttr("f8_scale", rewriter.getF64FloatAttr(scale)));

  if (isE4) {
    auto newType = getQuantF8E4M3Type(op.getOutput());
    rewriter.replaceOpWithNewOp<tpu::SubConstOp>(op, newType,
                                               op.getInput(), attrs);
  } else {
    auto newType = getQuantF8E5M2Type(op.getOutput());
    rewriter.replaceOpWithNewOp<tpu::SubConstOp>(op, newType,
                                               op.getInput(), attrs);
  }
}

void SubConstLowering::LoweringQuantized(PatternRewriter &rewriter,
                                         top::SubConstOp op) const {
  // llvm_unreachable("Not Implemented");
  LoweringINT8(rewriter, op, false);
}

} // namespace bm1684x
} // namespace tpu_mlir
