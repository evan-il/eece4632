// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// --------------------------------------------------------------------------------
// Tool Version: Vivado v.2024.1 (win64) Build 5076996 Wed May 22 18:37:14 MDT 2024
// Date        : Wed Apr 22 16:12:44 2026
// Host        : WFXA4BB6DBB317A running 64-bit major release  (build 9200)
// Command     : write_verilog -force -mode synth_stub
//               c:/Users/gao.hen/Desktop/parallelism/eece4632/parallelismvivado/parallelism/parallelism.gen/sources_1/bd/design_1/ip/design_1_blur_edge_0_0/design_1_blur_edge_0_0_stub.v
// Design      : design_1_blur_edge_0_0
// Purpose     : Stub declaration of top-level module interface
// Device      : xczu3eg-sfvc784-2-e
// --------------------------------------------------------------------------------

// This empty module with port declaration file causes synthesis tools to infer a black box for IP.
// The synthesis directives are for Synopsys Synplify support to prevent IO buffer insertion.
// Please paste the declaration into a Verilog source file or add the file as an additional source.
(* X_CORE_INFO = "blur_edge,Vivado 2024.1" *)
module design_1_blur_edge_0_0(s_axi_control_ARADDR, 
  s_axi_control_ARREADY, s_axi_control_ARVALID, s_axi_control_AWADDR, 
  s_axi_control_AWREADY, s_axi_control_AWVALID, s_axi_control_BREADY, 
  s_axi_control_BRESP, s_axi_control_BVALID, s_axi_control_RDATA, s_axi_control_RREADY, 
  s_axi_control_RRESP, s_axi_control_RVALID, s_axi_control_WDATA, s_axi_control_WREADY, 
  s_axi_control_WSTRB, s_axi_control_WVALID, ap_clk, ap_rst_n, interrupt, src_TDATA, src_TDEST, 
  src_TID, src_TKEEP, src_TLAST, src_TREADY, src_TSTRB, src_TUSER, src_TVALID, dst_TDATA, 
  dst_TDEST, dst_TID, dst_TKEEP, dst_TLAST, dst_TREADY, dst_TSTRB, dst_TUSER, dst_TVALID)
/* synthesis syn_black_box black_box_pad_pin="s_axi_control_ARADDR[5:0],s_axi_control_ARREADY,s_axi_control_ARVALID,s_axi_control_AWADDR[5:0],s_axi_control_AWREADY,s_axi_control_AWVALID,s_axi_control_BREADY,s_axi_control_BRESP[1:0],s_axi_control_BVALID,s_axi_control_RDATA[31:0],s_axi_control_RREADY,s_axi_control_RRESP[1:0],s_axi_control_RVALID,s_axi_control_WDATA[31:0],s_axi_control_WREADY,s_axi_control_WSTRB[3:0],s_axi_control_WVALID,ap_rst_n,interrupt,src_TDATA[7:0],src_TDEST[0:0],src_TID[0:0],src_TKEEP[0:0],src_TLAST[0:0],src_TREADY,src_TSTRB[0:0],src_TUSER[0:0],src_TVALID,dst_TDATA[7:0],dst_TDEST[0:0],dst_TID[0:0],dst_TKEEP[0:0],dst_TLAST[0:0],dst_TREADY,dst_TSTRB[0:0],dst_TUSER[0:0],dst_TVALID" */
/* synthesis syn_force_seq_prim="ap_clk" */;
  input [5:0]s_axi_control_ARADDR;
  output s_axi_control_ARREADY;
  input s_axi_control_ARVALID;
  input [5:0]s_axi_control_AWADDR;
  output s_axi_control_AWREADY;
  input s_axi_control_AWVALID;
  input s_axi_control_BREADY;
  output [1:0]s_axi_control_BRESP;
  output s_axi_control_BVALID;
  output [31:0]s_axi_control_RDATA;
  input s_axi_control_RREADY;
  output [1:0]s_axi_control_RRESP;
  output s_axi_control_RVALID;
  input [31:0]s_axi_control_WDATA;
  output s_axi_control_WREADY;
  input [3:0]s_axi_control_WSTRB;
  input s_axi_control_WVALID;
  input ap_clk /* synthesis syn_isclock = 1 */;
  input ap_rst_n;
  output interrupt;
  input [7:0]src_TDATA;
  input [0:0]src_TDEST;
  input [0:0]src_TID;
  input [0:0]src_TKEEP;
  input [0:0]src_TLAST;
  output src_TREADY;
  input [0:0]src_TSTRB;
  input [0:0]src_TUSER;
  input src_TVALID;
  output [7:0]dst_TDATA;
  output [0:0]dst_TDEST;
  output [0:0]dst_TID;
  output [0:0]dst_TKEEP;
  output [0:0]dst_TLAST;
  input dst_TREADY;
  output [0:0]dst_TSTRB;
  output [0:0]dst_TUSER;
  output dst_TVALID;
endmodule
