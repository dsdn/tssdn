SIM_BOTH = sender_receiver
SIM_SEND = sender
SIM_RECV = receiver

.DEFAULT_GOAL := all
MY_TARGETS = $(SIM_BOTH) $(SIM_SEND) $(SIM_RECV)

.PHONY: $(SIM_BOTH)
$(SIM_BOTH):
	@$(MAKE) -C $@;

.PHONY: $(SIM_SEND)
$(SIM_SEND):
	@$(MAKE) -C $@;

.PHONY: $(SIM_RECV)
$(SIM_RECV):
	@$(MAKE) -C $@;

.PHONY: all 
all: $(MY_TARGETS)

.PHONY: clean
clean: 
	rm -f common/*.o common/*~ *~ 
	cd $(SIM_BOTH) && $(MAKE) clean
	cd $(SIM_SEND) && $(MAKE) clean
	cd $(SIM_RECV) && $(MAKE) clean
