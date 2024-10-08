
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
	else 
	INSTALL_DIR = ~/.lv2
	endif


# check if clang is available
ifeq ($(TARGET), Linux)
  CXX_VERSION = $(shell clang++ --version 2>/dev/null)
  ifneq ($(CXX_VERSION),)
    CXX := clang++    
  endif
endif

	# check which architecture we build for
	TARGET_ARCH = $(shell $(CXX) -dumpmachine | sed 's/-.*//')

# get flags supported by CPU
ifeq ($(UNAME_S), FreeBSD)
  CPU_INFO = dmesg | grep Features | tr A-Z a-z
  ifeq ($(UNAME_M), amd64)
    UNAME_M = x86_64
  endif
else
  CPU_INFO = cat /proc/cpuinfo | grep flags
endif

ifneq ($(UNAME_M),$(TARGET_ARCH))
  CROSS_COMPILING = 1
  $(info $(yellow) INFO: $(reset)Cross Compile $(blue)$(UNAME_M)$(reset) to $(blue)$(TARGET_ARCH)$(reset))
endif

# avoid optimisation for x86_64 arch when we cross compile
#ifneq ($(CROSS_COMPILING), 1)
# check for sse optimisation level only on x86_64 architecture
ifeq ($(TARGET_ARCH), x86_64)
  ifneq ($$(filter $(CPU_INFO) | grep sse3 ) , )
    SSE_CFLAGS = -msse3 -mfpmath=sse -mfxsr -DUSE_SSE
    FFT_FLAG = -DFFTCONVOLVER_USE_SSE=1
  else ifneq ($$(filter $(CPU_INFO) | grep sse2 ) , )
    SSE_CFLAGS = -msse2 -mfpmath=sse -mfxsr -DUSE_SSE
    FFT_FLAG = -DFFTCONVOLVER_USE_SSE=1
  else ifneq ($$(filter $(CPU_INFO) | grep sse ) , )
    SSE_CFLAGS = -msse -mfpmath=sse -mfxsr -DUSE_SSE
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
#endif

CXX_v = $(shell $(CXX) -dumpversion)
ifneq ($(MAKECMDGOALS),install)
  ifneq ($(MAKECMDGOALS),clean)
    ifneq ($(NO_x86_64), $(x86_64_LEVEL))
      ifeq ($(TARGET_ARCH), aarch64)
        CXXFLAGS += -march=armv8-a
        $(info $(yellow) INFO: $(reset)optimised for $(blue)-march=armv8-a $(SSE_CFLAGS)$(reset))
      else ifeq ($(shell awk -v a="$(CXX_v)" -v b="11" 'BEGIN{print(a<b)}'), 1)
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

# check for c++ level
ifeq (,$(filter clean,$(MAKECMDGOALS)))
  ifeq (,$(filter install,$(MAKECMDGOALS)))
    $(info $(yellow) INFO: $(reset)build with    $(blue)$(CXX)$(reset))
    ifeq ($(shell awk -v a="$(CXX_v)" -v b="11" 'BEGIN{print(a<b)}'), 1)
      CXXFLAGS += -std=c++17
      $(info $(yellow) INFO: $(reset)using         $(blue)-std=c++17$(reset))
    else
      CXXFLAGS += -std=c++20
      $(info $(yellow) INFO: $(reset)using         $(blue)-std=c++20$(reset))
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

	RTN_DIR := ../RTNeural/RTNeural/
	RTN_DEPEND_DIR := ../RTNeural/modules/
	RTN_SOURCES := $(wildcard $(RTN_DIR)*.cpp)
	RTN_OBJ := $(patsubst %.cpp,%.o,$(RTN_SOURCES))
	RTN_INCLUDES := -I$(RTN_DIR) -I$(RTN_DEPEND_DIR)Eigen/ -I./ -I$(RTN_DEPEND_DIR)json/ 
	RTN_LIB := librtneural.$(STATIC_LIB_EXT)

	NAM_DIR := ../NeuralAmpModelerCore/NAM/
	NAM_DEPEND_DIR := ../NeuralAmpModelerCore/Dependencies/
	NAM_SOURCES := $(wildcard $(NAM_DIR)*.cpp)
	NAM_OBJ := $(patsubst %.cpp,%.o,$(NAM_SOURCES))
	NAM_INCLUDES := -I$(NAM_DIR) -I$(NAM_DEPEND_DIR)eigen/ -I./ -I$(NAM_DEPEND_DIR)nlohmann/
	NAM_LIB := libnam.$(STATIC_LIB_EXT)

	MODELER_SOURCES := RtNeuralModel.cpp NeuralModel.cpp
	MODELER_OBJ := $(patsubst %.cpp,%.o,$(MODELER_SOURCES))
	MODELER_LIB := libmodeler.$(STATIC_LIB_EXT)

	NEURAL_OBJ := $(NAM_OBJ) $(RTN_OBJ) $(MODELER_OBJ)
	NEURAL_LIB := libneural.$(STATIC_LIB_EXT)

	CONV_DIR := ../FFTConvolver/
	CONV_SOURCES :=  $(wildcard $(CONV_DIR)*.cpp)
	CONV_OBJ := $(patsubst %.cpp,%.o,$(CONV_SOURCES))
	CONV_LIB := libfftconvolver.$(STATIC_LIB_EXT)

	RESAMP_DIR := ./zita-resampler-1.1.0/
	RESAMP_SOURCES := $(wildcard $(RESAMP_DIR)*.cc) gx_resampler.cc
	RESAMP_OBJ := $(patsubst %.cc,%.o,$(RESAMP_SOURCES))
	RESAMP_LIB := libzita-resampler.$(STATIC_LIB_EXT)

	GUIIMPL_SOURCE := lv2_plugin.cc

	DEPS = $NEURAL_OBJ:%.o=%.d) $(CONV_OBJ:%.o=%.d) $(RESAMP_OBJ:%.o=%.d) Ratatouille.d

ifeq ($(TARGET), Linux)
	GUI_LDFLAGS += -I$(HEADER_DIR) -Wl,-Bstatic -L. $(UI_LIB) \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11` 

	LDFLAGS += -fvisibility=hidden -shared -lm -fPIC -pthread -lpthread \
	-Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections  -Wl,--exclude-libs,ALL \
	`$(PKGCONFIG) --cflags --libs sndfile`

	CXXFLAGS += -MMD -flto=auto -fPIC -DPIC -Ofast -Wall -funroll-loops $(SSE_CFLAGS) \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM $(FFT_FLAG) \
	-fomit-frame-pointer -fstack-protector -fvisibility=hidden \
	-fdata-sections -I. -I./ -I./zita-resampler-1.1.0 -I$(CONV_DIR) \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS

	ifneq ($(UNAME_S), FreeBSD)
	ifeq (,$(findstring clang, $(CXX)))
	CXXFLAGS += -fstrength-reduce -fno-fat-lto-objects -Wno-deprecated-declarations
	else
	CXXFLAGS += -Wno-unused-private-field -fdenormal-fp-math=positive-zero
	endif
	else
	CXXFLAGS += -Wno-unused-private-field -fdenormal-fp-math=positive-zero \
	-Wno-vla-cxx-extension -Wno-nan-infinity-disabled
	endif

	SLP_LDFLAGS += -fvisibility=hidden -Wl,-Bstatic `pkg-config --cflags --libs xputty` \
	-Wl,-Bdynamic `pkg-config --cflags --libs cairo x11 lilv-0` \
	-shared -lm -fPIC -Wl,-z,noexecstack -Wl,--no-undefined -Wl,--gc-sections

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -Wno-unused-result -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,-z,relro,-z,now -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATEMODGUI =  sed -i -e '191,206d' -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
else ifeq ($(TARGET), Windows)
	CXXFLAGS += -D_FORTIFY_SOURCE=2 -I. -fPIC -DPIC -O2 -Wall -funroll-loops \
	-ffast-math -fomit-frame-pointer -fstrength-reduce -Wno-deprecated-declarations \
	-Wno-sign-compare -Wno-reorder -Wno-infinite-recursion -DUSE_ATOM $(SSE_CFLAGS) $(FFT_FLAG) \
	-fdata-sections -I./ -I./zita-resampler-1.1.0 -I$(CONV_DIR) \
	-DRTNEURAL_DEFAULT_ALIGNMENT=32 -DRTNEURAL_USE_EIGEN=1 -DRTNEURAL_NAMESPACE=RTNeural \
	-DDSP_SAMPLE_FLOAT -DNAM_SAMPLE_FLOAT -Dneural_amp_modeler_EXPORTS

	LDFLAGS += -I. -shared -lm $(PAWPAW_LFLAGS) -Wl,--gc-sections \
	-Wl,--exclude-libs,ALL `$(PKGCONFIG) --cflags --libs sndfile`

	GUI_LDFLAGS += -I$(HEADER_DIR) -static-libgcc -static-libstdc++ \
	`$(PKGCONFIG) $(PKGCONFIG_FLAGS) --cflags --libs cairo ` \
	-L. $(LIB_DIR)libxputty.$(STATIC_LIB_EXT) -shared  -lm $(PAWPAW_LFLAGS)

	CFLAGS := -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -fvisibility=hidden \
	-fdata-sections -Wl,--gc-sections -Wl,--exclude-libs,ALL -DUSE_ATOM

	TTLUPDATE = sed -i '/lv2:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/manifest.ttl
	TTLUPDATEGUI = sed -i '/a guiext:X11UI/ s/X11UI/WindowsUI/ ; /guiext:binary/ s/\.so/\.dll/ ' ../bin/$(BUNDLE)/$(NAME).ttl
	TTLUPDATEMODGUI =  sed -i -e '183,197d' -e 's/guiext:ui <urn:brummer:ratatouille_ui> ;//' \
	-e '7d' ../bin/$(BUNDLE)/$(NAME).ttl
endif

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
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD  -c $(patsubst %.o,%.cpp,$@) -o $@ $(NAM_INCLUDES) $(RTN_INCLUDES)

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
	$(QUIET)$(CXX) $(CXXFLAGS) -MMD  -c $(patsubst %.o,%.cc,$@) -o $@ -I./zita-resampler-1.1.0

$(RESAMP_LIB): $(RESAMP_OBJ)
	@$(B_ECHO) "Build static library $@ $(reset)"
	$(QUIET)$(AR) rcs $(RESAMP_LIB) $(RESAMP_OBJ)
	@$(B_ECHO) "=================== DONE =======================$(reset)"

$(EXEC_NAME)_ui.$(LIB_EXT): $(EXEC_NAME).c $(GUIIMPL_SOURCE) $(UI_LIB)
	@$(B_ECHO) "Compiling $(EXEC_NAME)_ui.$(LIB_EXT) $(reset)"
	$(QUIET)$(CC) $(CFLAGS) $(EXEC_NAME).c -L. $(UI_LIB) -o $@ $(LDFLAGS) $(GUI_LDFLAGS) -I./
	$(QUIET)$(STRIP) -s -x -X -R .comment -R .note.ABI-tag $(EXEC_NAME)_ui.$(LIB_EXT)

$(EXEC_NAME).$(LIB_EXT): $(EXEC_NAME).cpp $(NEURAL_LIB) $(CONV_LIB) $(RESAMP_LIB)
	@$(B_ECHO) "Compiling $(EXEC_NAME).$(LIB_EXT) $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) $(NAM_INCLUDES) $(RTN_INCLUDES) $(EXEC_NAME).cpp  \
	-L. $(NEURAL_LIB) -L. $(CONV_LIB) -L. $(RESAMP_LIB) $(LDFLAGS) -o $@
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
	$(QUIET)rm -f *.a  *.lib *.o *.d *.so *.dll 
	$(QUIET)rm -f $(RESAMP_DIR)*.a $(RESAMP_DIR)*.lib $(RESAMP_DIR)*.o $(RESAMP_DIR)*.d
	$(QUIET)rm -f $(CONV_DIR)*.a $(CONV_DIR)*.lib $(CONV_DIR)*.o $(CONV_DIR)*.d
	$(QUIET)rm -f $(NAM_DIR)*.a $(NAM_DIR)*.lib $(NAM_DIR)*.o $(NAM_DIR)*.d
	$(QUIET)rm -f $(RTN_DIR)*.a $(RTN_DIR)*.lib $(RTN_DIR)*.o $(RTN_DIR)*.d
	$(QUIET)rm -rf ../bin

