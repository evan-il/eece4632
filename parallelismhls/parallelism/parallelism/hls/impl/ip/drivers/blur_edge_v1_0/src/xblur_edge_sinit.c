// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.1 (64-bit)
// Tool Version Limit: 2024.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef __linux__

#include "xstatus.h"
#ifdef SDT
#include "xparameters.h"
#endif
#include "xblur_edge.h"

extern XBlur_edge_Config XBlur_edge_ConfigTable[];

#ifdef SDT
XBlur_edge_Config *XBlur_edge_LookupConfig(UINTPTR BaseAddress) {
	XBlur_edge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = (u32)0x0; XBlur_edge_ConfigTable[Index].Name != NULL; Index++) {
		if (!BaseAddress || XBlur_edge_ConfigTable[Index].Control_BaseAddress == BaseAddress) {
			ConfigPtr = &XBlur_edge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XBlur_edge_Initialize(XBlur_edge *InstancePtr, UINTPTR BaseAddress) {
	XBlur_edge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XBlur_edge_LookupConfig(BaseAddress);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XBlur_edge_CfgInitialize(InstancePtr, ConfigPtr);
}
#else
XBlur_edge_Config *XBlur_edge_LookupConfig(u16 DeviceId) {
	XBlur_edge_Config *ConfigPtr = NULL;

	int Index;

	for (Index = 0; Index < XPAR_XBLUR_EDGE_NUM_INSTANCES; Index++) {
		if (XBlur_edge_ConfigTable[Index].DeviceId == DeviceId) {
			ConfigPtr = &XBlur_edge_ConfigTable[Index];
			break;
		}
	}

	return ConfigPtr;
}

int XBlur_edge_Initialize(XBlur_edge *InstancePtr, u16 DeviceId) {
	XBlur_edge_Config *ConfigPtr;

	Xil_AssertNonvoid(InstancePtr != NULL);

	ConfigPtr = XBlur_edge_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		InstancePtr->IsReady = 0;
		return (XST_DEVICE_NOT_FOUND);
	}

	return XBlur_edge_CfgInitialize(InstancePtr, ConfigPtr);
}
#endif

#endif

