// ==============================================================
// Vitis HLS - High-Level Synthesis from C, C++ and OpenCL v2024.1 (64-bit)
// Tool Version Limit: 2024.05
// Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
// Copyright 2022-2024 Advanced Micro Devices, Inc. All Rights Reserved.
// 
// ==============================================================
#ifndef XBLUR_EDGE_H
#define XBLUR_EDGE_H

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/
#ifndef __linux__
#include "xil_types.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xil_io.h"
#else
#include <stdint.h>
#include <assert.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
#endif
#include "xblur_edge_hw.h"

/**************************** Type Definitions ******************************/
#ifdef __linux__
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#else
typedef struct {
#ifdef SDT
    char *Name;
#else
    u16 DeviceId;
#endif
    u64 Control_BaseAddress;
} XBlur_edge_Config;
#endif

typedef struct {
    u64 Control_BaseAddress;
    u32 IsReady;
} XBlur_edge;

typedef u32 word_type;

/***************** Macros (Inline Functions) Definitions *********************/
#ifndef __linux__
#define XBlur_edge_WriteReg(BaseAddress, RegOffset, Data) \
    Xil_Out32((BaseAddress) + (RegOffset), (u32)(Data))
#define XBlur_edge_ReadReg(BaseAddress, RegOffset) \
    Xil_In32((BaseAddress) + (RegOffset))
#else
#define XBlur_edge_WriteReg(BaseAddress, RegOffset, Data) \
    *(volatile u32*)((BaseAddress) + (RegOffset)) = (u32)(Data)
#define XBlur_edge_ReadReg(BaseAddress, RegOffset) \
    *(volatile u32*)((BaseAddress) + (RegOffset))

#define Xil_AssertVoid(expr)    assert(expr)
#define Xil_AssertNonvoid(expr) assert(expr)

#define XST_SUCCESS             0
#define XST_DEVICE_NOT_FOUND    2
#define XST_OPEN_DEVICE_FAILED  3
#define XIL_COMPONENT_IS_READY  1
#endif

/************************** Function Prototypes *****************************/
#ifndef __linux__
#ifdef SDT
int XBlur_edge_Initialize(XBlur_edge *InstancePtr, UINTPTR BaseAddress);
XBlur_edge_Config* XBlur_edge_LookupConfig(UINTPTR BaseAddress);
#else
int XBlur_edge_Initialize(XBlur_edge *InstancePtr, u16 DeviceId);
XBlur_edge_Config* XBlur_edge_LookupConfig(u16 DeviceId);
#endif
int XBlur_edge_CfgInitialize(XBlur_edge *InstancePtr, XBlur_edge_Config *ConfigPtr);
#else
int XBlur_edge_Initialize(XBlur_edge *InstancePtr, const char* InstanceName);
int XBlur_edge_Release(XBlur_edge *InstancePtr);
#endif

void XBlur_edge_Start(XBlur_edge *InstancePtr);
u32 XBlur_edge_IsDone(XBlur_edge *InstancePtr);
u32 XBlur_edge_IsIdle(XBlur_edge *InstancePtr);
u32 XBlur_edge_IsReady(XBlur_edge *InstancePtr);
void XBlur_edge_EnableAutoRestart(XBlur_edge *InstancePtr);
void XBlur_edge_DisableAutoRestart(XBlur_edge *InstancePtr);

void XBlur_edge_Set_height(XBlur_edge *InstancePtr, u32 Data);
u32 XBlur_edge_Get_height(XBlur_edge *InstancePtr);
void XBlur_edge_Set_width(XBlur_edge *InstancePtr, u32 Data);
u32 XBlur_edge_Get_width(XBlur_edge *InstancePtr);
void XBlur_edge_Set_threshold(XBlur_edge *InstancePtr, u32 Data);
u32 XBlur_edge_Get_threshold(XBlur_edge *InstancePtr);

void XBlur_edge_InterruptGlobalEnable(XBlur_edge *InstancePtr);
void XBlur_edge_InterruptGlobalDisable(XBlur_edge *InstancePtr);
void XBlur_edge_InterruptEnable(XBlur_edge *InstancePtr, u32 Mask);
void XBlur_edge_InterruptDisable(XBlur_edge *InstancePtr, u32 Mask);
void XBlur_edge_InterruptClear(XBlur_edge *InstancePtr, u32 Mask);
u32 XBlur_edge_InterruptGetEnabled(XBlur_edge *InstancePtr);
u32 XBlur_edge_InterruptGetStatus(XBlur_edge *InstancePtr);

#ifdef __cplusplus
}
#endif

#endif
