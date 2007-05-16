/*******************************************************************************

  Copyright (c) 2001-2007, Intel Corporation 
  All rights reserved.
  
  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions are met:
  
   1. Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
  
   2. Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in the 
      documentation and/or other materials provided with the distribution.
  
   3. Neither the name of the Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.
  
  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/
/*$FreeBSD$*/


/* e1000_82540
 * e1000_82545
 * e1000_82546
 * e1000_82545_rev_3
 * e1000_82546_rev_3
 */

#include "e1000_api.h"

void e1000_init_function_pointers_82540(struct e1000_hw *hw);

STATIC s32  e1000_init_phy_params_82540(struct e1000_hw *hw);
STATIC s32  e1000_init_nvm_params_82540(struct e1000_hw *hw);
STATIC s32  e1000_init_mac_params_82540(struct e1000_hw *hw);
static s32  e1000_adjust_serdes_amplitude_82540(struct e1000_hw *hw);
STATIC void e1000_clear_hw_cntrs_82540(struct e1000_hw *hw);
STATIC s32  e1000_init_hw_82540(struct e1000_hw *hw);
STATIC s32  e1000_reset_hw_82540(struct e1000_hw *hw);
static s32  e1000_set_phy_mode_82540(struct e1000_hw *hw);
static s32  e1000_set_vco_speed_82540(struct e1000_hw *hw);
STATIC s32  e1000_setup_copper_link_82540(struct e1000_hw *hw);
STATIC s32  e1000_setup_fiber_serdes_link_82540(struct e1000_hw *hw);

/**
 * e1000_init_phy_params_82540 - Init PHY func ptrs.
 * @hw: pointer to the HW structure
 *
 * This is a function pointer entry point called by the api module.
 **/
STATIC s32
e1000_init_phy_params_82540(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	struct e1000_functions *func = &hw->func;
	s32 ret_val = E1000_SUCCESS;

	phy->addr                       = 1;
	phy->autoneg_mask               = AUTONEG_ADVERTISE_SPEED_DEFAULT;
	phy->reset_delay_us             = 10000;
	phy->type                       = e1000_phy_m88;

	/* Function Pointers */
	func->check_polarity            = e1000_check_polarity_m88;
	func->commit_phy                = e1000_phy_sw_reset_generic;
	func->force_speed_duplex        = e1000_phy_force_speed_duplex_m88;
	func->get_cable_length          = e1000_get_cable_length_m88;
	func->get_cfg_done              = e1000_get_cfg_done_generic;
	func->read_phy_reg              = e1000_read_phy_reg_m88;
	func->reset_phy                 = e1000_phy_hw_reset_generic;
	func->write_phy_reg             = e1000_write_phy_reg_m88;
	func->get_phy_info              = e1000_get_phy_info_m88;

	ret_val = e1000_get_phy_id(hw);
	if (ret_val)
		goto out;

	/* Verify phy id */
	switch (hw->mac.type) {
	case e1000_82540:
	case e1000_82545:
	case e1000_82545_rev_3:
	case e1000_82546:
	case e1000_82546_rev_3:
		if (phy->id == M88E1011_I_PHY_ID)
			break;
		/* Fall Through */
	default:
		ret_val = -E1000_ERR_PHY;
		goto out;
		break;
	}

out:
	return ret_val;
}

/**
 * e1000_init_nvm_params_82540 - Init NVM func ptrs.
 * @hw: pointer to the HW structure
 *
 * This is a function pointer entry point called by the api module.
 **/
STATIC s32
e1000_init_nvm_params_82540(struct e1000_hw *hw)
{
	struct e1000_nvm_info *nvm = &hw->nvm;
	struct e1000_functions *func = &hw->func;
	u32 eecd = E1000_READ_REG(hw, E1000_EECD);

	DEBUGFUNC("e1000_init_nvm_params_82540");

	nvm->type               = e1000_nvm_eeprom_microwire;
	nvm->delay_usec         = 50;
	nvm->opcode_bits        = 3;
	switch (nvm->override) {
	case e1000_nvm_override_microwire_large:
		nvm->address_bits       = 8;
		nvm->word_size          = 256;
		break;
	case e1000_nvm_override_microwire_small:
		nvm->address_bits       = 6;
		nvm->word_size          = 64;
		break;
	default:
		nvm->address_bits       = eecd & E1000_EECD_SIZE ? 8 : 6;
		nvm->word_size          = eecd & E1000_EECD_SIZE ? 256 : 64;
		break;
	}

	/* Function Pointers */
	func->acquire_nvm        = e1000_acquire_nvm_generic;
	func->read_nvm           = e1000_read_nvm_microwire;
	func->release_nvm        = e1000_release_nvm_generic;
	func->update_nvm         = e1000_update_nvm_checksum_generic;
	func->valid_led_default  = e1000_valid_led_default_generic;
	func->validate_nvm       = e1000_validate_nvm_checksum_generic;
	func->write_nvm          = e1000_write_nvm_microwire;

	return E1000_SUCCESS;
}

/**
 * e1000_init_mac_params_82540 - Init MAC func ptrs.
 * @hw: pointer to the HW structure
 *
 * This is a function pointer entry point called by the api module.
 **/
STATIC s32
e1000_init_mac_params_82540(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	struct e1000_functions *func = &hw->func;
	s32 ret_val = E1000_SUCCESS;

	DEBUGFUNC("e1000_init_mac_params_82540");

	/* Set media type */
	switch (hw->device_id) {
	case E1000_DEV_ID_82545EM_FIBER:
	case E1000_DEV_ID_82545GM_FIBER:
	case E1000_DEV_ID_82546EB_FIBER:
	case E1000_DEV_ID_82546GB_FIBER:
		hw->media_type = e1000_media_type_fiber;
		break;
	case E1000_DEV_ID_82545GM_SERDES:
	case E1000_DEV_ID_82546GB_SERDES:
		hw->media_type = e1000_media_type_internal_serdes;
		break;
	default:
		hw->media_type = e1000_media_type_copper;
		break;
	}

	/* Set mta register count */
	mac->mta_reg_count = 128;
	/* Set rar entry count */
	mac->rar_entry_count = E1000_RAR_ENTRIES;

	/* Function pointers */

	/* bus type/speed/width */
	func->get_bus_info = e1000_get_bus_info_pci_generic;
	/* reset */
	func->reset_hw = e1000_reset_hw_82540;
	/* hw initialization */
	func->init_hw = e1000_init_hw_82540;
	/* link setup */
	func->setup_link = e1000_setup_link_generic;
	/* physical interface setup */
	func->setup_physical_interface =
	        (hw->media_type == e1000_media_type_copper)
	                ? e1000_setup_copper_link_82540
	                : e1000_setup_fiber_serdes_link_82540;
	/* check for link */
	switch (hw->media_type) {
	case e1000_media_type_copper:
		func->check_for_link = e1000_check_for_copper_link_generic;
		break;
	case e1000_media_type_fiber:
		func->check_for_link = e1000_check_for_fiber_link_generic;
		break;
	case e1000_media_type_internal_serdes:
		func->check_for_link = e1000_check_for_serdes_link_generic;
		break;
	default:
		ret_val = -E1000_ERR_CONFIG;
		goto out;
		break;
	}
	/* link info */
	func->get_link_up_info =
	        (hw->media_type == e1000_media_type_copper)
	                ? e1000_get_speed_and_duplex_copper_generic
	                : e1000_get_speed_and_duplex_fiber_serdes_generic;
	/* multicast address update */
	func->mc_addr_list_update = e1000_mc_addr_list_update_generic;
	/* writing VFTA */
	func->write_vfta = e1000_write_vfta_generic;
	/* clearing VFTA */
	func->clear_vfta = e1000_clear_vfta_generic;
	/* setting MTA */
	func->mta_set = e1000_mta_set_generic;
	/* setup LED */
	func->setup_led = e1000_setup_led_generic;
	/* cleanup LED */
	func->cleanup_led = e1000_cleanup_led_generic;
	/* turn on/off LED */
	func->led_on = e1000_led_on_generic;
	func->led_off = e1000_led_off_generic;
	/* clear hardware counters */
	func->clear_hw_cntrs = e1000_clear_hw_cntrs_82540;

out:
	return ret_val;
}

/**
 * e1000_init_function_pointers_82540 - Init func ptrs.
 * @hw: pointer to the HW structure
 *
 * The only function explicitly called by the api module to initialize
 * all function pointers and parameters.
 **/
void
e1000_init_function_pointers_82540(struct e1000_hw *hw)
{
	DEBUGFUNC("e1000_init_function_pointers_82540");

	hw->func.init_mac_params = e1000_init_mac_params_82540;
	hw->func.init_nvm_params = e1000_init_nvm_params_82540;
	hw->func.init_phy_params = e1000_init_phy_params_82540;
}

/**
 *  e1000_reset_hw_82540 - Reset hardware
 *  @hw: pointer to the HW structure
 *
 *  This resets the hardware into a known state.  This is a
 *  function pointer entry point called by the api module.
 **/
STATIC s32
e1000_reset_hw_82540(struct e1000_hw *hw)
{
	u32 ctrl, icr, manc;
	s32 ret_val = E1000_SUCCESS;

	DEBUGFUNC("e1000_reset_hw_82540");

	DEBUGOUT("Masking off all interrupts\n");
	E1000_WRITE_REG(hw, E1000_IMC, 0xFFFFFFFF);

	E1000_WRITE_REG(hw, E1000_RCTL, 0);
	E1000_WRITE_REG(hw, E1000_TCTL, E1000_TCTL_PSP);
	E1000_WRITE_FLUSH(hw);

	/* Delay to allow any outstanding PCI transactions to complete
	 * before resetting the device.
	 */
	msec_delay(10);

	ctrl = E1000_READ_REG(hw, E1000_CTRL);

	DEBUGOUT("Issuing a global reset to 82540/82545/82546 MAC\n");
	switch (hw->mac.type) {
	case e1000_82545_rev_3:
	case e1000_82546_rev_3:
		E1000_WRITE_REG(hw, E1000_CTRL_DUP, ctrl | E1000_CTRL_RST);
		break;
	default:
		/* These controllers can't ack the 64-bit write when
		 * issuing the reset, so we use IO-mapping as a
		 * workaround to issue the reset.
		 */
		E1000_WRITE_REG_IO(hw, E1000_CTRL, ctrl | E1000_CTRL_RST);
		break;
	}

	/* Wait for EEPROM reload */
	msec_delay(5);

	/* Disable HW ARPs on ASF enabled adapters */
	manc = E1000_READ_REG(hw, E1000_MANC);
	manc &= ~E1000_MANC_ARP_EN;
	E1000_WRITE_REG(hw, E1000_MANC, manc);

	E1000_WRITE_REG(hw, E1000_IMC, 0xffffffff);
	icr = E1000_READ_REG(hw, E1000_ICR);

	return ret_val;
}

/**
 *  e1000_init_hw_82540 - Initialize hardware
 *  @hw: pointer to the HW structure
 *
 *  This inits the hardware readying it for operation.  This is a
 *  function pointer entry point called by the api module.
 **/
STATIC s32
e1000_init_hw_82540(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	u32 txdctl, ctrl_ext;
	s32 ret_val = E1000_SUCCESS;
	u16 i;

	DEBUGFUNC("e1000_init_hw_82540");

	/* Initialize identification LED */
	ret_val = e1000_id_led_init_generic(hw);
	if (ret_val) {
		DEBUGOUT("Error initializing identification LED\n");
		goto out;
	}

	/* Disabling VLAN filtering */
	DEBUGOUT("Initializing the IEEE VLAN\n");
	if (mac->type < e1000_82545_rev_3) {
		E1000_WRITE_REG(hw, E1000_VET, 0);
	}
	e1000_clear_vfta(hw);

	/* Setup the receive address. */
	e1000_init_rx_addrs_generic(hw, mac->rar_entry_count);

	/* Zero out the Multicast HASH table */
	DEBUGOUT("Zeroing the MTA\n");
	for (i = 0; i < mac->mta_reg_count; i++) {
		E1000_WRITE_REG_ARRAY(hw, E1000_MTA, i, 0);
		/* Avoid back to back register writes by adding the register
		 * read (flush).  This is to protect against some strange
		 * bridge configurations that may issue Memory Write Block
		 * (MWB) to our register space.  The *_rev_3 hardware at
		 * least doesn't respond correctly to every other dword in an
		 * MWB to our register space.
		 */
		E1000_WRITE_FLUSH(hw);
	}

	if (mac->type < e1000_82545_rev_3)
		e1000_pcix_mmrbc_workaround_generic(hw);

	/* Setup link and flow control */
	ret_val = e1000_setup_link(hw);

	txdctl = E1000_READ_REG(hw, E1000_TXDCTL);
	txdctl = (txdctl & ~E1000_TXDCTL_WTHRESH) |
	         E1000_TXDCTL_FULL_TX_DESC_WB;
	E1000_WRITE_REG(hw, E1000_TXDCTL, txdctl);

	/* Clear all of the statistics registers (clear on read).  It is
	 * important that we do this after we have tried to establish link
	 * because the symbol error count will increment wildly if there
	 * is no link.
	 */
	e1000_clear_hw_cntrs_82540(hw);

	if ((hw->device_id == E1000_DEV_ID_82546GB_QUAD_COPPER) ||
	    (hw->device_id == E1000_DEV_ID_82546GB_QUAD_COPPER_KSP3)) {
		ctrl_ext = E1000_READ_REG(hw, E1000_CTRL_EXT);
		/* Relaxed ordering must be disabled to avoid a parity
		 * error crash in a PCI slot. */
		ctrl_ext |= E1000_CTRL_EXT_RO_DIS;
		E1000_WRITE_REG(hw, E1000_CTRL_EXT, ctrl_ext);
	}

out:
	return ret_val;
}

/**
 *  e1000_setup_copper_link_82540 - Configure copper link settings
 *  @hw: pointer to the HW structure
 *
 *  Calls the appropriate function to configure the link for auto-neg or forced
 *  speed and duplex.  Then we check for link, once link is established calls
 *  to configure collision distance and flow control are called.  If link is
 *  not established, we return -E1000_ERR_PHY (-2).  This is a function
 *  pointer entry point called by the api module.
 **/
STATIC s32
e1000_setup_copper_link_82540(struct e1000_hw *hw)
{
	u32 ctrl;
	s32 ret_val = E1000_SUCCESS;
	u16 data;

	DEBUGFUNC("e1000_setup_copper_link_82540");

	ctrl = E1000_READ_REG(hw, E1000_CTRL);
	ctrl |= E1000_CTRL_SLU;
	ctrl &= ~(E1000_CTRL_FRCSPD | E1000_CTRL_FRCDPX);
	E1000_WRITE_REG(hw, E1000_CTRL, ctrl);

	ret_val = e1000_set_phy_mode_82540(hw);
	if (ret_val)
		goto out;

	if (hw->mac.type == e1000_82545_rev_3 ||
	    hw->mac.type == e1000_82546_rev_3) {
		ret_val = e1000_read_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, &data);
		if (ret_val)
			goto out;
		data |= 0x00000008;
		ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_SPEC_CTRL, data);
		if (ret_val)
			goto out;
	}

	ret_val = e1000_copper_link_setup_m88(hw);
	if (ret_val)
		goto out;

	ret_val = e1000_setup_copper_link_generic(hw);

out:
	return ret_val;
}

/**
 *  e1000_setup_fiber_serdes_link_82540 - Setup link for fiber/serdes
 *  @hw: pointer to the HW structure
 *
 *  Set the output amplitude to the value in the EEPROM and adjust the VCO
 *  speed to improve Bit Error Rate (BER) performance.  Configures collision
 *  distance and flow control for fiber and serdes links.  Upon successful
 *  setup, poll for link.  This is a function pointer entry point called by
 *  the api module.
 **/
STATIC s32
e1000_setup_fiber_serdes_link_82540(struct e1000_hw *hw)
{
	struct e1000_mac_info *mac = &hw->mac;
	s32 ret_val = E1000_SUCCESS;

	DEBUGFUNC("e1000_setup_fiber_serdes_link_82540");

	switch (mac->type) {
	case e1000_82545_rev_3:
	case e1000_82546_rev_3:
		if (hw->media_type == e1000_media_type_internal_serdes) {
			/* If we're on serdes media, adjust the output
			 * amplitude to value set in the EEPROM.
			 */
			ret_val = e1000_adjust_serdes_amplitude_82540(hw);
			if (ret_val)
				goto out;
		}
		/* Adjust VCO speed to improve BER performance */
		ret_val = e1000_set_vco_speed_82540(hw);
		if (ret_val)
			goto out;
	default:
		break;
	}

	ret_val = e1000_setup_fiber_serdes_link_generic(hw);

out:
	return ret_val;
}

/**
 *  e1000_adjust_serdes_amplitude_82540 - Adjust amplitude based on EEPROM
 *  @hw: pointer to the HW structure
 *
 *  Adjust the SERDES ouput amplitude based on the EEPROM settings.
 **/
static s32
e1000_adjust_serdes_amplitude_82540(struct e1000_hw *hw)
{
	s32 ret_val = E1000_SUCCESS;
	u16 nvm_data;

	DEBUGFUNC("e1000_adjust_serdes_amplitude_82540");

	ret_val = e1000_read_nvm(hw, NVM_SERDES_AMPLITUDE, 1, &nvm_data);
	if (ret_val) {
		goto out;
	}

	if (nvm_data != NVM_RESERVED_WORD) {
		/* Adjust serdes output amplitude only. */
		nvm_data &= NVM_SERDES_AMPLITUDE_MASK;
		ret_val = e1000_write_phy_reg(hw,
		                             M88E1000_PHY_EXT_CTRL,
		                             nvm_data);
		if (ret_val)
			goto out;
	}

out:
	return ret_val;
}

/**
 *  e1000_set_vco_speed_82540 - Set VCO speed for better performance
 *  @hw: pointer to the HW structure
 *
 *  Set the VCO speed to improve Bit Error Rate (BER) performance.
 **/
static s32
e1000_set_vco_speed_82540(struct e1000_hw *hw)
{
	s32  ret_val = E1000_SUCCESS;
	u16 default_page = 0;
	u16 phy_data;

	DEBUGFUNC("e1000_set_vco_speed_82540");

	/* Set PHY register 30, page 5, bit 8 to 0 */

	ret_val = e1000_read_phy_reg(hw,
	                            M88E1000_PHY_PAGE_SELECT,
	                            &default_page);
	if (ret_val)
		goto out;

	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_PAGE_SELECT, 0x0005);
	if (ret_val)
		goto out;

	ret_val = e1000_read_phy_reg(hw, M88E1000_PHY_GEN_CONTROL, &phy_data);
	if (ret_val)
		goto out;

	phy_data &= ~M88E1000_PHY_VCO_REG_BIT8;
	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_GEN_CONTROL, phy_data);
	if (ret_val)
		goto out;

	/* Set PHY register 30, page 4, bit 11 to 1 */

	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_PAGE_SELECT, 0x0004);
	if (ret_val)
		goto out;

	ret_val = e1000_read_phy_reg(hw, M88E1000_PHY_GEN_CONTROL, &phy_data);
	if (ret_val)
		goto out;

	phy_data |= M88E1000_PHY_VCO_REG_BIT11;
	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_GEN_CONTROL, phy_data);
	if (ret_val)
		goto out;

	ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_PAGE_SELECT,
	                              default_page);

out:
	return ret_val;
}

/**
 *  e1000_set_phy_mode_82540 - Set PHY to class A mode
 *  @hw: pointer to the HW structure
 *
 *  Sets the PHY to class A mode and assumes the following operations will
 *  follow to enable the new class mode:
 *    1.  Do a PHY soft reset.
 *    2.  Restart auto-negotiation or force link.
 **/
static s32
e1000_set_phy_mode_82540(struct e1000_hw *hw)
{
	struct e1000_phy_info *phy = &hw->phy;
	s32 ret_val = E1000_SUCCESS;
	u16 nvm_data;

	DEBUGFUNC("e1000_set_phy_mode_82540");

	if (hw->mac.type != e1000_82545_rev_3)
		goto out;

	ret_val = e1000_read_nvm(hw, NVM_PHY_CLASS_WORD, 1, &nvm_data);
	if (ret_val) {
		ret_val = -E1000_ERR_PHY;
		goto out;
	}

	if ((nvm_data != NVM_RESERVED_WORD) && (nvm_data & NVM_PHY_CLASS_A)) {
		ret_val = e1000_write_phy_reg(hw, M88E1000_PHY_PAGE_SELECT,
					      0x000B);
		if (ret_val) {
			ret_val = -E1000_ERR_PHY;
			goto out;
		}
		ret_val = e1000_write_phy_reg(hw,
					     M88E1000_PHY_GEN_CONTROL,
					     0x8104);
		if (ret_val) {
			ret_val = -E1000_ERR_PHY;
			goto out;
		}

		phy->reset_disable = FALSE;
	}

out:
	return ret_val;
}

/**
 *  e1000_clear_hw_cntrs_82540 - Clear device specific hardware counters
 *  @hw: pointer to the HW structure
 *
 *  Clears the hardware counters by reading the counter registers.
 **/
STATIC void
e1000_clear_hw_cntrs_82540(struct e1000_hw *hw)
{
	volatile u32 temp;

	DEBUGFUNC("e1000_clear_hw_cntrs_82540");

	e1000_clear_hw_cntrs_base_generic(hw);

	temp = E1000_READ_REG(hw, E1000_PRC64);
	temp = E1000_READ_REG(hw, E1000_PRC127);
	temp = E1000_READ_REG(hw, E1000_PRC255);
	temp = E1000_READ_REG(hw, E1000_PRC511);
	temp = E1000_READ_REG(hw, E1000_PRC1023);
	temp = E1000_READ_REG(hw, E1000_PRC1522);
	temp = E1000_READ_REG(hw, E1000_PTC64);
	temp = E1000_READ_REG(hw, E1000_PTC127);
	temp = E1000_READ_REG(hw, E1000_PTC255);
	temp = E1000_READ_REG(hw, E1000_PTC511);
	temp = E1000_READ_REG(hw, E1000_PTC1023);
	temp = E1000_READ_REG(hw, E1000_PTC1522);

	temp = E1000_READ_REG(hw, E1000_ALGNERRC);
	temp = E1000_READ_REG(hw, E1000_RXERRC);
	temp = E1000_READ_REG(hw, E1000_TNCRS);
	temp = E1000_READ_REG(hw, E1000_CEXTERR);
	temp = E1000_READ_REG(hw, E1000_TSCTC);
	temp = E1000_READ_REG(hw, E1000_TSCTFC);

	temp = E1000_READ_REG(hw, E1000_MGTPRC);
	temp = E1000_READ_REG(hw, E1000_MGTPDC);
	temp = E1000_READ_REG(hw, E1000_MGTPTC);
}

