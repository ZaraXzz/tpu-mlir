//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "pymlir.h"
#include "tpu_mlir/Support/MathUtils.h"
#include "tpu_mlir/Support/Float16.h"

#include <cudnn.h>
#include <cuda_runtime.h>

// Error checking macros
#define CHECK_CUDNN(status)                                                    \
  if (status != CUDNN_STATUS_SUCCESS) {                                        \
    std::cerr << "[" << __FILE__ << ":" << __LINE__                            \
              << "] CUDNN failure: " << cudnnGetErrorString(status)            \
              << std::endl;                                                    \
    exit(EXIT_FAILURE);                                                        \
  }

#define CHECK_CUDA(status)                                                     \
  if (status != cudaSuccess) {                                                 \
    std::cerr << "[" << __FILE__ << ":" << __LINE__                            \
              << "] CUDA failure: " << cudaGetErrorString(status)              \
              << std::endl;                                                    \
    exit(EXIT_FAILURE);                                                        \
  }

// 自定义删除器，用于std::unique_ptr来调用cudaFree
struct cudaDeleter {
  void operator()(void *ptr) {
    if (ptr != nullptr) {
      CHECK_CUDA(cudaFree(ptr));
    }
  }
};

typedef std::unique_ptr<void, cudaDeleter> cuda_ptr;

class py_cuda {
public:
  py_cuda();
  ~py_cuda();
  void load(std::string filename);

  // only can set input data
  void set_tensor(
      std::string name,
      py::array_t<float, py::array::c_style | py::array::forcecast> data);
  void invoke(bool dump_all);
  py::array get_tensor(std::string name);
  py::dict get_all_tensor();

private:
  // -------------------------------------------------------------------
  // -------------- helper functions -----------------------------------
  // get data in cuda by activation_map_ and weight_map_; if not find, will
  // assert
  void *getCudaData(Value v);
  // get cudnn type from mlir type
  cudnnDataType_t getCudnnType(Value v);
  size_t getCudnnTypeBytes(cudnnDataType_t type);
  // convert cuda data from one type to another type
  cuda_ptr newCudaData(void *data, size_t num, cudnnDataType_t src_type,
                       cudnnDataType_t dst_type);
  // alloc new buffer to store new type
  cuda_ptr newCudaData(Value v, cudnnDataType_t dst_type);

  // -------------------------------------------------------------------
  // -------------- op inference by cuda -------------------------------
  void cudaAddOp(tpu::AddOp op);
  void cudaConv2DOp(tpu::Conv2DOp op);
  void cudaCastOp(tpu::CastOp op);
  void cudaConcatOp(tpu::ConcatOp op);
  void cudaDeconvOp(tpu::DeconvOp op);
  void cudaDepth2SpaceOp(tpu::Depth2SpaceOp op);
  void cudaGenericCpuOp(tpu::GenericCpuOp op);
  void cudaLutOp(tpu::LutOp op);
  void cudaMatMulOp(tpu::MatMulOp op);
  void cudaMulOp(tpu::MulOp op);
  void cudaMulShiftOp(tpu::MulShiftOp op);
  void cudaReshapeOp(tpu::ReshapeOp op);
  void cudaRequantIntAxisOp(tpu::RequantIntAxisOp op);
  void cudaPool2DOp(tpu::Pool2DOp op);
  void cudaPermuteOp(tpu::PermuteOp op);
  void cudaSliceOp(tpu::SliceOp op);
  void cudaUpsampleOp(tpu::UpsampleOp op);

private:
  cuda_ptr cuda_malloc(size_t bytes);
  void cuda_malloc(std::map<std::string, cuda_ptr> &map, Value v);
  void cuda_to_host(const std::string &name);

public:
  py::list input_names;
  py::list output_names;

private:
  std::unique_ptr<mlir::MLIRContext> context_;
  OwningOpRef<ModuleOp> module_;
  cudnnHandle_t cudnn_;
  bool dump_all_;
  std::vector<std::string> input_names_;
  std::vector<std::string> output_names_;
  std::map<std::string, Value> value_map_;
  std::map<std::string, cuda_ptr> input_map_;
  std::map<std::string, cuda_ptr> weight_map_;
  std::map<std::string, cuda_ptr> activation_map_;
  std::map<std::string, std::shared_ptr<std::vector<float>>> buffer_map_;
};
