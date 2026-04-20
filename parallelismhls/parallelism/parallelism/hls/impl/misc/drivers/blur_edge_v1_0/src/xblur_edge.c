// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.1 (64-bit)
// Tool Version Limit: 2024.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
/***************************** Include Files *********************************/
#include "xblur_edge.h"

/************************** Function Implementation *************************/
#ifndef __linux__
int XBlur_edge_CfgInitialize(XBlur_edge *InstancePtr, XBlur_edge_Config *ConfigPtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(ConfigPtr != NULL);

    InstancePtr->Control_BaseAddress = ConfigPtr->Control_BaseAddress;
    InstancePtr->IsReady = XIL_COMPONENT_IS_READY;

    return XST_SUCCESS;
}
#endif

void XBlur_edge_Start(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL) & 0x80;
    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL, Data | 0x01);
}

u32 XBlur_edge_IsDone(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 1) & 0x1;
}

u32 XBlur_edge_IsIdle(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL);
    return (Data >> 2) & 0x1;
}

u32 XBlur_edge_IsReady(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL);
    // check ap_start to see if the pcore is ready for next input
    return !(Data & 0x1);
}

void XBlur_edge_EnableAutoRestart(XBlur_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL, 0x80);
}

void XBlur_edge_DisableAutoRestart(XBlur_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_AP_CTRL, 0);
}

void XBlur_edge_Set_height(XBlur_edge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_HEIGHT_DATA, Data);
}

u32 XBlur_edge_Get_height(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_HEIGHT_DATA);
    return Data;
}

void XBlur_edge_Set_width(XBlur_edge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_WIDTH_DATA, Data);
}

u32 XBlur_edge_Get_width(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_WIDTH_DATA);
    return Data;
}

void XBlur_edge_Set_threshold(XBlur_edge *InstancePtr, u32 Data) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_THRESHOLD_DATA, Data);
}

u32 XBlur_edge_Get_threshold(XBlur_edge *InstancePtr) {
    u32 Data;

    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Data = XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_THRESHOLD_DATA);
    return Data;
}

void XBlur_edge_InterruptGlobalEnable(XBlur_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_GIE, 1);
}

void XBlur_edge_InterruptGlobalDisable(XBlur_edge *InstancePtr) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_GIE, 0);
}

void XBlur_edge_InterruptEnable(XBlur_edge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_IER);
    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_IER, Register | Mask);
}

void XBlur_edge_InterruptDisable(XBlur_edge *InstancePtr, u32 Mask) {
    u32 Register;

    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    Register =  XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_IER);
    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_IER, Register & (~Mask));
}

void XBlur_edge_InterruptClear(XBlur_edge *InstancePtr, u32 Mask) {
    Xil_AssertVoid(InstancePtr != NULL);
    Xil_AssertVoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    XBlur_edge_WriteReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_ISR, Mask);
}

u32 XBlur_edge_InterruptGetEnabled(XBlur_edge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_IER);
}

u32 XBlur_edge_InterruptGetStatus(XBlur_edge *InstancePtr) {
    Xil_AssertNonvoid(InstancePtr != NULL);
    Xil_AssertNonvoid(InstancePtr->IsReady == XIL_COMPONENT_IS_READY);

    return XBlur_edge_ReadReg(InstancePtr->Control_BaseAddress, XBLUR_EDGE_CONTROL_ADDR_ISR);
}

