/*
 * Copyright 2020 Astera Labs, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at:
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

/**
 * @file aries_mpw_reg_defines.h
 * @brief Definition of register offsets used in Aries MPW
 */

#ifndef ASTERA_ARIES_MPW_REG_DEFINES_H
#define ASTERA_ARIES_MPW_REG_DEFINES_H

///////////////////////////////////////////////////////////////
////////////////////////// Main SRAM //////////////////////////
///////////////////////////////////////////////////////////////

/** AL SRAM DMEM offset (MPW) */
#define AL_MAIN_SRAM_DMEM_OFFSET 60 * 1024

/** AL Path SRAM DMEM offset (MPW) */
#define AL_PATH_SRAM_DMEM_OFFSET (45 * 1024)

/** SRAM read command */
#define AL_TG_RD_LOC_IND_SRAM 0x16

/** SRAM write command */
#define AL_TG_WR_LOC_IND_SRAM 0x17

///////////////////////////////////////////////////////////
////////////////////////// Micros /////////////////////////
///////////////////////////////////////////////////////////

/** Offset for main micro FW info */
#define ARIES_MAIN_MICRO_FW_INFO (64 * 1024 - 128)

/** Offset for path micro FW info */
#define ARIES_PATH_MICRO_FW_INFO_ADDRESS (48 * 1024 - 256)

/** Link Struct Base address (Main Micro)*/
#define ARIES_LINK_0_MM_BASE_ADDR 0xF660

///////////////////////////////////////////////////////////
/////////////////// Path Micro Members ////////////////////
///////////////////////////////////////////////////////////

/** FW Info (Major) offset location in struct */
#define ARIES_MM_FW_VERSION_MAJOR 0

/** FW Info (Minor) offset location in struct */
#define ARIES_MM_FW_VERSION_MINOR 1

/** FW Info (Build no.) offset location in struct */
#define ARIES_MM_FW_VERSION_BUILD 2

/** AL print info struct address (Main Micro) */
#define ARIES_MM_AL_PRINT_INFO_STRUCT_ADDR 4

/** Aries Link Struct Addr offset */
#define ARIES_MM_LINK_STRUCT_ADDR_OFFSET 10

/**AL print info struct address (Path Micro) */
#define ARIES_PM_AL_PRINT_INFO_STRUCT_ADDR 4

/** GP Ctrl status struct address (Main Micro) */
#define ARIES_MM_GP_CTRL_STS_STRUCT_ADDR 6

/** GP Ctrl status struct address (Path Micro) */
#define ARIES_PM_GP_CTRL_STS_STRUCT_ADDR 6

/** Offset to enable LTSSM logger */
#define ARIES_PRINT_INFO_STRUCT_PRINT_EN_OFFSET 0

/** Offset to enable one batch mode inside LTSSM logger */
#define ARIES_PRINT_INFO_STRUCT_ONE_BATCH_MODE_EN_OFFSET 2

/** Offset to enable one batch write inside LTSSM logger */
#define ARIES_PRINT_INFO_STRUCT_ONE_BATCH_WR_EN_OFFSET 3

/** Offset to get write_offset inside LTSSM logger */
#define ARIES_PRINT_INFO_STRUCT_WR_PTR_OFFSET 5

/** Offset to get print_class_en inside LTSSM logger for Path Micro */
#define ARIES_PM_PRINT_INFO_STRUCT_PRINT_CLASS_EN_OFFSET 7

/** Offset to get print_class_en inside LTSSM logger for Main Micro */
#define ARIES_MM_PRINT_INFO_STRUCT_PRINT_CLASS_EN_OFFSET 15

/** Offset to get print buffer inside LTSSM logger */
#define ARIES_PRINT_INFO_STRUCT_PRINT_BUFFER_OFFSET 23

/** Offset to get FW state inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_FW_STATE 0

/** Offset to get last speed inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_PCIE_GEN 2

/** Offset to get last preset reqs for lane 0 inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_PRESET_REQS_LN0 5

/** Offset to get last preset reqs for lane 1 inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_PRESET_REQS_LN1 15

/** Offset to get last FOMs for lane 0 inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FOMS_LN0 25

/** Offset to get last FOMs for lane 1 inside GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FOMS_LN1 35

/** Offset to get preset val for lane 0 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PRESET_LN0 45

/** Offset to get preset val for lane 1 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PRESET_LN1 46

/** Offset to get pre cursor val for lane 0 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PRE_LN0 47

/** Offset to get pre cursor val for lane 1 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PRE_LN1 48

/** Offset to get cursor val for lane 0 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_CUR_LN0 49

/** Offset to get cursor val for lane 1 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_CUR_LN1 50

/** Offset to get post cursor val for lane 0 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PST_LN0 51

/** Offset to get post cursor val for lane 1 in GP_CTRL_STS_STRUCT */
#define ARIES_CTRL_STS_STRUCT_LAST_EQ_FINAL_REQ_PST_LN1 52

/** Offset to link width member in link struct*/
#define ARIES_LINK_STRUCT_WIDTH_OFFSET 1

/** Offset to link rate member in link struct*/
#define ARIES_LINK_STRUCT_RATE_OFFSET 6

/** Offset to link state member in link struct*/
#define ARIES_LINK_STRUCT_STATE_OFFSET 10

/** Offset seperating 2 links in Aries Link struct */
#define ARIES_LINK_STRUCT_LINK_SEP_OFFSET 126

///////////////////////////////////////////////////////////
////////////////////// PMA registers //////////////////////
///////////////////////////////////////////////////////////

/** PMA Slice 0 Cmd register address*/
#define ARIES_PMA_QS0_CMD_ADDRESS 0x4400

/** PMA Slice 0 Address_1 register address*/
#define ARIES_PMA_QS0_ADDR_1_ADDRESS 0x4401

/** PMA Slice 0 Address_0 register address*/
#define ARIES_PMA_QS0_ADDR_0_ADDRESS 0x4402

/** PMA Slice 0 Data_0 register address*/
#define ARIES_PMA_QS0_DATA_0_ADDRESS 0x4403

/** PMA Slice 0 Data_1 register address*/
#define ARIES_PMA_QS0_DATA_1_ADDRESS 0x4404

/** Reg offset for PMA reg LANE_DIG_ASIC_RX_OVRD_IN_3 */
#define ARIES_PMA_LANE_DIG_ASIC_RX_OVRD_IN_3 0x101a

/** Reg offset for PMA reg LANE0_DIG_ASIC_RX_ASIC_IN_1 */
#define ARIES_PMA_LANE_DIG_ASIC_RX_ASIC_IN_1 0x1029

/** Reg offset for PMA register LANE0_DIG_RX_ADPTCTL_ATT_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_ATT_STATUS 0x10ab

/** Reg offset for PMA register LANE0_DIG_RX_ADPTCTL_VGA_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_VGA_STATUS 0x10ac

/** Reg offset for PMA register LANE0_DIG_RX_ADPTCTL_CTLE_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_CTLE_STATUS 0x10ad

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP1_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP1_STATUS 0x10ae

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP2_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP2_STATUS 0x10af

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP3_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP3_STATUS 0x10b0

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP4_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP4_STATUS 0x10b1

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP5_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP5_STATUS 0x10b2

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP6_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP6_STATUS 0x10ce

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP7_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP7_STATUS 0x10cf

/** Reg offset for PMA register DIG_RX_ADPTCTL_DFE_TAP8_STATUS */
#define ARIES_PMA_LANE_DIG_RX_ADPTCTL_DFE_TAP8_STATUS 0x10d0

/** Reg offset for PMA register DIG_PCS_XF_RX_OVRD_IN_1 at Lane 0*/
#define ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_1 0x2006

/** Reg offset for PMA register DIG_PCS_XF_RX_OVRD_IN_7 at Lane 0*/
#define ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_7 0x200c

/** Reg offset for PMA register DIG_PCS_XF_RX_OVRD_IN_9 at Lane 0*/
#define ARIES_PMA_RAWLANE_DIG_PCS_XF_RX_OVRD_IN_9 0x203a

///////////////////////////////////////////////////////////
////////////////////// Common offsets /////////////////////
///////////////////////////////////////////////////////////

/** Address offset between between lanes */
#define ARIES_PMA_LANE_STRIDE 0x200

/** Address offset between quad slices */
#define ARIES_QS_STRIDE 0x4000

/** Address offset between between path wrappers */
#define ARIES_PATH_WRP_STRIDE 0x1000

/** Address offset between between path lanes */
#define ARIES_PATH_LANE_STRIDE 0x400

///////////////////////////////////////////////////////////
/////////////////// Hierarchical offsets //////////////////
///////////////////////////////////////////////////////////

////////////////////// AL Top CSRs ///////////////////////

/** AL Misc CSR Offset */
#define ARIES_MISC_CSR_OFFSET 0x0

/** AL Main Micro CSR Offset */
#define ARIES_MAIN_MICRO_CSR_OFFSET 0xc00

/** AL Quad Slice 0 Offset */
#define ARIES_QS_0_CSR_OFFSET 0x4000

/** AL Quad Slice 1 Offset */
#define ARIES_QS_1_CSR_OFFSET 0x8000

/** AL Quad Slice 2 Offset */
#define ARIES_QS_2_CSR_OFFSET 0xc000

/** AL Quad Slice 3 Offset */
#define ARIES_QS_3_CSR_OFFSET 0x10000

/////////////////// AL Quad Slice CSRs  ////////////////////

/** AL Path Wrapper 0 CSR Offset */
#define ARIES_PATH_WRAPPER_0_CSR_OFFSET 0x0

/** AL Path Wrapper 1 CSR Offset */
#define ARIES_PATH_WRAPPER_1_CSR_OFFSET 0x1000

/** AL Path Wrapper 2 CSR Offset */
#define ARIES_PATH_WRAPPER_2_CSR_OFFSET 0x2000

/** AL Path Wrapper 3 CSR Offset */
#define ARIES_PATH_WRAPPER_3_CSR_OFFSET 0x3000

/////////////////// AL Path Wrapper CSRs  ////////////////////

/** Path Micro CSR Offset */
#define ARIES_PATH_MICRO_CSR_OFFSET 0x0

/** Pac CSR Offset */
#define ARIES_PAC_CSR_OFFSET 0x400

/** Path Global CSR Offset */
#define ARIES_PATH_GLOBAL_CSR_OFFSET 0x600

/** AL Path Lane 0 CSR offset */
#define ARIES_PATH_LANE_0_CSR_OFFSET 0x800

/** AL Path Lane 1 CSR offset */
#define ARIES_PATH_LANE_1_CSR_OFFSET 0xc00

/////////////////// AL Path Global CSRs  ////////////////////

/** Reg offset for MAC-to-PHY rate and pclk_rate overrides */
#define ARIES_GBL_CSR_MAC_PHY_RATE_AND_PCLK_RATE 0xe

/////////////////// AL Path Lane X CSRs  ////////////////////

/** Captured Lane number reg offset */
#define ARIES_LN_CAPT_NUM 0x6

/** MAC-to-PHY Tx equalization setting override */
#define ARIES_MAC_PHY_TXDEEMPH 0x120

/** MAC-to-PHY Rx polarity override */
#define ARIES_MAC_RX_POLARITY 0x125

/** Deskew delta in pclk cycles */
#define ARIES_DSK_CC_DELTA 0x154

/** Deskew status */
#define ARIES_DESKEW_STATUS 0x156

/** MAC-to-PHY Tx deemph idle observe */
#define ARIES_MAC_PHY_TXDEEMPH_OB 0x175

#endif
