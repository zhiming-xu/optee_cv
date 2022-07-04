CFG_TEE_TA_LOG_LEVEL ?= 4
CPPFLAGS += -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL)
LDFLAGS += -static

# The UUID for the Trusted Application
BINARY=12345678-5b69-11e4-9dbb-101f74f000ff

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk
ldargs-$(binary).elf += m

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
