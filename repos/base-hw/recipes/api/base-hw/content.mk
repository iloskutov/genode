FROM_BASE_HW := etc include

content: $(FROM_BASE_HW) LICENSE

#
# The Cpu_control session has an archive of its own
# (api/cpu_control_session)
#
$(FROM_BASE_HW):
	$(mirror_from_rep_dir)
	rm -rf include/cpu_control_session

LICENSE:
	cp $(GENODE_DIR)/LICENSE $@
