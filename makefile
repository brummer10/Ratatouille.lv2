
include libxputty/Build/Makefile.base

NOGOAL := install all features mod modapp standalone lv2 lv2log jack clap vst2

SWITCHGOAL := all modapp standalone lv2 lv2log jack clap vst2

PASS := features 

SUBDIR := Ratatouille

.PHONY: $(SUBDIR) libxputty  recurse

$(MAKECMDGOALS) recurse: $(SUBDIR)

check-and-reinit-submodules :
ifeq (,$(filter $(NOGOAL),$(MAKECMDGOALS)))
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
	@if git submodule status 2>/dev/null | egrep -q '^[-]|^[+]' ; then \
		echo "$(red)INFO: Need to reinitialize git submodules$(reset)"; \
		git submodule update --init; \
		echo "$(blue)Done$(reset)"; \
	else echo "$(blue) Submodule up to date$(reset)"; \
	fi
endif
endif

libxputty: check-and-reinit-submodules
ifeq (,$(filter $(NOGOAL),$(MAKECMDGOALS)))
ifeq (,$(wildcard ./libxputty/xputty/resources/texture.png))
	@cp ./Ratatouille/resources/*.png ./libxputty/xputty/resources/
endif
	@exec $(MAKE) --no-print-directory -j 1 -C $@ $(MAKECMDGOALS)
endif
ifneq (,$(filter $(SWITCHGOAL),$(MAKECMDGOALS)))
ifeq (,$(wildcard ./libxputty/xputty/resources/texture.png))
	@cp ./Ratatouille/resources/*.png ./libxputty/xputty/resources/
endif
	@exec $(MAKE) --no-print-directory -j 1 -C $@ all
endif


$(SUBDIR): libxputty
ifeq (,$(filter $(PASS),$(MAKECMDGOALS)))
	@exec $(MAKE) --no-print-directory -j 1 -C $@ $(MAKECMDGOALS)
endif

clean:
	@rm -f ./libxputty/xputty/resources/texture.png
	@rm -f ./libxputty/xputty/resources/wavdir.png
	@rm -f ./libxputty/xputty/resources/neuraldir.png
	@rm -f ./libxputty/xputty/resources/norm.png
	@rm -f ./libxputty/xputty/resources/eject.png

features:
