
	# check on which OS we build
	UNAME_S := $(shell uname -s)
	# check which architecture we build on
	UNAME_M := $(shell uname -m)
	# check which architecture we build for
	TARGET_ARCH = UNKNOWN
	# check if user is root
	user = $(shell whoami)
	ifeq ($(user),root)
	INSTALL_DIR = /usr/lib/lv2
	EXE_INSTALL_DIR = /usr/bin
	CLAP_INSTAL_DIR = /usr/lib/clap
	VST2_INSTAL_DIR = /usr/lib/vst
	else 
	INSTALL_DIR = ~/.lv2
	EXE_INSTALL_DIR = ~/bin
	CLAP_INSTAL_DIR = ~/.clap
	VST2_INSTAL_DIR = ~/.vst
	endif


	# check which architecture we build for
	TARGET_ARCH = $(shell $(CXX) -dumpmachine | sed 's/-.*//')

ifneq ($(UNAME_M),$(TARGET_ARCH))
  CROSS_COMPILING = 1
  $(info $(yellow) INFO: $(reset)Cross Compile $(blue)$(UNAME_M)$(reset) to $(blue)$(TARGET_ARCH)$(reset))
endif

# avoid optimisation for x86_64 arch when we cross compile
ifneq ($(CROSS_COMPILING), 1)

# check if clang is available
ifeq ($(TARGET), Linux)
  CXX_VERSION = $(shell clang++ --version 2>/dev/null)
  ifneq ($(CXX_VERSION),)
    CXX := clang++    
  endif
endif

# get flags supported by CPU
ifeq ($(UNAME_S), FreeBSD)
  CPU_INFO = dmesg | grep Features | tr A-Z a-z
  ifeq ($(UNAME_M), amd64)
    UNAME_M = x86_64
  endif
else
  CPU_INFO = cat /proc/cpuinfo | grep flags
endif

# check for sse optimisation level only on x86_64 architecture
ifeq ($(TARGET_ARCH), x86_64)
  ifneq ($$(filter $(CPU_INFO) | grep sse3 ) , )
    SSE_CFLAGS = -msse3 -mfpmath=sse -mfxsr -DUSE_SSE=1
    FFT_FLAG = -DFFTCONVOLVER_USE_SSE=1
  else ifneq ($$(filter $(CPU_INFO) | grep sse2 ) , )
    SSE_CFLAGS = -msse2 -mfpmath=sse -mfxsr -DUSE_SSE=1
    FFT_FLAG = -DFFTCONVOLVER_USE_SSE=1
  else ifneq ($$(filter $(CPU_INFO) | grep sse ) , )
    SSE_CFLAGS = -msse -mfpmath=sse -mfxsr -DUSE_SSE=1
    FFT_FLAG = -DFFTCONVOLVER_USE_SSE=1
  else ifneq ($$(filter $(CPU_INFO) | grep ARM ) , )
    ifneq ($$(filter $(CPU_INFO) | grep ARMv7 ) , )
      ifneq ($$(filter $(CPU_INFO) | grep vfpd32 ) , )
        SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3 
      else ifneq ($$(filter $(CPU_INFO) | grep vfpv3 ) , )
        SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3
      endif
    else
      ARMCPU = "YES"
    endif
  endif
else
  SSE_CFLAGS =
endif

# check for x86_64 optimisation level
FLAGS_v3    := avx avx2 bmi1 bmi2 f16c fma abm movbe xsave
FLAGS_v2    := cx16 lahf_lm popcnt sse4_1 sse4_2 ssse3
#FLAGS_v1    := lm cmov cx8 fpu fxsr mmx syscall sse2
FLAGS      := $(shell $(CPU_INFO))


NO_x86_64 =
define LOOPBODY
  ifneq ($$(filter $(v),$(FLAGS)),)
    YES_x86_64 = $(x86_64_LEVEL)
  else
    NO_x86_64 = $(x86_64_LEVEL)
  endif
endef

x86_64_LEVEL := -v3
$(foreach v,$(FLAGS_v3),$(eval $(LOOPBODY)))
ifeq ($(NO_x86_64), $(x86_64_LEVEL))
  x86_64_LEVEL := -v2
  $(foreach v,$(FLAGS_v2),$(eval $(LOOPBODY)))
endif
ifeq ($(NO_x86_64), $(x86_64_LEVEL))
  x86_64_LEVEL :=
endif

endif

CXX_v = $(shell $(CXX) -dumpversion)
CXX_VERSION   := $(subst ., ,$(lastword $(CXX_v)))
CXX_MAJOR_VER := $(word 1,$(CXX_VERSION))
ifneq ($(MAKECMDGOALS),install)
  ifeq (,$(filter uninstall,$(MAKECMDGOALS)))
    ifneq ($(MAKECMDGOALS),clean)
      ifneq ($(NO_x86_64), $(x86_64_LEVEL))
        ifeq ($(TARGET_ARCH), aarch64)
          CXXFLAGS += -march=armv8-a
          $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=armv8-a $(SSE_CFLAGS)$(reset))
        else ifeq ($(shell awk -v a="$(CXX_MAJOR_VER)" -v b="11" 'BEGIN{print(a<b)}'), 1)
          SSE_CFLAGS += -march=native
          $(info $(yellow) INFO: $(reset)optimised for $(blue)$(SSE_CFLAGS)$(reset))
        else ifeq ($(TARGET_ARCH), x86_64)
          SSE_CFLAGS += -march=x86-64$(x86_64_LEVEL)
          $(info $(yellow) INFO: $(reset)optimised for $(blue)$(SSE_CFLAGS)$(reset))
        else
          SSE_CFLAGS += -march=native
          $(info $(yellow) INFO: $(reset)optimised for $(blue)$(SSE_CFLAGS)$(reset))
        endif
      endif
    endif
  endif
endif

# check for c++ level
ifeq (,$(filter clean,$(MAKECMDGOALS)))
  ifeq (,$(filter install,$(MAKECMDGOALS)))
    ifeq (,$(filter uninstall,$(MAKECMDGOALS)))
      $(info $(yellow) INFO: $(reset)build with    $(blue)$(CXX) $(CXX_v)$(reset))
      ifeq ($(shell awk -v a="$(CXX_MAJOR_VER)" -v b="11" 'BEGIN{print(a<b)}'), 1)
        CXXFLAGS += -std=c++17
        $(info $(yellow) INFO: $(reset)using         $(blue)-std=c++17$(reset))
      else
        CXXFLAGS += -std=c++20
        $(info $(yellow) INFO: $(reset)using         $(blue)-std=c++20$(reset))
    endif
    endif
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

	PREFIX ?= /usr
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications/
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps/

	RTN_DIR := ../RTNeural/RTNeural/
	RTN_DEPEND_DIR := ../RTNeural/modules/
	RTN_SOURCES := $(wildcard $(RTN_DIR)*.cpp)
	RTN_OBJ := $(patsubst %.cpp,%.o,$(RTN_SOURCES))
	RTN_INCLUDES := -I$(RTN_DIR) -I$(RTN_DEPEND_DIR)Eigen/ -I./ -I$(RTN_DEPEND_DIR)json/ 
	RTN_LIB := librtneural.$(STATIC_LIB_EXT)

	ENGINE_DIR := ./engine/
	ENGINE_INCLUDE := -I./engine/

	CLAP_DIR := ./clap/
	CLAP_INCLUDE := -I./engine/ -I./clap/clap/ -I./clap/
	CLAP_SOURCES := $(CLAP_DIR)RatatouilleClap.cpp

	VST2_DIR := ./vst2/
	VST2_INCLUDE := -I./engine/ -I./clap/ -I./vst2/
	VST2_SOURCES := $(VST2_DIR)Ratatouillevst.cpp

	LV2_DIR := ./lv2/
	LV2_INCLUDE := -I./lv2/

	GUI_DIR := ./gui/
	GUI_INCLUDE := -I./gui/

	STANDALONE_DIR := ./standalone/
	STANDALONE_INCLUDE := -I./standalone/
	STANDALONE := $(STANDALONE_DIR)main

	LV2_PLUGIN := $(LV2_DIR)$(EXEC_NAME)
	LV2_GUI := $(GUI_DIR)$(EXEC_NAME)

	NAM_DIR := ../NeuralAmpModelerCore/NAM/
	NAM_DEPEND_DIR := ../NeuralAmpModelerCore/Dependencies/
	NAM_SOURCES := $(wildcard $(NAM_DIR)*.cpp)
	NAM_OBJ := $(patsubst %.cpp,%.o,$(NAM_SOURCES))
	NAM_INCLUDES := -I$(NAM_DIR) -I$(NAM_DEPEND_DIR)eigen/ -I./ -I$(NAM_DEPEND_DIR)nlohmann/
	NAM_LIB := libnam.$(STATIC_LIB_EXT)

	MODELER_SOURCES := $(ENGINE_DIR)RtNeuralModel.cpp $(ENGINE_DIR)NeuralModel.cpp
	MODELER_OBJ := $(patsubst %.cpp,%.o,$(MODELER_SOURCES))
	MODELER_LIB := libmodeler.$(STATIC_LIB_EXT)

	NEURAL_OBJ := $(NAM_OBJ) $(RTN_OBJ) $(MODELER_OBJ)
	NEURAL_LIB := libneural.$(STATIC_LIB_EXT)

	CONV_DIR := ../FFTConvolver/
	CONV_SOURCES :=  $(wildcard $(CONV_DIR)*.cpp)
	CONV_SOURCES += ./engine/fftconvolver.cpp
	CONV_OBJ := $(patsubst %.cpp,%.o,$(CONV_SOURCES))
	CONV_LIB := libfftconvolver.$(STATIC_LIB_EXT)

	RESAMP_DIR := ./zita-resampler-1.1.0/
	RESAMP_SOURCES := $(wildcard $(RESAMP_DIR)*.cc) $(ENGINE_DIR)gx_resampler.cc
	RESAMP_OBJ := $(patsubst %.cc,%.o,$(RESAMP_SOURCES))
	RESAMP_LIB := libzita-resampler.$(STATIC_LIB_EXT)

	GUIIMPL_SOURCE := $(LV2_DIR)lv2_plugin.cc $(GUI_DIR)widgets.cc

	DEPS = $NEURAL_OBJ:%.o=%.d) $(CONV_OBJ:%.o=%.d) $(RESAMP_OBJ:%.o=%.d) Ratatouille.d

ifeq (,$(filter clean,$(MAKECMDGOALS)))
ifeq (,$(filter install,$(MAKECMDGOALS)))
ifeq (,$(filter uninstall,$(MAKECMDGOALS)))
ifeq (,$(filter mod,$(MAKECMDGOALS)))
ifeq (,$(filter modapp,$(MAKECMDGOALS)))
ifeq (,$(filter lv2,$(MAKECMDGOALS)))
ifeq (,$(filter clap,$(MAKECMDGOALS)))
ifeq (,$(filter vst2,$(MAKECMDGOALS)))
	INFOSTRING = with
	HAVEJACK = $(shell $(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags  --libs jack 2>/dev/null)
	ifneq ($(HAVEJACK), )
	JACKFLAGS += -DHAVE_JACK
	INFOSTRING += JACK
	endif
	HAVEPA = $(shell $(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags  --libs portaudio-2.0 2>/dev/null)
	ifneq ($(HAVEPA), )
	ifneq ($(HAVEJACK), )
	INFOSTRING += and
	endif
	JACKFLAGS += -DHAVE_PA
	INFOSTRING += PortAudio
	endif
	ifneq ($(JACKFLAGS), )
	INFOSTRING += support
    $(info $(yellow) INFO: $(reset)Stand-alone   $(blue)$(INFOSTRING)$(reset))
    else
    $(info $(yellow) INFO: $(reset)Stand-alone   $(red)not supported$(reset))
	endif
endif
endif
endif
endif
endif
endif
endif
endif

ifeq ($(PAWPAW_BUILD),1)
	CXXFLAGS += -DPAWPAW=1
endif

ifeq ($(TARGET), Linux)

	GUI_LDFLAGS += -I$(HEADER_DIR) $(GUI_INCLUDE) -Wl,-Bstatic -L. $(UI_LIB) \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11` 

	LDFLAGS += -fvisibility=hidden -shared -lm -fPIC -pthread -lpthread \
	-Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections  -Wl,--exclude-libs,ALL \
	`$(PKGCONFIG) --cflags --libs sndfile`

	JACKLDFLAGS += -fvisibility=hidden -lm -fPIC -pthread -lpthread \
	-Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections  -Wl,--exclude-libs,ALL \
	`$(PKGCONFIG) --cflags --libs sndfile ` $(HAVEPA) $(HAVEJACK) $(GUI_LDFLAGS)

	CXXFLAGS += -MMD -flto=auto -fPIC -DPIC -O3 -Wall -funroll-loops $(SSE_CFLAGS) \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM $(FFT_FLAG) \
	-fomit-frame-pointer -fstack-protector -fvisibility=hidden -Wno-pessimizing-move \
	-fdata-sections -I. -I./ -I./zita-resampler-1.1.0 -I$(CONV_DIR) -DNDEBUG \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS

	ifneq ($(UNAME_S), FreeBSD)
	ifeq (,$(findstring clang, $(CXX)))
	CXXFLAGS += -fstrength-reduce -fno-fat-lto-objects -Wno-deprecated-declarations
	else
	CXXFLAGS += -Wno-unused-private-field -fdenormal-fp-math=positive-zero -Wno-vla-cxx-extension \
	-Wno-unknown-argument -Wno-nan-infinity-disabled
	endif
	else
	CXXFLAGS += -Wno-unused-private-field -fdenormal-fp-math=positive-zero \
	-Wno-vla-cxx-extension -Wno-nan-infinity-disabled
	endif

	# remove unwanted flags from PawPaw
	ifneq (,$(findstring clang, $(CXX)))
	CXXFLAGS := $(filter-out -fno-gnu-unique,$(CXXFLAGS))
	LDFLAGS := $(filter-out -fno-gnu-unique,$(LDFLAGS))
	endif

	SLP_LDFLAGS += -fvisibility=hidden -Wl,-Bstatic `pkg-config --cflags --libs xputty` \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11 lilv-0` \
	-shared -lm -fPIC -Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -Wno-unused-result -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATEMODGUI =  sed -i -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '/<urn:brummer:ratatouille_ui>/,/] ./d' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
else ifeq ($(TARGET), Windows)
	CXXFLAGS += -D_FORTIFY_SOURCE=2 -I. -fPIC -DPIC -O3 -Wall -funroll-loops \
	-ffast-math -fomit-frame-pointer -fstrength-reduce -Wno-deprecated-declarations \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM $(SSE_CFLAGS) $(FFT_FLAG) \
	-fdata-sections -I./ -I./zita-resampler-1.1.0 -I$(CONV_DIR) -DNDEBUG \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS

	LDFLAGS += -I. -shared -lm $(PAWPAW_LFLAGS) -Wl,--gc-sections \
	-Wl,--exclude-libs,ALL `$(PKGCONFIG) --cflags --libs sndfile`

	JACKLDFLAGS += -I. -lm $(PAWPAW_LFLAGS) -Wl,--gc-sections -pthread  $(PKGCONFIG_FLAGS) -lpthread  \
	-Wl,--exclude-libs,ALL `$(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags --libs sndfile ` $(HAVEPA) $(HAVEJACK) $(GUI_LDFLAGS)

	GUI_LDFLAGS += -I$(HEADER_DIR) $(GUI_INCLUDE) -static-libgcc -static-libstdc++ \
	`$(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags --libs cairo ` \
	-L. $(LIB_DIR)libxputty.$(STATIC_LIB_EXT) -lm $(PAWPAW_LFLAGS)

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATE = sed -i '/lv2:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/manifest.ttl
	TTLUPDATEGUI = sed -i '/a guiext:X11UI/ s/X11UI/WindowsUI/ ; /guiext:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/$(NAME).ttl
	TTLUPDATEMODGUI =  sed -i -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '/<urn:brummer:ratatouille_ui>/,/] ./d' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
endif

ifneq ($(MAKECMDGOALS),install)
  ifneq ($(MAKECMDGOALS),clean)
    $(info ==================$(yellow) CXXFLAGS $(reset)=====================)
    $(info $(blue)$(CXXFLAGS)$(reset))
    $(info ===================$(yellow) BUILD $(reset)=======================)
  endif
endif

.PHONY : all mod install uninstall clean

.NOTPARALLEL:

all: $(EXEC_NAME).$(LIB_EXT) $(EXEC_NAME)_ui.$(LIB_EXT) modapp standalone clap vst2

lv2: $(EXEC_NAME).$(LIB_EXT) $(EXEC_NAME)_ui.$(LIB_EXT)
	@$(B_ECHO) "Create $(BUNDLE) $(reset)"
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp $(LV2_DIR)*.ttl ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEGUI)
	$(QUIET)cp ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(EXEC_NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "Sorry, build fail$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

lv2log: CXXFLAGS += -DLV2LOG=1 
lv2log: CFLAGS += -DLV2LOG=1
lv2log: lv2

mod: $(EXEC_NAME).$(LIB_EXT)

	@$(B_ECHO) "Create $(BUNDLE) $(reset)"
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp $(LV2_DIR)*.ttl ../bin/$(BUNDLE)
	@$(B_ECHO) "Copy MOD UI to $(BUNDLE) $(reset)"
	$(QUIET)cp -r ./mod/* ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEMODGUI)
	$(QUIET)cp ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(EXEC_NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "Sorry, build fail$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

modapp: $(EXEC_NAME).$(LIB_EXT) $(EXEC_NAME)_ui.$(LIB_EXT)

	@$(B_ECHO) "Create $(BUNDLE) $(reset)"
	$(QUIET)mkdir -p ../bin/$(BUNDLE)
	$(QUIET)cp $(LV2_DIR)*.ttl ../bin/$(BUNDLE)
	@$(B_ECHO) "Copy MOD UI to $(BUNDLE) $(reset)"
	$(QUIET)cp -r ./mod/* ../bin/$(BUNDLE)
	$(QUIET)$(TTLUPDATE)
	$(QUIET)$(TTLUPDATEGUI)
	$(QUIET)cp ./*.$(LIB_EXT) ../bin/$(BUNDLE)
	@if [ -f ../bin/$(BUNDLE)/$(EXEC_NAME).$(LIB_EXT) ]; then \
		$(B_ECHO) "build finish, . . . $(reset)"; \
	else \
		$(R_ECHO) "Sorry, build fail$(reset)"; \
	fi
	@$(B_ECHO) "=================== DONE =======================$(reset)"

standalone: $(EXEC_NAME)$(EXE_EXT)
ifneq ($(JACKFLAGS), )
	$(QUIET)mkdir -p ../bin/
	$(QUIET)cp ./$(EXEC_NAME)$(EXE_EXT) ../bin/
	@$(B_ECHO) "=================== DONE =======================$(reset)"
endif

vst2: $(NAME)vst.$(LIB_EXT)
	$(QUIET)mkdir -p ../bin/
	$(QUIET)cp ./$(NAME)vst.$(LIB_EXT) ../bin/
	@$(B_ECHO) "=================== DONE =======================$(reset)"

clap: $(NAME).clap
	$(QUIET)mkdir -p ../bin/
	$(QUIET)cp ./$(NAME).clap ../bin/
	@$(B_ECHO) "=================== DONE =======================$(reset)"

-include $(DEPS)

$(RTN_OBJ): $(RTN_SOURCES)
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD -Wall -c $(patsubst %.o,%.cpp,$@) -o $@ $(RTN_INCLUDES)

$(RTN_LIB): $(RTN_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(RTN_LIB) $(RTN_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(NAM_OBJ): $(NAM_SOURCES)
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD -Wall -c $(patsubst %.o,%.cpp,$@) -o $@  $(NAM_INCLUDES)

$(NAM_LIB): $(NAM_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(NAM_LIB) $(NAM_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(CONV_OBJ): $(CONV_SOURCES)
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD -Wall -c $(patsubst %.o,%.cpp,$@) -o $@

$(CONV_LIB): $(CONV_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(CONV_LIB) $(CONV_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(MODELER_OBJ): $(MODELER_SOURCES)
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD  -c $(patsubst %.o,%.cpp,$@) -o $@ $(NAM_INCLUDES) $(RTN_INCLUDES) $(ENGINE_INCLUDE)

$(MODELER_LIB): $(MODELER_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(MODELER_LIB) $(MODELER_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(NEURAL_LIB): $(NEURAL_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(NEURAL_LIB) $(NEURAL_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(RESAMP_OBJ): $(RESAMP_SOURCES)
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD  -c $(patsubst %.o,%.cc,$@) -o $@ -I./zita-resampler-1.1.0 $(ENGINE_INCLUDE)

$(RESAMP_LIB): $(RESAMP_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(RESAMP_LIB) $(RESAMP_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(EXEC_NAME)_ui.$(LIB_EXT): $(LV2_GUI).c $(GUIIMPL_SOURCE) $(UI_LIB)
	@$(B_ECHO) "Compiling $(EXEC_NAME)_ui.$(LIB_EXT) $(reset)"
	$(QUIET)$(CC) $(CFLAGS) $(LV2_GUI).c -L. $(UI_LIB) -o $@ $(LDFLAGS) $(GUI_LDFLAGS) -I./ $(LV2_INCLUDE)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME)_ui.$(LIB_EXT)

$(EXEC_NAME).$(LIB_EXT): $(LV2_PLUGIN).cpp $(NEURAL_LIB) $(CONV_LIB) $(RESAMP_LIB)
	@$(B_ECHO) "Compiling $(EXEC_NAME).$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) $(NAM_INCLUDES) $(RTN_INCLUDES) $(ENGINE_INCLUDE) $(LV2_INCLUDE) $(LV2_PLUGIN).cpp  \
	-L. $(NEURAL_LIB) -L. $(CONV_LIB) -L. $(RESAMP_LIB) $(LDFLAGS) -o $(EXEC_NAME).$(LIB_EXT)
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME).$(LIB_EXT)

$(EXEC_NAME)$(EXE_EXT): $(NEURAL_LIB) $(CONV_LIB) $(RESAMP_LIB) $(UI_LIB) $(STANDALONE).cpp
ifneq ($(JACKFLAGS), )
	@$(B_ECHO) "Compiling $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) $(NAM_INCLUDES) $(RTN_INCLUDES) $(ENGINE_INCLUDE) \
	$(STANDALONE_INCLUDE) $(JACKFLAGS) $(STANDALONE).cpp -L. $(UI_LIB) \
	-L. $(NEURAL_LIB) -L. $(CONV_LIB) -L. $(RESAMP_LIB) $(JACKLDFLAGS) -o $@
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME)$(EXE_EXT)
endif

$(NAME)vst.$(LIB_EXT): $(VST2_SOURCES) $(CLAP_DIR)$(NAME).cc $(NEURAL_LIB) $(CONV_LIB) $(RESAMP_LIB)
	@$(B_ECHO) "Compiling $(NAME)vst.$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -Wno-multichar $(NAM_INCLUDES) $(RTN_INCLUDES) $(ENGINE_INCLUDE) $(VST2_INCLUDE) $(VST2_SOURCES)  \
	-L. $(NEURAL_LIB) -L. $(CONV_LIB) -L. $(RESAMP_LIB) $(GUI_LDFLAGS) $(LDFLAGS) -o $(NAME)vst.$(LIB_EXT)
ifeq (,$(filter yes,$(DEBUG)))
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME)vst.$(LIB_EXT)
endif

$(NAME).clap: $(CLAP_SOURCES) $(CLAP_DIR)$(NAME).cc $(NEURAL_LIB) $(CONV_LIB) $(RESAMP_LIB)
	@$(B_ECHO) "Compiling $(NAME).clap $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) $(NAM_INCLUDES) $(RTN_INCLUDES) $(ENGINE_INCLUDE) $(CLAP_INCLUDE) $(CLAP_SOURCES)  \
	-L. $(NEURAL_LIB) -L. $(CONV_LIB) -L. $(RESAMP_LIB) $(GUI_LDFLAGS) $(LDFLAGS) -o $(NAME).clap
ifeq (,$(filter yes,$(DEBUG)))
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(NAME).clap
endif


install :
ifeq ($(TARGET), Linux)
ifneq ("$(wildcard ../bin/$(BUNDLE))","")
	@$(B_ECHO) "Install $(NAME).lv2 to $(DESTDIR)$(INSTALL_DIR)/$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)cp -r ../bin/$(BUNDLE)/* $(DESTDIR)$(INSTALL_DIR)/$(BUNDLE)
	@$(B_ECHO) ". ., done$(reset)"
else
	@$(B_ECHO) "$(BUNDLE) skipped$(reset)"
endif
ifneq ("$(wildcard ../bin/$(EXEC_NAME)$(EXE_EXT))","")
	@$(B_ECHO) "Install  $(EXEC_NAME)$(EXE_EXT) to $(DESTDIR)$(EXE_INSTALL_DIR)/$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(EXE_INSTALL_DIR)/
	$(QUIET)cp -r ../bin/$(EXEC_NAME)$(EXE_EXT) $(DESTDIR)$(EXE_INSTALL_DIR)/$(EXEC_NAME)$(EXE_EXT)

  ifeq ($(user),root)
	$(QUIET)mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	$(QUIET)cp $(STANDALONE_DIR)$(EXEC_NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)$(EXEC_NAME).desktop
	$(QUIET)mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	$(QUIET)cp $(STANDALONE_DIR)$(EXEC_NAME).svg $(DESTDIR)$(PIXMAPS_DIR)$(EXEC_NAME).svg 
	$(QUIET)update-desktop-database || true
  endif
	@$(B_ECHO) ". ., done$(reset)"
else
	@$(B_ECHO) "$(EXEC_NAME)$(EXE_EXT) standalone skipped$(reset)"
endif
ifneq ("$(wildcard ../bin/$(NAME).clap)","")
	@$(B_ECHO) "Install  $(NAME).clap to $(DESTDIR)$(CLAP_INSTAL_DIR)/$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(CLAP_INSTAL_DIR)/
	$(QUIET)cp -r ../bin/$(NAME).clap $(DESTDIR)$(CLAP_INSTAL_DIR)/$(NAME).clap
else
	@$(B_ECHO) "$(NAME).clap Clap skipped$(reset)"
endif
ifneq ("$(wildcard ../bin/$(NAME)vst.$(LIB_EXT))","")
	@$(B_ECHO) "Install  $(NAME)vst.$(LIB_EXT) to $(DESTDIR)$(VST2_INSTAL_DIR)/$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(VST2_INSTAL_DIR)/
	$(QUIET)cp -r ../bin/$(NAME)vst.$(LIB_EXT) $(DESTDIR)$(VST2_INSTAL_DIR)/$(NAME)vst.$(LIB_EXT)
else
	@$(B_ECHO) "$(NAME)vst.$(LIB_EXT) vst2 skipped$(reset)"
endif

else
	$(QUIET)$(R_ECHO) "Install is not implemented for windows, please copy the folder $(NAME).lv2 to Program Files/Common Files/LV2$(reset)"
endif

uninstall :
ifeq ($(TARGET), Linux)
	@$(B_ECHO) "Uninstall $(NAME).lv2 $(reset)"
	$(QUIET)rm -rf $(INSTALL_DIR)/$(BUNDLE)
	$(QUIET)rm -rf $(DESTDIR)$(CLAP_INSTAL_DIR)/$(NAME)clap.clap
	$(QUIET)rm -rf $(DESTDIR)$(VST2_INSTAL_DIR)/$(NAME)vst.so
	$(QUIET)rm -rf $(DESTDIR)$(EXE_INSTALL_DIR)/$(EXEC_NAME)
	$(QUIET)echo ". ., done"
  ifeq ($(user),root)
	$(QUIET)rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(EXEC_NAME).desktop
	$(QUIET)rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(EXEC_NAME).svg
  endif
else
	$(QUIET)$(R_ECHO) "Uninstall is not implemented for windows, please delete the folder $(NAME).lv2 from Program Files/Common Files/LV2$(reset)"
endif

clean:
ifndef EXTRAQUIET
	@$(ECHO) ". ., clean up$(reset)"
endif
	$(QUIET)rm -f *.a  *.lib *.o *.d *.so *.dll $(EXEC_NAME) $(EXEC_NAME).exe *.clap
	$(QUIET)rm -f $(RESAMP_DIR)*.a $(RESAMP_DIR)*.lib $(RESAMP_DIR)*.o $(RESAMP_DIR)*.d
	$(QUIET)rm -f $(CONV_DIR)*.a $(CONV_DIR)*.lib $(CONV_DIR)*.o $(CONV_DIR)*.d
	$(QUIET)rm -f $(NAM_DIR)*.a $(NAM_DIR)*.lib $(NAM_DIR)*.o $(NAM_DIR)*.d
	$(QUIET)rm -f $(RTN_DIR)*.a $(RTN_DIR)*.lib $(RTN_DIR)*.o $(RTN_DIR)*.d
	$(QUIET)rm -f $(ENGINE_DIR)*.a $(ENGINE_DIR)*.lib $(ENGINE_DIR)*.o $(ENGINE_DIR)*.d
	$(QUIET)rm -rf ../bin

