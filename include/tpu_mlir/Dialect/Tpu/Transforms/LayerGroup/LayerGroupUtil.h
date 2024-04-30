//===----------------------------------------------------------------------===//
//
// Copyright (C) 2022 Sophgo Technologies Inc.  All rights reserved.
//
// TPU-MLIR is licensed under the 2-Clause BSD License except for the
// third-party components.
//
//===----------------------------------------------------------------------===//

#pragma once

#include "mlir/Support/LLVM.h"
#include "tpu_mlir/Dialect/Tpu/IR/TpuOps.h"
#include "tpu_mlir/Support/Module.h"
#include <list>
#include <map>
#include <set>

#include "tpu_mlir/Dialect/Tpu/Transforms/LayerGroup/BasicTimeStep.h"
#include "tpu_mlir/Dialect/Tpu/Transforms/LayerGroup/LayerGroupDefs.h"

namespace tpu_mlir {
namespace tpu {
class ILPTimeStep;
class dot_graph;
typedef std::pair<int64_t, int64_t> slice_pair_t; // idx and slice
shape_secs_t get_group_max_secs(const LgInfo &lg_info, std::vector<std::pair<Operation*, int>>& vec_op_hsecs);
bool init_group_data_secs(const LgInfo &lg_info, shape_secs_t &shape_secs,
                          std::vector<std::pair<Value, int64_t>>& value_size);
void update_tensor_infos(const LgInfo &lg_info, TensorInfo &tensor_infos);
bool update_data_split(BasicTimeStepPtr time_step, const LgInfo &lg_info,
                       shape_secs_t &shape_secs);

bool strip_back_judge(Value v, const LgInfo &lg_info,
                      const std::multiset<Operation *> &op_set,
                      const std::set<Value, value_compare> &out_tensor_set);
bool is_same_slice_info(const slice_info_t &si0, const slice_info_t &si1);
slice_info_t get_out_slice_info(const shape_secs_t &shape_secs, int64_t n,
                                int64_t c, int64_t h, int64_t d, int64_t w,
                                int64_t bitwidth);
bool get_backward_slice_info(slice_info_t &in_si, const slice_info_t &out_si,
                             Operation *op, Value in,
                             const shape_secs_t &shape_secs,
                             group_type_t group_type, bool &hold_in_lmem,
                             bool is_group_in);
bool stripe_mine_max_slice(const LgInfo &lg_info,
                           const shape_secs_t &shape_secs,
                           TensorInfo &tensor_infos);

void get_max_slice_nchdw(const slice_info_t &slice_info, int64_t &max_nslice,
                         int64_t &max_cslice, int64_t &max_hslice,
                         int64_t &max_dslice, int64_t &max_wslice);

std::vector<slice_pair_t>
get_max_slice_nchdw_and_idx(const slice_info_t &slice_info, int64_t &max_nslice,
                            int64_t &max_cslice, int64_t &max_hslice,
                            int64_t &max_dslice, int64_t &max_wslice);

void assign_dhwsecs(const LgInfo &lg_info, shape_secs_t &shape_secs,
                    int64_t &dhw_secs, const shape_secs_t &max_shape_secs);

int64_t get_buffer_size(Value v, tensor_info_t &ti,
                        group_type_t group_type, Operation* owner_op = nullptr);

bool stripe_mine_idx_slice(const LgInfo &lg_info,
                           const shape_secs_t &shape_secs,
                           TensorInfo &tensor_infos);

void set_fake_local_layer_param(Operation *op, int64_t nidx, int64_t nslice,
                                int64_t hidx, int64_t hslice, int64_t didx,
                                int64_t cidx, int64_t cslice, int64_t dslice,
                                int64_t widx, int64_t wslice);
void delete_fake_local_layer_param(Operation *op);

void set_weight_allow_split_attr(Operation *op);

void delete_weight_allow_split_attr(Operation *op);

void generate_fake_global_addr(Operation *op);

void delete_fake_global_addr(Operation *op);

bool is_eu_align(Value opd);
bool is_value_weight(Value opd);

bool need_bcast(Value opd);

int64_t use_3ic(Value opd);

std::vector<Value> get_input_values(Operation *op);
std::vector<Value> get_output_values(Operation *op);

bool strip_back_judge2(Value v, const LgInfo &lg_info,
                      const std::multiset<Operation *> &op_set,
                      const std::set<Value, value_compare> &out_tensor_set);
bool stripe_mine_idx_slice2(const LgInfo &lg_info,
                           const shape_secs_t &shape_secs,
                           TensorInfo &tensor_infos, Operation*& fail_op);
class CycleCalculator;
bool backward_gen_ilp_var2(const LgInfo &lg_info,
                           const shape_secs_t &shape_secs,
                           TensorInfo &tensor_infos, std::shared_ptr<CycleCalculator> cycle_calculator_, ILPTimeStep& ilp_timeStep,
                           const std::vector<int64_t>& ncdhw_idx, int ts_offset,
                           std::vector<op_var_pos_info>& op_var_bound, Operation* returnOp, int64_t& load_bytes_for_next_ts,
                           std::shared_ptr<dot_graph> dot_graph_log,
                           std::vector<std::pair<Value, int64_t>> tmp_value_size, Operation*& failOp,
                           bool l2m_en = true, bool last_slice = false, bool train = false, int max_ahead_or_delay_ts = 4);
bool isLgSupport(Operation *op);
void find_all_pre_ops(Operation * op, std::vector<Operation*>& glayer_pre_ops);
void find_all_next_ops(Operation * op, std::vector<Operation*>& glayer_next_ops);
std::string replaceChars_for_dot(std::string str);
bool isPreOpHaveAComputeOp(Operation * op);


struct dot_graph {
  dot_graph() {
    add_node_into_graph("global_info");
  }
  ~dot_graph() { }

  bool endsWith(const std::string& str, const std::string& suffix) {
      return str.rfind(suffix) == str.length() - suffix.length();
  }

  void add_node_into_graph(std::string node_name) {
    node_name = replaceChars_for_dot(node_name);
    if (main_graph_nodes.find(node_name) == main_graph_nodes.end()) {
      main_graph_nodes[node_name] = std::vector<std::string>();
    }
  }
  void add_node_label(std::string node_name, std::string node_label) {
    node_name = replaceChars_for_dot(node_name);
    if (main_graph_nodes.find(node_name) != main_graph_nodes.end()) {
      auto& tmp = main_graph_nodes[node_name];
      if (std::find(tmp.begin(), tmp.end(), node_label) == tmp.end()) {
        tmp.push_back(node_label);
      }
    }
  }
  void add_all_node_label(std::string info, std::string filter = "_ori") {
    for (auto& itr: main_graph_nodes) {
      if (endsWith(itr.first, filter)) {
        itr.second.push_back(info);
      }
    }
  }
  void add_edge_into_graph(std::string from_node_name, std::string to_node_name) {
    assert(from_node_name != to_node_name);
    from_node_name = replaceChars_for_dot(from_node_name);
    to_node_name = replaceChars_for_dot(to_node_name);
    if (main_graph_edges.find(from_node_name) == main_graph_edges.end()) {
      main_graph_edges[from_node_name] = std::vector<std::string>();
    }
    auto& tmp = main_graph_edges[from_node_name];
    if (std::find(tmp.begin(), tmp.end(), to_node_name) == tmp.end()) {
      tmp.push_back(to_node_name);
    }
  }

  void export_dot(std::string file_name) {
    if (module::isDebugCmdEnable("disable_dot")) {
      return;
    }
    std::stringstream ssTime;
    auto clockNow = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(clockNow);
    ssTime << std::put_time(std::localtime(&t), "%Y%m%d-%H%M%S");
    file_name =  file_name + "-" +ssTime.str() + ".dot";
    llvm::errs() << "export_dot to file:"<<file_name<<"\n";
    std::ofstream log_stream(file_name, std::ios::out);
    log_stream << "digraph G { " <<std::endl;
    log_stream << " graph [label=\"GroupOps\";";
    // log_stream << " labelloc=\"t\";";
    log_stream << " fontsize="<< 30 << ";";
    log_stream << " fontcolor=\""<< "black" << "\";]" <<std::endl;
    log_stream << " node [shape = record, color = black]" <<std::endl;

    for (auto itr: main_graph_nodes) {
      std::string tmp_str;
      for (auto itr2: itr.second) {
        if (tmp_str == "")
          tmp_str = itr2;
        else
          tmp_str = tmp_str + "\\n" + itr2;
      }
      log_stream << llvm::formatv(" {0} [label= \"{0}\\n{1}\"];",
                    itr.first.c_str(), tmp_str).str() <<std::endl;
    }
    for (auto itr: main_graph_edges) {
      for (auto itr2: itr.second) {
        // log_stream << llvm::formatv(" {0} -> {1} [label= \"{0}\"];", itr.first.c_str(), itr2.c_str()).str() <<std::endl;
        log_stream << llvm::formatv(" {0} -> {1};", itr.first.c_str(), itr2.c_str()).str() <<std::endl;
      }
    }

    log_stream << "}" <<std::endl;
    log_stream.close();
    system(llvm::formatv("dot -Tsvg {0} -o ./{0}.svg", file_name.c_str()).str().c_str());
  }
  std::map<std::string, std::vector<std::string>> main_graph_nodes;
  std::map<std::string, std::vector<std::string>> main_graph_edges;
};

} // namespace tpu
} // namespace tpu_mlir
