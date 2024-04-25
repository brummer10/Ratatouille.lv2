
	# check on which OS we build
	UNAME_S := $(shell uname -s)
	# check if user is root
	user = $(shell whoami)
	ifeq ($(user),root)
	INSTALL_DIR = /usr/lib/lv2
	else 
	INSTALL_DIR = ~/.lv2
	endif

# check CPU and supported optimization flags
ifneq ($(UNAME_S), FreeBSD)
  ifneq ($(shell cat /proc/cpuinfo | grep sse3 ) , )
    SSE_CFLAGS = -msse3 -mfpmath=sse -mfxsr
  else ifneq ($(shell cat /proc/cpuinfo | grep sse2 ) , )
    SSE_CFLAGS = -msse2 -mfpmath=sse -mfxsr
  else ifneq ($(shell cat /proc/cpuinfo | grep sse ) , )
    SSE_CFLAGS = -msse -mfpmath=sse -mfxsr
  else ifneq ($(shell cat /proc/cpuinfo | grep ARM ) , )
    ifneq ($(shell cat /proc/cpuinfo | grep ARMv7 ) , )
      ifneq ($(shell cat /proc/cpuinfo | grep vfpd32 ) , )
        SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3 
      else ifneq ($(shell cat /proc/cpuinfo | grep vfpv3 ) , )
        SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3
      endif
    else
      ARMCPU = "YES"
    endif
  else
    SSE_CFLAGS =
  endif
else
  ifneq ($(shell dmidecode -t processor -t cache | grep SSE3 ) , )
    SSE_CFLAGS = -msse3 -mfpmath=sse -mfxsr
  else ifneq ($(shell dmidecode -t processor -t cache | grep SSE2 ) , )
    SSE_CFLAGS = -msse2 -mfpmath=sse -mfxsr
  else ifneq ($(shell dmidecode -t processor -t cache | grep SSE ) , )
    SSE_CFLAGS = -msse -mfpmath=sse -mfxsr
  endif
endif

ifneq ($(UNAME_S), FreeBSD)
ifneq ($(MAKECMDGOALS),install)
  ifneq ($(MAKECMDGOALS),clean)
    ifeq ($(shell arch), x86_64)
      ifneq ($(shell /usr/lib64/ld-linux-x86-64.so.2 --help 2>/dev/null | grep 'x86-64-v3 (supported, searched)') , )
        CXXFLAGS += -march=x86-64-v3
        $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=x86-64-v3 $(SSE_CFLAGS)$(reset))
      else ifneq ($(shell /usr/lib64/ld-linux-x86-64.so.2 --help 2>/dev/null | grep 'x86-64-v2 (supported, searched)') , )
        CXXFLAGS += -march=x86-64-v2
        $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=x86-64-v2 $(SSE_CFLAGS)$(reset))
      else
        CXXFLAGS += -march=x86-64
        $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=x86_64 $(SSE_CFLAGS)$(reset))
      endif
    else ifeq ($(shell arch), aarch64)
      CXXFLAGS += -march=armv8
      $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=armv8 $(SSE_CFLAGS)$(reset))
    endif
  endif
endif
endif

ifeq ($(UNAME_S), Linux)
  ifeq ($(TARGET), Linux)
    CXX_VERSION = $(shell clang++ --version 2>/dev/null)
    CXXFLAGS:=$(filter-out -fno-gnu-unique,$(CXXFLAGS))
    ifdef CXX_VERSION
      CXX := clang++
    endif
  endif
endif

ifeq (,$(filter clean,$(MAKECMDGOALS)))
  ifeq (,$(filter install,$(MAKECMDGOALS)))
    $(info $(yellow) INFO: $(reset)with          $(blue)$(CXX)$(reset))
  endif
endif

	NAME = Ratatouille
	space := $(subst ,, )
	EXEC_NAME := $(subst $(space),_,$(NAME))
	BUNDLE = $(EXEC_NAME).lv2
	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	UI_LIB:= $(LIB_DIR)libxputty.$(STATIC_LIB_EXT)
	STRIP ?= strip
	RTN_DIR := ../RTNeural/RTNeural/
	RTN_DEPEND_DIR := ../RTNeural/modules/
	RTN_SOURCES := $(wildcard $(RTN_DIR)*.cpp)
	NAM_DIR := ../NeuralAmpModelerCore/NAM/
	NAM_DEPEND_DIR := ../NeuralAmpModelerCore/Dependencies/
	NAM_SOURCES := $(wildcard $(NAM_DIR)*.cpp)
	IMPL_SOURCE := RtNeuralMulti.cc NeuralAmpMulti.cc
	GUIIMPL_SOURCE := lv2_plugin.cc

ifeq ($(TARGET), Linux)
	GUI_LDFLAGS += -I$(HEADER_DIR) -Wl,-Bstatic -L. $(UI_LIB) \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11` 

	LDFLAGS += -fvisibility=hidden -shared -lm -fPIC -pthread -lpthread \
	-Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections -lfftw3 -lfftw3f \
	-lstdc++fs `$(PKGCONFIG) --cflags --libs sndfile`

	CXXFLAGS += -MMD -std=c++20 -flto=auto -fPIC -DPIC -Ofast -Wall -funroll-loops $(SSE_CFLAGS) \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM \
	-fomit-frame-pointer -fstack-protector -fvisibility=hidden \
	-I$(RTN_DIR) -I./ -I$(RTN_DEPEND_DIR)Eigen/ -I./ -I$(RTN_DEPEND_DIR)json/  \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL -I./zita-resampler-1.1.0 \
	-I. -I./zita-convolver -I./zita-resampler-1.1.0 \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-I$(NAM_DIR) -I./ -I$(DEPEND_DIR)eigen/ -I./ -I$(DEPEND_DIR)nlohmann/  \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS -MD

	ifneq ($(UNAME_S), FreeBSD)
	ifeq (,$(findstring clang, $(CXX)))
	CXXFLAGS += -fstrength-reduce -fno-fat-lto-objects
	else
	CXXFLAGS += -Wno-unused-private-field
	endif
	else
	CXXFLAGS += -Wno-unused-private-field
	endif

	SLP_LDFLAGS += -fvisibility=hidden -Wl,-Bstatic `pkg-config --cflags --libs xputty` \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11 lilv-0` \
	-shared -lm -fPIC -Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATEMODGUI =  sed -i -e '164,179d' -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
else ifeq ($(TARGET), Windows)
	CXXFLAGS += -std=c++17 -D_FORTIFY_SOURCE=2 -I. -I./dsp -I./plugin -fPIC -DPIC -O2 -Wall -funroll-loops \
	-fstack-protector -ffast-math -fomit-frame-pointer -fstrength-reduce \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL $(SSE_CFLAGS) \
	-I$(RTN_DIR) -I./ -I$(RTN_DEPEND_DIR)Eigen/ -I./ -I$(RTN_DEPEND_DIR)json/  \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL -I./zita-resampler-1.1.0 \
	-I. -I./zita-convolver -I./zita-resampler-1.1.0 \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-I$(NAM_DIR) -I./ -I$(DEPEND_DIR)eigen/ -I./ -I$(DEPEND_DIR)nlohmann/  \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS -MD

	LDFLAGS += -I. -shared -lm $(PAWPAW_LFLAGS) -lfftw3 -lfftw3f \
	`$(PKGCONFIG) --cflags --libs sndfile`

	GUI_LDFLAGS += -I$(HEADER_DIR) -static-libgcc -static-libstdc++ \
	`$(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags --libs cairo ` \
	-L. $(LIB_DIR)libxputty.$(STATIC_LIB_EXT) -shared  -lm $(PAWPAW_LFLAGS)

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATE = sed -i '/lv2:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/manifest.ttl
	TTLUPDATEGUI = sed -i '/a guiext:X11UI/ s/X11UI/WindowsUI/ ; /guiext:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/$(NAME).ttl
	TTLUPDATEMODGUI =  sed -i -e '164,179d' -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
endif

	FAUSTFLAGS :=  \
	 \
	

.PHONY : all mod install uninstall clean

.NOTPARALLEL:

all: $(EXEC_NAME).$(LIB_EXT) $(EXEC_NAME)_ui.$(LIB_EXT)

	@$(B_ECHO) "Create $(BUNDLE) $(reset)"
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp ./*.ttl ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEGUI)
	$(QUIET)cp ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(EXEC_NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "Sorry, build fail$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

mod: $(EXEC_NAME).$(LIB_EXT)

	@$(B_ECHO) "Create $(BUNDLE) $(reset)"
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp ./*.ttl ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEMODGUI)
	$(QUIET)cp ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(EXEC_NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "Sorry, build fail$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(EXEC_NAME)_ui.$(LIB_EXT): $(EXEC_NAME).c $(GUIIMPL_SOURCE) $(UI_LIB)
	@$(B_ECHO) "Compiling $(EXEC_NAME)_ui.$(LIB_EXT) $(reset)"
	$(QUIET)$(CC) $(CFLAGS) '$(NAME).c' -L. $(UI_LIB) -o '$(EXEC_NAME)_ui.$(LIB_EXT)' $(LDFLAGS) $(GUI_LDFLAGS) -I./
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME)_ui.$(LIB_EXT)


$(EXEC_NAME).$(LIB_EXT): $(EXEC_NAME).cpp $(RTN_SOURCES) $(NAM_SOURCES) $(IMPL_SOURCE)
	@$(B_ECHO) "Compiling $(EXEC_NAME).$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) $(FAUSTFLAGS) $(EXEC_NAME).cpp $(RTN_SOURCES) $(NAM_SOURCES) $(LDFLAGS) -o $(EXEC_NAME).$(LIB_EXT)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME).$(LIB_EXT)

install :
ifeq ($(TARGET), Linux)
ifneq ("$(wildcard ../bin/$(BUNDLE))","")
	@$(B_ECHO) "Install $(NAME).lv2 $(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)cp -r ../bin/$(BUNDLE)/* $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	@$(B_ECHO) ". ., done$(reset)"
else
	@$(R_ECHO) ". ., you must build first$(reset)"
endif

else
	$(QUIET)$(R_ECHO) "Install is not implemented for windows, please copy the folder $(NAME).lv2 to Program Files/Common Files/LV2$(reset)"
endif

uninstall :
ifeq ($(TARGET), Linux)
	@$(B_ECHO) "Uninstall $(NAME).lv2 $(reset)"
	$(QUIET)rm -rf $(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)echo ". ., done"
else
	$(QUIET)$(R_ECHO) "Uninstall is not implemented for windows, please delete the folder $(NAME).lv2 from Program Files/Common Files/LV2$(reset)"
endif

clean:
ifndef EXTRAQUIET
	@$(ECHO) ". ., clean up$(reset)"
endif
	$(QUIET)rm -f *.a  *.lib *.o *.d *.so *.dll xresources.h
	$(QUIET)rm -rf ../bin

