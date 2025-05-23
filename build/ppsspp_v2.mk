#cd ffmpeg
#PATH=/home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin:$PATH ./linux_arm64.sh

BR2_PACKAGE_RECALBOX_TARGETGROUP_ROCKCHIP=y
TARGET_CC=/home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-gcc
TARGET_CXX=/home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-g++
TARGET_LD=/home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-ld
COMPILER_COMMONS_CFLAGS_EXE=-I/home/wmt/work_trimui/usr/include
COMPILER_COMMONS_CXXFLAGS_EXE=-I/home/wmt/work_trimui/usr/include
COMPILER_COMMONS_LDFLAGS_EXE=-lz -lbz2 -lfreetype -lEGL -lIMGegl -lsrv_um -lusc -lglslcompiler -L/home/wmt/work_trimui/usr/lib
#-DUSE_SYSTEM_FFMPEG=ON->OFF

################################################################################
#
# PPSSPP
#
################################################################################
ifeq ($(BR2_PACKAGE_RECALBOX_TARGETGROUP_ROCKCHIP),y)
PPSSPP_VERSION = e66f019ffaaf7ff4224aa3605ebf59f5654bd0d4
else
PPSSPP_VERSION = 48934df6787cd9d693779ec1b0915a5c1ce02c72
endif
PPSSPP_SITE = git://github.com/hrydgard/ppsspp.git
PPSSPP_GIT_SUBMODULES=y
PPSSPP_DEPENDENCIES = sdl2 zlib libzip linux zip ffmpeg snappy

# required at least on x86
ifeq ($(BR2_PACKAGE_LIBGLU),y)
PPSSPP_DEPENDENCIES += libglu
endif

define PPSSPP_CONFIGURE_PI
	sed -i "s+/opt/vc+$(STAGING_DIR)/usr+g" $(@D)/CMakeLists.txt
endef

PPSSPP_PRE_CONFIGURE_HOOKS += PPSSPP_CONFIGURE_PI

define PPSSPP_INSTALL_TO_TARGET
	$(INSTALL) -D -m 0755 $(@D)/PPSSPPSDL $(TARGET_DIR)/usr/bin
	mkdir -p $(TARGET_DIR)/usr/share/ppsspp
	cp -R $(@D)/assets $(TARGET_DIR)/usr/share/ppsspp
endef

PPSSPP_INSTALL_TARGET_CMDS = $(PPSSPP_INSTALL_TO_TARGET)
PPSSPP_CONF_OPTS += -DUSE_FFMPEG=1

ifeq ($(BR2_PACKAGE_MALI_OPENGLES_SDK),y)
PPSSPP_CONF_OPTS += -DUSING_FBDEV=1
PPSSPP_CONF_OPTS += -DUSING_GLES2=1
endif

ifeq ($(BR2_PACKAGE_RPI_USERLAND),y)
PPSSPP_DEPENDENCIES += rpi-userland
PPSSPP_CONF_OPTS += -DUSING_FBDEV=1
PPSSPP_CONF_OPTS += -DUSING_GLES2=1
endif

ifeq ($(BR2_aarch64)$(BR2_ARM_CPU_HAS_NEON),y)
PPSSPP_CONF_OPTS += -DARM_NEON=1
endif

PPSSPP_CFLAGS = $(COMPILER_COMMONS_CFLAGS_EXE)
PPSSPP_CXXFLAGS = $(COMPILER_COMMONS_CXXFLAGS_EXE)

ifeq ($(BR2_PACKAGE_RECALBOX_TARGETGROUP_ROCKCHIP),y)
# Rockchip
PPSSPP_CONF_OPTS += \
	-DANDROID=OFF -DWIN32=OFF -DAPPLE=OFF -DX86=OFF -DX86_64=OFF\
	-DUSE_SYSTEM_FFMPEG=OFF -DCMAKE_CROSSCOMPILING=ON \
	-DUSING_FBDEV=ON -DUSING_GLES2=ON -DUSING_EGL=OFF -DOPENGL_LIBRARIES=GLESv2 \
	-DARM=ON -DARM64=ON -DFORCED_CPU=ARM64 \
	-DUSING_X11_VULKAN=OFF -DVULKAN=OFF -DARM_NO_VULKAN=ON -DUSE_WAYLAND_WSI=OFF \
	-DUSING_QT_UI=OFF -DUNITTEST=OFF -DSIMULATOR=OFF -DHEADLESS=OFF \
	-DSDL2_LIBRARY=/home/wmt/work_trimui/usr/lib/libSDL2.a \
	-DSDL2_INCLUDE_DIR=/home/wmt/work_trimui/usr/include/SDL2 \
	-DCMAKE_VERBOSE_MAKEFILE=ON \
	-DSDL2_ttf_PKGCONFIG_STATIC_INCLUDE_DIRS=/home/wmt/work_trimui/usr/include/SDL2 \
	-DSDL2PC_INCLUDE_DIRS=/home/wmt/work_trimui/usr/include/SDL2 \
	-DSDL2PC_STATIC_INCLUDE_DIRS=/home/wmt/work_trimui/usr/include/SDL2 \
	-DSDL2_ttf_PKGCONFIG_INCLUDE_DIRS=/home/wmt/work_trimui/usr/include/SDL2 \

####Actually use SDL2_ttf_PKGCONFIG_INCLUDE_DIRS

# In order to support the custom resolution patch, permissive compile is needed
PPSSPP_CXXFLAGS += -fpermissive

# Rockchip uses png16, not 17
define PPSSPP_CONFIGURE_ROCK
sed -i 's/png17/png16/g' $(@D)/CMakeLists.txt
endef
PPSSPP_PRE_CONFIGURE_HOOKS += PPSSPP_CONFIGURE_ROCK

# New version of emulator requires some libaries that the cmake system doesn't auto-install
define PPSSPP_INSTALL_ROCK
cp $(@D)/lib/*.so $(TARGET_DIR)/usr/lib
endef
PPSSPP_POST_BUILD_HOOKS += PPSSPP_INSTALL_ROCK
else
# odroid xu4 / rpi3
ifeq ($(BR2_arm),y)
PPSSPP_CONF_OPTS += -DARMV7=1
endif
endif


PPSSPP_CONF_OPTS += -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
PPSSPP_CONF_OPTS += -DCMAKE_C_ARCHIVE_FINISH=true
PPSSPP_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
PPSSPP_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_FINISH=true
PPSSPP_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
PPSSPP_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
PPSSPP_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
PPSSPP_CONF_OPTS += -DCMAKE_LINKER="$(TARGET_LD)"
PPSSPP_CONF_OPTS += -DCMAKE_C_FLAGS="$(PPSSPP_CFLAGS)"
PPSSPP_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(PPSSPP_CXXFLAGS)"
PPSSPP_CONF_OPTS += -DCMAKE_EXE_LINKER_FLAGS="$(COMPILER_COMMONS_LDFLAGS_EXE)"
#CMAKE_LINKER_EXE_FLAGS
$(eval $(cmake-package))

all:
	cmake $(PPSSPP_CONF_OPTS) ..

echo:
	@echo $(PPSSPP_CONF_OPTS)

clean:
	rm -rf assets CMakeFiles ext include lib 
	rm -rf CMakeCache.txt cmake_install.cmake compile_commands.json
	rm -rf CTestTestfile.cmake git-version.cpp Makefile miniupnpcstrings.h
	rm -rf ppsspp.desktop PPSSPPSDL


