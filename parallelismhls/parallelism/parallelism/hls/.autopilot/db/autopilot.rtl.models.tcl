set SynModuleInfo {
  {SRCNAME blur_edge_Pipeline_row_loop_col_loop MODELNAME blur_edge_Pipeline_row_loop_col_loop RTLNAME blur_edge_blur_edge_Pipeline_row_loop_col_loop
    SUBMODULES {
      {MODELNAME blur_edge_urem_31ns_4ns_3_35_1 RTLNAME blur_edge_urem_31ns_4ns_3_35_1 BINDTYPE op TYPE urem IMPL auto LATENCY 34 ALLOW_PRAGMA 1}
      {MODELNAME blur_edge_sparsemux_11_3_8_1_1 RTLNAME blur_edge_sparsemux_11_3_8_1_1 BINDTYPE op TYPE sparsemux IMPL auto}
      {MODELNAME blur_edge_sparsemux_9_3_8_1_1 RTLNAME blur_edge_sparsemux_9_3_8_1_1 BINDTYPE op TYPE sparsemux IMPL auto}
      {MODELNAME blur_edge_sparsemux_7_3_8_1_1 RTLNAME blur_edge_sparsemux_7_3_8_1_1 BINDTYPE op TYPE sparsemux IMPL auto}
      {MODELNAME blur_edge_blur_edge_Pipeline_row_loop_col_loop_blur_edge_stream_stream_axis_0_int_int_ibkb RTLNAME blur_edge_blur_edge_Pipeline_row_loop_col_loop_blur_edge_stream_stream_axis_0_int_int_ibkb BINDTYPE storage TYPE ram IMPL auto LATENCY 2 ALLOW_PRAGMA 1}
      {MODELNAME blur_edge_flow_control_loop_pipe_sequential_init RTLNAME blur_edge_flow_control_loop_pipe_sequential_init BINDTYPE interface TYPE internal_upc_flow_control INSTNAME blur_edge_flow_control_loop_pipe_sequential_init_U}
    }
  }
  {SRCNAME blur_edge MODELNAME blur_edge RTLNAME blur_edge IS_TOP 1
    SUBMODULES {
      {MODELNAME blur_edge_mul_32ns_32ns_64_1_1 RTLNAME blur_edge_mul_32ns_32ns_64_1_1 BINDTYPE op TYPE mul IMPL auto LATENCY 0 ALLOW_PRAGMA 1}
      {MODELNAME blur_edge_control_s_axi RTLNAME blur_edge_control_s_axi BINDTYPE interface TYPE interface_s_axilite}
      {MODELNAME blur_edge_regslice_both RTLNAME blur_edge_regslice_both BINDTYPE interface TYPE adapter IMPL reg_slice}
    }
  }
}
