# xubuntu 20.04 64bit
MIYOO:=0

ifeq ($(MIYOO),2)
CC  := /home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-gcc
CPP := /home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-g++
AR  := /home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-ar cru
RANLIB := /home/wmt/work_trimui/aarch64-linux-gnu-7.5.0-linaro/bin/aarch64-linux-gnu-ranlib
else ifeq ($(MIYOO),1)
CC  := /home/wmt/work_a30/gcc-linaro-7.5.0-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
CXX := /home/wmt/work_a30/gcc-linaro-7.5.0-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++ -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
AR  := /home/wmt/work_a30/gcc-linaro-7.5.0-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-ar cru
RANLIB := /home/wmt/work_a30/gcc-linaro-7.5.0-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-ranlib
else
#CC  := gcc
#CXX := g++
#AR  := ar
CC := gcc
CPP := g++
AR := ar cru
#ar qc
RANLIB := ranlib
endif
RM := rm -rf

CCFLAGS :=

#CCFLAGS += -g 
CCFLAGS += -g0 -O0 
CCFLAGS += -D_DEBUG
CCFLAGS += -DNO_VULKAN 
#ifeq ($(MIYOO),2)
#else
CCFLAGS += -DGLEW_STATIC 
CCFLAGS += -DSDL 
#endif
CCFLAGS += -DSHARED_ZLIB 

ifeq ($(MIYOO),2)
CCFLAGS += -DUSING_EGL -DUSING_GLES2 -DUSING_FBDEV -DNO_SDLGL=1
# -DUSING_FBDEV to skip X11 headers
# -DUSING_GLES2 will skip gl/glew.h headers, see ext/native/gfx/gl_common.h
#gedit ext/native/base/PCMain.cpp, EGL init 
#EGL_BAD_ALLOC (0x3003)
else ifeq ($(MIYOO),1)
CCFLAGS += -DUSING_EGL -DUSING_GLES2 -DUSING_FBDEV -DNO_SDLGL=1
else
CCFLAGS += -DUSING_EGL -DUSING_GLES2 -DUSING_FBDEV 
#-DNO_SDLGL=1 -DFORCE_CHECK_EXT=1

#-DUSING_SDL_TEST
#-DNO_SDLGL=1
endif

CCFLAGS += -DUSE_FFMPEG=1 
CCFLAGS += -D_FILE_OFFSET_BITS=64 
CCFLAGS += -D_LARGEFILE64_SOURCE=1 
CCFLAGS += -D_XOPEN_SOURCE=700 
CCFLAGS += -D_XOPEN_SOURCE_EXTENDED 
CCFLAGS += -D__BSD_VISIBLE=1 
CCFLAGS += -D__STDC_CONSTANT_MACROS 

CCFLAGS += -DAMD_EXTENSIONS 
CCFLAGS += -DENABLE_HLSL 
CCFLAGS += -DGLSLANG_OSINCLUDE_UNIX 
CCFLAGS += -DNO_VULKAN 
CCFLAGS += -DNV_EXTENSIONS 

CCFLAGS += -I./ext/armips
CCFLAGS += -I./ext/native 
CCFLAGS += -I./ext/native/ext/rg_etc1 
CCFLAGS += -I./ext/cityhash 
CCFLAGS += -I./ext/native/ext/libzip 
CCFLAGS += -I./ext/native/ext 
CCFLAGS += -I./ext/libkirk 
CCFLAGS += -I./ext/sfmt19937 
CCFLAGS += -I./ext/xbrz 
CCFLAGS += -I./ext/xxhash 
CCFLAGS += -I./ext/snappy
#CCFLAGS += -I./ext/glew 
CCFLAGS += -I. 
CCFLAGS += -I./Common #should put it to last

ifeq ($(MIYOO),2)
CCFLAGS += -isystem /home/wmt/work_trimui/usr/include/SDL2
CCFLAGS += -isystem ./ffmpeg/linux/aarch64/include  
CCFLAGS += -I/home/wmt/work_trimui/usr/include
else ifeq ($(MIYOO),1)
CCFLAGS += -isystem /home/wmt/work_a30/staging_dir/target/usr/include/SDL2
CCFLAGS += -isystem ./ffmpeg/linux/armv7/include  
CCFLAGS += -I/home/wmt/work_a30/staging_dir/target/usr/include
else
CCFLAGS += -isystem /usr/include/SDL2
CCFLAGS += -isystem ./ffmpeg/linux/x86_64/include  
CCFLAGS += -msse2 
endif

CCFLAGS += -Wno-multichar 
###CCFLAGS += -Wno-deprecated-register 
CCFLAGS += -fno-strict-aliasing
#CCFLAGS += -Wno-psabi 
CCFLAGS += -include ./ppsspp_config.h

#CCFLAGS += -std=gnu++11 
CCFLAGS += -std=c++11 

###CCFLAGS += -Wall 
#CCFLAGS += -Wmaybe-uninitialized 
#CCFLAGS += -Wuninitialized 
#CCFLAGS += -Wunused 
#CCFLAGS += -Wunused-local-typedefs 
#CCFLAGS += -Wunused-parameter 
#CCFLAGS += -Wunused-value 
#CCFLAGS += -Wunused-variable 
#CCFLAGS += -Wunused-but-set-parameter 
#CCFLAGS += -Wunused-but-set-variable 
#CCFLAGS += -fno-exceptions 
#CCFLAGS += -Wno-reorder 

#CCFLAGS += -fPIC   








LDFLAGS := 

LDFLAGS += -lpthread 
LDFLAGS += -lrt 
LDFLAGS += -ldl 

ifeq ($(MIYOO),2)
LDFLAGS += /home/wmt/work_trimui/usr/lib/libSDL2.a 
LDFLAGS += /home/wmt/work_trimui/usr/lib/libz.a
LDFLAGS += /home/wmt/work_trimui/usr/lib/libEGL.so
LDFLAGS += /home/wmt/work_trimui/usr/lib/libGLESv2.so 
#for trimui smart pro
LDFLAGS += -lIMGegl -lsrv_um -lusc -lglslcompiler -L/home/wmt/work_trimui/usr/lib
LDFLAGS += ./ffmpeg/linux/aarch64/lib/libavformat.a 
LDFLAGS += ./ffmpeg/linux/aarch64/lib/libavcodec.a 
LDFLAGS += ./ffmpeg/linux/aarch64/lib/libswresample.a 
LDFLAGS += ./ffmpeg/linux/aarch64/lib/libswscale.a 
LDFLAGS += ./ffmpeg/linux/aarch64/lib/libavutil.a
else ifeq ($(MIYOO),1)
LDFLAGS += /home/wmt/work_a30/staging_dir/target/usr/lib/libSDL2.a 
LDFLAGS += /home/wmt/work_a30/staging_dir/target/usr/lib/libz.a
LDFLAGS += /home/wmt/work_a30/staging_dir/target/usr/lib/libEGL.so 
LDFLAGS += /home/wmt/work_a30/staging_dir/target/usr/lib/libGLESv2.so 
LDFLAGS += ./ffmpeg/linux/armv7/lib/libavformat.a 
LDFLAGS += ./ffmpeg/linux/armv7/lib/libavcodec.a 
LDFLAGS += ./ffmpeg/linux/armv7/lib/libswresample.a 
LDFLAGS += ./ffmpeg/linux/armv7/lib/libswscale.a 
LDFLAGS += ./ffmpeg/linux/armv7/lib/libavutil.a
else
LDFLAGS += /usr/lib/x86_64-linux-gnu/libSDL2.so 
LDFLAGS += /usr/lib/x86_64-linux-gnu/libz.so 
#LDFLAGS += /usr/lib/x86_64-linux-gnu/libSDL2_test.a #-DUSING_SDL_TEST
#LDFLAGS += /usr/lib/x86_64-linux-gnu/libGL.so 
#LDFLAGS += /usr/lib/x86_64-linux-gnu/libGLU.so 
LDFLAGS += /usr/lib/x86_64-linux-gnu/libEGL.so 
LDFLAGS += /usr/lib/x86_64-linux-gnu/libGLESv2.so 
LDFLAGS += ./ffmpeg/linux/x86_64/lib/libavformat.a 
LDFLAGS += ./ffmpeg/linux/x86_64/lib/libavcodec.a 
LDFLAGS += ./ffmpeg/linux/x86_64/lib/libswresample.a 
LDFLAGS += ./ffmpeg/linux/x86_64/lib/libswscale.a 
LDFLAGS += ./ffmpeg/linux/x86_64/lib/libavutil.a
endif 

LIBCORE_OBJS :=
LIBCORE_OBJS += Core/MIPS/IR/IRCompALU.o 
LIBCORE_OBJS += Core/MIPS/IR/IRCompBranch.o 
LIBCORE_OBJS += Core/MIPS/IR/IRCompFPU.o 
LIBCORE_OBJS += Core/MIPS/IR/IRCompLoadStore.o 
LIBCORE_OBJS += Core/MIPS/IR/IRCompVFPU.o 
LIBCORE_OBJS += Core/MIPS/IR/IRFrontend.o 
LIBCORE_OBJS += Core/MIPS/IR/IRInst.o 
LIBCORE_OBJS += Core/MIPS/IR/IRInterpreter.o 
LIBCORE_OBJS += Core/MIPS/IR/IRJit.o 
LIBCORE_OBJS += Core/MIPS/IR/IRPassSimplify.o 
LIBCORE_OBJS += Core/MIPS/IR/IRRegCache.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmAsm.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompALU.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompBranch.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompFPU.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompLoadStore.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompVFPU.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompVFPUNEON.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompVFPUNEONUtil.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmCompReplace.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmJit.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmRegCache.o 
LIBCORE_OBJS += Core/MIPS/ARM/ArmRegCacheFPU.o 
LIBCORE_OBJS += GPU/Common/VertexDecoderArm.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64Asm.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompALU.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompBranch.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompFPU.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompLoadStore.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompVFPU.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64CompReplace.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64Jit.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64RegCache.o 
LIBCORE_OBJS += Core/MIPS/ARM64/Arm64RegCacheFPU.o 
LIBCORE_OBJS += GPU/Common/VertexDecoderArm64.o 
LIBCORE_OBJS += Core/Util/DisArm64.o 
LIBCORE_OBJS += Core/MIPS/x86/Asm.o 
LIBCORE_OBJS += Core/MIPS/x86/CompALU.o 
LIBCORE_OBJS += Core/MIPS/x86/CompBranch.o 
LIBCORE_OBJS += Core/MIPS/x86/CompFPU.o 
LIBCORE_OBJS += Core/MIPS/x86/CompLoadStore.o 
LIBCORE_OBJS += Core/MIPS/x86/CompVFPU.o 
LIBCORE_OBJS += Core/MIPS/x86/CompReplace.o 
LIBCORE_OBJS += Core/MIPS/x86/Jit.o 
LIBCORE_OBJS += Core/MIPS/x86/JitSafeMem.o 
LIBCORE_OBJS += Core/MIPS/x86/RegCache.o 
LIBCORE_OBJS += Core/MIPS/x86/RegCacheFPU.o 
LIBCORE_OBJS += GPU/Common/VertexDecoderX86.o 
LIBCORE_OBJS += GPU/Software/SamplerX86.o 
LIBCORE_OBJS += Core/MIPS/MIPS/MipsJit.o 
LIBCORE_OBJS += GPU/Common/VertexDecoderFake.o 
LIBCORE_OBJS += Core/AVIDump.o 
LIBCORE_OBJS += Core/WaveFile.o 
LIBCORE_OBJS += Core/Config.o 
LIBCORE_OBJS += Core/Core.o 
LIBCORE_OBJS += Core/Compatibility.o 
LIBCORE_OBJS += Core/CoreTiming.o 
LIBCORE_OBJS += Core/CwCheat.o 
LIBCORE_OBJS += Core/HDRemaster.o 
LIBCORE_OBJS += Core/Debugger/Breakpoints.o 
LIBCORE_OBJS += Core/Debugger/SymbolMap.o 
LIBCORE_OBJS += Core/Debugger/DisassemblyManager.o 
LIBCORE_OBJS += Core/Dialog/PSPDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPGamedataInstallDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPMsgDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPNetconfDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPOskDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPPlaceholderDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPSaveDialog.o 
LIBCORE_OBJS += Core/Dialog/PSPScreenshotDialog.o 
LIBCORE_OBJS += Core/Dialog/SavedataParam.o 
LIBCORE_OBJS += Core/ELF/ElfReader.o 
LIBCORE_OBJS += Core/ELF/PBPReader.o 
LIBCORE_OBJS += Core/ELF/PrxDecrypter.o 
LIBCORE_OBJS += Core/ELF/ParamSFO.o 
LIBCORE_OBJS += Core/FileSystems/tlzrc.o 
LIBCORE_OBJS += Core/FileSystems/BlobFileSystem.o 
LIBCORE_OBJS += Core/FileSystems/BlockDevices.o 
LIBCORE_OBJS += Core/FileSystems/DirectoryFileSystem.o 
LIBCORE_OBJS += Core/FileSystems/FileSystem.o 
LIBCORE_OBJS += Core/FileSystems/ISOFileSystem.o 
LIBCORE_OBJS += Core/FileSystems/MetaFileSystem.o 
LIBCORE_OBJS += Core/FileSystems/VirtualDiscFileSystem.o 
LIBCORE_OBJS += Core/Font/PGF.o 
LIBCORE_OBJS += Core/HLE/HLE.o 
LIBCORE_OBJS += Core/HLE/ReplaceTables.o 
LIBCORE_OBJS += Core/HLE/HLEHelperThread.o 
LIBCORE_OBJS += Core/HLE/HLETables.o 
LIBCORE_OBJS += Core/HLE/KUBridge.o 
LIBCORE_OBJS += Core/HLE/__sceAudio.o 
LIBCORE_OBJS += Core/HLE/sceAdler.o 
LIBCORE_OBJS += Core/HLE/sceAtrac.o 
LIBCORE_OBJS += Core/HLE/sceAudio.o 
LIBCORE_OBJS += Core/HLE/sceAudiocodec.o 
LIBCORE_OBJS += Core/HLE/sceAudioRouting.o 
LIBCORE_OBJS += Core/HLE/sceCcc.o 
LIBCORE_OBJS += Core/HLE/sceChnnlsv.o 
LIBCORE_OBJS += Core/HLE/sceCtrl.o 
LIBCORE_OBJS += Core/HLE/sceDeflt.o 
LIBCORE_OBJS += Core/HLE/sceDisplay.o 
LIBCORE_OBJS += Core/HLE/sceDmac.o 
LIBCORE_OBJS += Core/HLE/sceG729.o 
LIBCORE_OBJS += Core/HLE/sceGameUpdate.o 
LIBCORE_OBJS += Core/HLE/sceGe.o 
LIBCORE_OBJS += Core/HLE/sceFont.o 
LIBCORE_OBJS += Core/HLE/sceHeap.o 
LIBCORE_OBJS += Core/HLE/sceHprm.o 
LIBCORE_OBJS += Core/HLE/sceHttp.o 
LIBCORE_OBJS += Core/HLE/sceImpose.o 
LIBCORE_OBJS += Core/HLE/sceIo.o 
LIBCORE_OBJS += Core/HLE/sceJpeg.o 
LIBCORE_OBJS += Core/HLE/sceKernel.o 
LIBCORE_OBJS += Core/HLE/sceKernelAlarm.o 
LIBCORE_OBJS += Core/HLE/sceKernelEventFlag.o 
LIBCORE_OBJS += Core/HLE/sceKernelInterrupt.o 
LIBCORE_OBJS += Core/HLE/sceKernelMbx.o 
LIBCORE_OBJS += Core/HLE/sceKernelMemory.o 
LIBCORE_OBJS += Core/HLE/sceKernelModule.o 
LIBCORE_OBJS += Core/HLE/sceKernelMsgPipe.o 
LIBCORE_OBJS += Core/HLE/sceKernelMutex.o 
LIBCORE_OBJS += Core/HLE/sceKernelSemaphore.o 
LIBCORE_OBJS += Core/HLE/sceKernelThread.o 
LIBCORE_OBJS += Core/HLE/sceKernelTime.o 
LIBCORE_OBJS += Core/HLE/sceKernelVTimer.o 
LIBCORE_OBJS += Core/HLE/sceMpeg.o 
LIBCORE_OBJS += Core/HLE/sceNet.o 
LIBCORE_OBJS += Core/HLE/sceNetAdhoc.o 
LIBCORE_OBJS += Core/HLE/proAdhoc.o 
LIBCORE_OBJS += Core/HLE/proAdhocServer.o 
LIBCORE_OBJS += Core/HLE/sceOpenPSID.o 
LIBCORE_OBJS += Core/HLE/sceP3da.o 
LIBCORE_OBJS += Core/HLE/sceMt19937.o 
LIBCORE_OBJS += Core/HLE/sceMd5.o 
LIBCORE_OBJS += Core/HLE/sceMp4.o 
LIBCORE_OBJS += Core/HLE/sceMp3.o 
LIBCORE_OBJS += Core/HLE/sceParseHttp.o 
LIBCORE_OBJS += Core/HLE/sceParseUri.o 
LIBCORE_OBJS += Core/HLE/scePower.o 
LIBCORE_OBJS += Core/HLE/scePsmf.o 
LIBCORE_OBJS += Core/HLE/sceRtc.o 
LIBCORE_OBJS += Core/HLE/sceSas.o 
LIBCORE_OBJS += Core/HLE/sceSfmt19937.o 
LIBCORE_OBJS += Core/HLE/sceSha256.o 
LIBCORE_OBJS += Core/HLE/sceSsl.o 
LIBCORE_OBJS += Core/HLE/sceUmd.o 
LIBCORE_OBJS += Core/HLE/sceUsb.o 
LIBCORE_OBJS += Core/HLE/sceUsbCam.o 
LIBCORE_OBJS += Core/HLE/sceUsbGps.o 
LIBCORE_OBJS += Core/HLE/sceUtility.o 
LIBCORE_OBJS += Core/HLE/sceVaudio.o 
LIBCORE_OBJS += Core/HLE/scePspNpDrm_user.o 
LIBCORE_OBJS += Core/HLE/sceNp.o 
LIBCORE_OBJS += Core/HLE/scePauth.o 
LIBCORE_OBJS += Core/HW/SimpleAudioDec.o 
LIBCORE_OBJS += Core/HW/AsyncIOManager.o 
LIBCORE_OBJS += Core/HW/MediaEngine.o 
LIBCORE_OBJS += Core/HW/MpegDemux.o 
LIBCORE_OBJS += Core/HW/MemoryStick.o 
LIBCORE_OBJS += Core/HW/SasAudio.o 
LIBCORE_OBJS += Core/HW/SasReverb.o 
LIBCORE_OBJS += Core/HW/StereoResampler.o 
LIBCORE_OBJS += Core/Host.o 
LIBCORE_OBJS += Core/Loaders.o 
LIBCORE_OBJS += Core/FileLoaders/CachingFileLoader.o 
LIBCORE_OBJS += Core/FileLoaders/DiskCachingFileLoader.o 
LIBCORE_OBJS += Core/FileLoaders/HTTPFileLoader.o 
LIBCORE_OBJS += Core/FileLoaders/LocalFileLoader.o 
LIBCORE_OBJS += Core/FileLoaders/RamCachingFileLoader.o 
LIBCORE_OBJS += Core/FileLoaders/RetryingFileLoader.o 
LIBCORE_OBJS += Core/MIPS/JitCommon/JitCommon.o 
LIBCORE_OBJS += Core/MIPS/JitCommon/JitBlockCache.o 
LIBCORE_OBJS += Core/MIPS/JitCommon/JitState.o 
LIBCORE_OBJS += Core/MIPS/MIPS.o 
LIBCORE_OBJS += Core/MIPS/MIPSAnalyst.o 
LIBCORE_OBJS += Core/MIPS/MIPSCodeUtils.o 
LIBCORE_OBJS += Core/MIPS/MIPSDebugInterface.o 
LIBCORE_OBJS += Core/MIPS/MIPSDis.o 
LIBCORE_OBJS += Core/MIPS/MIPSDisVFPU.o 
LIBCORE_OBJS += Core/MIPS/MIPSInt.o 
LIBCORE_OBJS += Core/MIPS/MIPSIntVFPU.o 
LIBCORE_OBJS += Core/MIPS/MIPSStackWalk.o 
LIBCORE_OBJS += Core/MIPS/MIPSTables.o 
LIBCORE_OBJS += Core/MIPS/MIPSVFPUUtils.o 
LIBCORE_OBJS += Core/MIPS/MIPSAsm.o 
LIBCORE_OBJS += Core/MemMap.o 
LIBCORE_OBJS += Core/MemMapFunctions.o 
LIBCORE_OBJS += Core/PSPLoaders.o 
LIBCORE_OBJS += Core/Reporting.o 
LIBCORE_OBJS += Core/SaveState.o 
LIBCORE_OBJS += Core/Screenshot.o 
LIBCORE_OBJS += Core/System.o 
LIBCORE_OBJS += Core/TextureReplacer.o 
LIBCORE_OBJS += Core/Util/AudioFormat.o 
LIBCORE_OBJS += Core/Util/GameManager.o 
LIBCORE_OBJS += Core/Util/BlockAllocator.o 
LIBCORE_OBJS += Core/Util/PPGeDraw.o 
LIBCORE_OBJS += Core/Util/ppge_atlas.o 
LIBCORE_OBJS += GPU/GLES/DepalettizeShaderGLES.o 
LIBCORE_OBJS += GPU/GLES/GPU_GLES.o 
LIBCORE_OBJS += GPU/GLES/FragmentShaderGeneratorGLES.o 
LIBCORE_OBJS += GPU/GLES/FragmentTestCacheGLES.o 
LIBCORE_OBJS += GPU/GLES/FramebufferManagerGLES.o 
LIBCORE_OBJS += GPU/GLES/ShaderManagerGLES.o 
LIBCORE_OBJS += GPU/GLES/StateMappingGLES.o 
LIBCORE_OBJS += GPU/GLES/StencilBufferGLES.o 
LIBCORE_OBJS += GPU/GLES/TextureCacheGLES.o 
LIBCORE_OBJS += GPU/GLES/TextureScalerGLES.o 
LIBCORE_OBJS += GPU/GLES/DrawEngineGLES.o 
LIBCORE_OBJS += GPU/GLES/VertexShaderGeneratorGLES.o 
LIBCORE_OBJS += GPU/Common/DepalettizeShaderCommon.o 
LIBCORE_OBJS += GPU/Common/FramebufferCommon.o 
LIBCORE_OBJS += GPU/Common/GPUDebugInterface.o 
LIBCORE_OBJS += GPU/Common/GPUStateUtils.o 
LIBCORE_OBJS += GPU/Common/DrawEngineCommon.o 
LIBCORE_OBJS += GPU/Common/ShaderId.o 
LIBCORE_OBJS += GPU/Common/ShaderUniforms.o 
LIBCORE_OBJS += GPU/Common/ShaderCommon.o 
LIBCORE_OBJS += GPU/Common/ShaderTranslation.o 
LIBCORE_OBJS += GPU/Common/SplineCommon.o 
LIBCORE_OBJS += GPU/Common/StencilCommon.o 
LIBCORE_OBJS += GPU/Common/SoftwareTransformCommon.o 
LIBCORE_OBJS += GPU/Common/VertexDecoderCommon.o 
LIBCORE_OBJS += GPU/Common/TransformCommon.o 
LIBCORE_OBJS += GPU/Common/IndexGenerator.o 
LIBCORE_OBJS += GPU/Common/TextureDecoder.o 

LIBCORE_OBJS += GPU/Common/TextureDecoderNEON.o  #for arm and aarch64

LIBCORE_OBJS += GPU/Common/TextureCacheCommon.o 
LIBCORE_OBJS += GPU/Common/TextureScalerCommon.o 
LIBCORE_OBJS += GPU/Common/PostShader.o 
LIBCORE_OBJS += GPU/Debugger/Breakpoints.o 
LIBCORE_OBJS += GPU/Debugger/Record.o 
LIBCORE_OBJS += GPU/Debugger/Stepping.o 
LIBCORE_OBJS += GPU/GeDisasm.o 
LIBCORE_OBJS += GPU/GPU.o 
LIBCORE_OBJS += GPU/GPUCommon.o 
LIBCORE_OBJS += GPU/GPUState.o 
LIBCORE_OBJS += GPU/Math3D.o 
LIBCORE_OBJS += GPU/Null/NullGpu.o 
LIBCORE_OBJS += GPU/Software/Clipper.o 
LIBCORE_OBJS += GPU/Software/Lighting.o 
LIBCORE_OBJS += GPU/Software/Rasterizer.o 
LIBCORE_OBJS += GPU/Software/Sampler.o 
LIBCORE_OBJS += GPU/Software/SoftGpu.o 
LIBCORE_OBJS += GPU/Software/TransformUnit.o 
LIBCORE_OBJS += git-version.o 
LIBCORE_OBJS += ext/disarm.o

LIBCOMMON_OBJS := 
LIBCOMMON_OBJS += Common/ABI.o 
LIBCOMMON_OBJS += Common/CPUDetect.o 
LIBCOMMON_OBJS += Common/Thunk.o 
LIBCOMMON_OBJS += Common/x64Analyzer.o 
LIBCOMMON_OBJS += Common/x64Emitter.o 
LIBCOMMON_OBJS += Common/ArmCPUDetect.o 
LIBCOMMON_OBJS += Common/ArmEmitter.o 
LIBCOMMON_OBJS += Common/ColorConvNEON.o 
LIBCOMMON_OBJS += Common/Arm64Emitter.o 
LIBCOMMON_OBJS += Core/Util/DisArm64.o 
LIBCOMMON_OBJS += Common/MipsCPUDetect.o 
LIBCOMMON_OBJS += Common/MipsEmitter.o 
LIBCOMMON_OBJS += Common/stdafx.o 
LIBCOMMON_OBJS += Common/ColorConv.o 
LIBCOMMON_OBJS += Common/ChunkFile.o 
LIBCOMMON_OBJS += Common/ConsoleListener.o 
LIBCOMMON_OBJS += Common/Crypto/md5.o 
LIBCOMMON_OBJS += Common/Crypto/sha1.o 
LIBCOMMON_OBJS += Common/Crypto/sha256.o 
LIBCOMMON_OBJS += Common/FileUtil.o 
LIBCOMMON_OBJS += Common/KeyMap.o 
LIBCOMMON_OBJS += Common/LogManager.o 
LIBCOMMON_OBJS += Common/MemArenaAndroid.o 
LIBCOMMON_OBJS += Common/MemArenaDarwin.o 
LIBCOMMON_OBJS += Common/MemArenaPosix.o 
LIBCOMMON_OBJS += Common/MemArenaWin32.o 
LIBCOMMON_OBJS += Common/MemoryUtil.o 
LIBCOMMON_OBJS += Common/Misc.o 
LIBCOMMON_OBJS += Common/MsgHandler.o 
LIBCOMMON_OBJS += Common/OSVersion.o 
LIBCOMMON_OBJS += Common/StringUtils.o 
LIBCOMMON_OBJS += Common/ThreadPools.o 
LIBCOMMON_OBJS += Common/Timer.o

LIBSNAPPY_OBJS := 
LIBSNAPPY_OBJS += ext/snappy/snappy-c.o 
LIBSNAPPY_OBJS += ext/snappy/snappy.o

LIBNATIVE_OBJS := 
LIBNATIVE_OBJS += SDL/SDLJoystick.o 
LIBNATIVE_OBJS += ext/native/base/PCMain.o 
LIBNATIVE_OBJS += ext/native/base/backtrace.o 
LIBNATIVE_OBJS += ext/native/base/buffer.o 
LIBNATIVE_OBJS += ext/native/base/colorutil.o 
LIBNATIVE_OBJS += ext/native/base/display.o 
LIBNATIVE_OBJS += ext/native/base/stringutil.o 
LIBNATIVE_OBJS += ext/native/base/timeutil.o 
LIBNATIVE_OBJS += ext/native/data/compression.o 
LIBNATIVE_OBJS += ext/native/ext/vjson/json.o 
LIBNATIVE_OBJS += ext/native/ext/vjson/block_allocator.o 
LIBNATIVE_OBJS += ext/native/file/chunk_file.o 
LIBNATIVE_OBJS += ext/native/file/fd_util.o 
LIBNATIVE_OBJS += ext/native/file/file_util.o 
LIBNATIVE_OBJS += ext/native/file/free.o 
LIBNATIVE_OBJS += ext/native/file/ini_file.o 
LIBNATIVE_OBJS += ext/native/file/path.o 
LIBNATIVE_OBJS += ext/native/file/zip_read.o 
LIBNATIVE_OBJS += ext/native/gfx/GLStateCache.o 
LIBNATIVE_OBJS += ext/native/gfx/gl_debug_log.o 
LIBNATIVE_OBJS += ext/native/gfx/gl_lost_manager.o 
LIBNATIVE_OBJS += ext/native/gfx/texture_atlas.o 
LIBNATIVE_OBJS += ext/native/gfx/d3d9_shader.o 
LIBNATIVE_OBJS += ext/native/gfx/d3d9_state.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/draw_buffer.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/draw_text.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/draw_text_win.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/draw_text_qt.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/draw_text_android.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/gpu_features.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/glsl_program.o 
LIBNATIVE_OBJS += ext/native/gfx_es2/gl3stub.o 
LIBNATIVE_OBJS += ext/native/i18n/i18n.o 
LIBNATIVE_OBJS += ext/native/image/png_load.o 
LIBNATIVE_OBJS += ext/native/image/zim_load.o 
LIBNATIVE_OBJS += ext/native/image/zim_save.o 
LIBNATIVE_OBJS += ext/native/input/gesture_detector.o 
LIBNATIVE_OBJS += ext/native/input/input_state.o 
LIBNATIVE_OBJS += ext/native/math/fast/fast_math.o 
LIBNATIVE_OBJS += ext/native/math/fast/fast_matrix.o 
LIBNATIVE_OBJS += ext/native/math/fast/fast_matrix_neon.o 
LIBNATIVE_OBJS += ext/native/math/fast/fast_matrix_sse.o 
LIBNATIVE_OBJS += ext/native/math/dataconv.o 
LIBNATIVE_OBJS += ext/native/math/curves.o 
LIBNATIVE_OBJS += ext/native/math/expression_parser.o 
LIBNATIVE_OBJS += ext/native/math/lin/matrix4x4.o 
LIBNATIVE_OBJS += ext/native/math/lin/plane.o 
LIBNATIVE_OBJS += ext/native/math/lin/quat.o 
LIBNATIVE_OBJS += ext/native/math/lin/vec3.o 
LIBNATIVE_OBJS += ext/native/math/math_util.o 
LIBNATIVE_OBJS += ext/native/net/http_client.o 
LIBNATIVE_OBJS += ext/native/net/http_headers.o 
LIBNATIVE_OBJS += ext/native/net/http_server.o 
LIBNATIVE_OBJS += ext/native/net/resolve.o 
LIBNATIVE_OBJS += ext/native/net/sinks.o 
LIBNATIVE_OBJS += ext/native/net/url.o 
LIBNATIVE_OBJS += ext/native/profiler/profiler.o 
LIBNATIVE_OBJS += ext/native/thin3d/thin3d.o 
LIBNATIVE_OBJS += ext/native/thin3d/thin3d_gl.o 
LIBNATIVE_OBJS += ext/native/thread/executor.o 
LIBNATIVE_OBJS += ext/native/thread/prioritizedworkqueue.o 
LIBNATIVE_OBJS += ext/native/thread/threadutil.o 
LIBNATIVE_OBJS += ext/native/thread/threadpool.o 
LIBNATIVE_OBJS += ext/native/ui/screen.o 
LIBNATIVE_OBJS += ext/native/ui/ui.o 
LIBNATIVE_OBJS += ext/native/ui/ui_context.o 
LIBNATIVE_OBJS += ext/native/ui/ui_screen.o 
LIBNATIVE_OBJS += ext/native/ui/view.o 
LIBNATIVE_OBJS += ext/native/ui/viewgroup.o 
LIBNATIVE_OBJS += ext/native/util/hash/hash.o 
LIBNATIVE_OBJS += ext/native/util/text/utf8.o 
LIBNATIVE_OBJS += ext/native/util/text/parsers.o 
LIBNATIVE_OBJS += ext/native/util/text/wrap_text.o 
LIBNATIVE_OBJS += ext/native/ext/jpge/jpgd.o 
LIBNATIVE_OBJS += ext/native/ext/jpge/jpge.o

LIBLIBZIP_OBJS :=
LIBLIBZIP_OBJS += ext/native/ext/libzip/mkstemp.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_add.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_add_dir.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_close.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_delete.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_dirent.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_entry_free.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_entry_new.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_err_str.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error_clear.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error_get.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error_get_sys_type.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error_strerror.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_error_to_str.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_fclose.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_file_error_clear.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_file_error_get.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_file_get_offset.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_file_strerror.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_filerange_crc.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_fopen.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_fopen_index.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_fread.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_free.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_get_archive_comment.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_get_archive_flag.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_get_file_comment.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_get_name.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_get_num_files.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_memdup.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_name_locate.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_new.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_open.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_rename.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_replace.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_set_archive_comment.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_set_archive_flag.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_set_file_comment.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_set_name.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_buffer.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_file.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_filep.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_free.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_function.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_source_zip.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_stat.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_stat_index.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_stat_init.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_strerror.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_unchange.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_unchange_all.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_unchange_archive.o 
LIBLIBZIP_OBJS += ext/native/ext/libzip/zip_unchange_data.o

LIBPNG17_OBJS :=
LIBPNG17_OBJS +=  ext/native/ext/libpng17/png.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngerror.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngget.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngmem.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngpread.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngread.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngrio.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngrtran.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngrutil.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngset.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngtrans.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngwio.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngwrite.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngwtran.o 
LIBPNG17_OBJS +=  ext/native/ext/libpng17/pngwutil.o

LIBRG_ETCL_OBJS := 
LIBRG_ETCL_OBJS += ext/native/ext/rg_etc1/rg_etc1.o

LIBVJSON_OBJS := 
LIBVJSON_OBJS += ext/native/ext/vjson/json.o 
LIBVJSON_OBJS += ext/native/ext/vjson/block_allocator.o

LIBUDIS86_OBJS := 
LIBUDIS86_OBJS += ext/udis86/decode.o 
LIBUDIS86_OBJS += ext/udis86/itab.o 
LIBUDIS86_OBJS += ext/udis86/syn-att.o 
LIBUDIS86_OBJS += ext/udis86/syn-intel.o 
LIBUDIS86_OBJS += ext/udis86/syn.o 
LIBUDIS86_OBJS += ext/udis86/udis86.o

LIBGLEW_OBJS := 
ifeq ($(MIYOO),2)
LIBGLEW_OBJS += 
else ifeq ($(MIYOO),2)
LIBGLEW_OBJS += 
else
#LIBGLEW_OBJS += ext/glew/glew.o
endif

LIBKIRK_OBJS := 
LIBKIRK_OBJS += ext/libkirk/AES.o 
LIBKIRK_OBJS += ext/libkirk/amctrl.o 
LIBKIRK_OBJS += ext/libkirk/SHA1.o 
LIBKIRK_OBJS += ext/libkirk/bn.o 
LIBKIRK_OBJS += ext/libkirk/ec.o 
LIBKIRK_OBJS += ext/libkirk/kirk_engine.o

LIBCITYHASH_OBJS := 
LIBCITYHASH_OBJS += ext/native/ext/cityhash/city.o

LIBSFMT19937_OBJS := 
LIBSFMT19937_OBJS += ext/sfmt19937/SFMT.o

LIBXBRZ_OBJS := 
LIBXBRZ_OBJS += ext/xbrz/xbrz.o

LIBXXHASH_OBJS := 
LIBXXHASH_OBJS += ext/xxhash.o

LIBSPIRV_OBJS := 
LIBSPIRV_OBJS += ext/glslang/SPIRV/GlslangToSpv.o 
LIBSPIRV_OBJS += ext/glslang/SPIRV/InReadableOrder.o 
LIBSPIRV_OBJS += ext/glslang/SPIRV/Logger.o 
LIBSPIRV_OBJS += ext/glslang/SPIRV/SpvBuilder.o 
LIBSPIRV_OBJS += ext/glslang/SPIRV/doc.o 
LIBSPIRV_OBJS += ext/glslang/SPIRV/disassemble.o

LIBGLSLANG_OBJS := 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/glslang_tab.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/Constant.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/iomapper.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/InfoSink.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/Initialize.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/IntermTraverse.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/Intermediate.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/ParseContextBase.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/ParseHelper.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/PoolAlloc.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/RemoveTree.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/Scan.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/ShaderLang.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/SymbolTable.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/Versions.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/intermOut.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/limits.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/linkValidate.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/parseConst.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/reflection.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/preprocessor/Pp.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/preprocessor/PpAtom.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/preprocessor/PpContext.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/preprocessor/PpScanner.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/preprocessor/PpTokens.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/MachineIndependent/propagateNoContraction.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/GenericCodeGen/CodeGen.o 
LIBGLSLANG_OBJS += ext/glslang/glslang/GenericCodeGen/Link.o

LIBOGLCOMPILER_OBJS := 
LIBOGLCOMPILER_OBJS += ext/glslang/OGLCompilersDLL/InitializeDll.o

LIBOSDEPENDENT_OBJS := 
LIBOSDEPENDENT_OBJS += ext/glslang/glslang/OSDependent/Unix/ossource.o

LIBHLSL_OBJS := 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslAttributes.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslParseHelper.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslScanContext.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslOpMap.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslTokenStream.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslGrammar.o 
LIBHLSL_OBJS += ext/glslang/hlsl/hlslParseables.o

LIBSPVREMAPPER_OBJS := 
LIBSPVREMAPPER_OBJS += ext/glslang/SPIRV/SPVRemapper.o 
LIBSPVREMAPPER_OBJS += ext/glslang/SPIRV/doc.o

LIBARMIPS_OBJS := 
LIBARMIPS_OBJS += ext/armips/Archs/Architecture.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/Arm.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/ArmOpcodes.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/ArmParser.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/ArmRelocator.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/CArmInstruction.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/CThumbInstruction.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/Pool.o 
LIBARMIPS_OBJS += ext/armips/Archs/ARM/ThumbOpcodes.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/CMipsInstruction.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/Mips.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/MipsElfFile.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/MipsMacros.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/MipsOpcodes.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/MipsParser.o 
LIBARMIPS_OBJS += ext/armips/Archs/MIPS/PsxRelocator.o 
LIBARMIPS_OBJS += ext/armips/Commands/CAssemblerCommand.o 
LIBARMIPS_OBJS += ext/armips/Commands/CAssemblerLabel.o 
LIBARMIPS_OBJS += ext/armips/Commands/CDirectiveArea.o 
LIBARMIPS_OBJS += ext/armips/Commands/CDirectiveConditional.o 
LIBARMIPS_OBJS += ext/armips/Commands/CDirectiveData.o 
LIBARMIPS_OBJS += ext/armips/Commands/CDirectiveFile.o 
LIBARMIPS_OBJS += ext/armips/Commands/CDirectiveMessage.o 
LIBARMIPS_OBJS += ext/armips/Commands/CommandSequence.o 
LIBARMIPS_OBJS += ext/armips/Core/ELF/ElfRelocator.o 
LIBARMIPS_OBJS += ext/armips/Core/ELF/ElfFile.o 
LIBARMIPS_OBJS += ext/armips/Core/Assembler.o 
LIBARMIPS_OBJS += ext/armips/Core/Common.o 
LIBARMIPS_OBJS += ext/armips/Core/Expression.o 
LIBARMIPS_OBJS += ext/armips/Core/ExpressionFunctions.o 
LIBARMIPS_OBJS += ext/armips/Core/FileManager.o 
LIBARMIPS_OBJS += ext/armips/Core/Misc.o 
LIBARMIPS_OBJS += ext/armips/Core/SymbolData.o 
LIBARMIPS_OBJS += ext/armips/Core/SymbolTable.o 
LIBARMIPS_OBJS += ext/armips/Parser/DirectivesParser.o 
LIBARMIPS_OBJS += ext/armips/Parser/ExpressionParser.o 
LIBARMIPS_OBJS += ext/armips/Parser/Parser.o 
LIBARMIPS_OBJS += ext/armips/Parser/Tokenizer.o 
LIBARMIPS_OBJS += ext/armips/Util/ByteArray.o 
LIBARMIPS_OBJS += ext/armips/Util/CRC.o 
LIBARMIPS_OBJS += ext/armips/Util/EncodingTable.o 
LIBARMIPS_OBJS += ext/armips/Util/FileClasses.o 
LIBARMIPS_OBJS += ext/armips/Util/Util.o

OBJS := 
OBJS += android/jni/TestRunner.o
OBJS += UI/NativeApp.o
OBJS += UI/BackgroundAudio.o
OBJS += UI/DevScreens.o
OBJS += UI/DisplayLayoutEditor.o
OBJS += UI/DisplayLayoutScreen.o
OBJS += UI/EmuScreen.o
OBJS += UI/GameInfoCache.o
OBJS += UI/MainScreen.o
OBJS += UI/MiscScreens.o
OBJS += UI/PauseScreen.o
OBJS += UI/GameScreen.o
OBJS += UI/GameSettingsScreen.o
OBJS += UI/TiltAnalogSettingsScreen.o
OBJS += UI/TiltEventProcessor.o
OBJS += UI/TouchControlLayoutScreen.o
OBJS += UI/TouchControlVisibilityScreen.o
OBJS += UI/GamepadEmu.o
OBJS += UI/OnScreenDisplay.o
OBJS += UI/ControlMappingScreen.o
OBJS += UI/RemoteISOScreen.o
OBJS += UI/ReportScreen.o
OBJS += UI/SavedataScreen.o
OBJS += UI/Store.o
OBJS += UI/CwCheatScreen.o
OBJS += UI/InstallZipScreen.o
OBJS += UI/ProfilerDraw.o
OBJS += UI/ui_atlas.o
OBJS += UI/TextureUtil.o
OBJS += UI/ComboKeyMappingScreen.o

LIBS := 
LIBS += libCore.a
LIBS += libCommon.a
LIBS += libsnappy.a
LIBS += libnative.a
LIBS += liblibzip.a
LIBS += libpng17.a
LIBS += librg_etc1.a
LIBS += libvjson.a
LIBS += libudis86.a
LIBS += libglew.a
LIBS += libkirk.a
LIBS += libcityhash.a
LIBS += libsfmt19937.a
LIBS += libxbrz.a
LIBS += libxxhash.a
LIBS += libSPIRV.a
LIBS += libglslang.a
LIBS += libOGLCompiler.a
LIBS += libOSDependent.a
LIBS += libHLSL.a
LIBS += libSPVRemapper.a
LIBS += libarmips.a

all: PPSSPPSDL

PPSSPPSDL: $(LIBS) $(OBJS)
	$(CPP) $(CCFLAGS) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

libCore.a: $(LIBCORE_OBJS)
	$(AR) $@ $(LIBCORE_OBJS)
	$(RANLIB) $@

libCommon.a: $(LIBCOMMON_OBJS)
	$(AR) $@ $(LIBCOMMON_OBJS)
	$(RANLIB) $@

libsnappy.a: $(LIBSNAPPY_OBJS)
	$(AR) $@ $(LIBSNAPPY_OBJS)
	$(RANLIB) $@
	
libnative.a: $(LIBNATIVE_OBJS)
	$(AR) $@ $(LIBNATIVE_OBJS)
	$(RANLIB) $@
	
liblibzip.a: $(LIBLIBZIP_OBJS)
	$(AR) $@ $(LIBLIBZIP_OBJS)
	$(RANLIB) $@
	
libpng17.a: $(LIBPNG17_OBJS)
	$(AR) $@ $(LIBPNG17_OBJS)
	$(RANLIB) $@
	
librg_etc1.a: $(LIBRG_ETCL_OBJS)
	$(AR) $@ $(LIBRG_ETCL_OBJS)
	$(RANLIB) $@
	
libvjson.a: $(LIBVJSON_OBJS)
	$(AR) $@ $(LIBVJSON_OBJS)
	$(RANLIB) $@
	
libudis86.a: $(LIBUDIS86_OBJS)
	$(AR) $@ $(LIBUDIS86_OBJS)
	$(RANLIB) $@
	
libglew.a: $(LIBGLEW_OBJS)
	$(AR) $@ $(LIBGLEW_OBJS)
	$(RANLIB) $@
	
libkirk.a: $(LIBKIRK_OBJS)
	$(AR) $@ $(LIBKIRK_OBJS)
	$(RANLIB) $@
	
libcityhash.a: $(LIBCITYHASH_OBJS)
	$(AR) $@ $(LIBCITYHASH_OBJS)
	$(RANLIB) $@
	
libsfmt19937.a: $(LIBSFMT19937_OBJS)
	$(AR) $@ $(LIBSFMT19937_OBJS)
	$(RANLIB) $@
	
libxbrz.a: $(LIBXBRZ_OBJS)
	$(AR) $@ $(LIBXBRZ_OBJS)
	$(RANLIB) $@
	
libxxhash.a: $(LIBXXHASH_OBJS)
	$(AR) $@ $(LIBXXHASH_OBJS)
	$(RANLIB) $@
	
libSPIRV.a: $(LIBSPIRV_OBJS)
	$(AR) $@ $(LIBSPIRV_OBJS)
	$(RANLIB) $@
	
libglslang.a: $(LIBGLSLANG_OBJS)
	$(AR) $@ $(LIBGLSLANG_OBJS)
	$(RANLIB) $@
	
libOGLCompiler.a: $(LIBOGLCOMPILER_OBJS)
	$(AR) $@ $(LIBOGLCOMPILER_OBJS)
	$(RANLIB) $@
	
libOSDependent.a: $(LIBOSDEPENDENT_OBJS)
	$(AR) $@ $(LIBOSDEPENDENT_OBJS)
	$(RANLIB) $@
	
libHLSL.a: $(LIBHLSL_OBJS)
	$(AR) $@ $(LIBHLSL_OBJS)
	$(RANLIB) $@
	
libSPVRemapper.a: $(LIBSPVREMAPPER_OBJS)
	$(AR) $@ $(LIBSPVREMAPPER_OBJS)
	$(RANLIB) $@
	
libarmips.a: $(LIBARMIPS_OBJS)
	$(AR) $@ $(LIBARMIPS_OBJS)
	$(RANLIB) $@

%.o : %.S
	$(CC) $(CCFLAGS) -o $@ -c $<	
	
%.o : %.c
	$(CC) $(CCFLAGS) -o $@ -c $<

%.o : %.cpp
	$(CPP) $(CCFLAGS) -o $@ -c $<


clean: 
	$(RM) PPSSPPSDL $(LIBS) $(OBJS)
	$(RM) $(LIBCORE_OBJS)
	$(RM) $(LIBCOMMON_OBJS)
	$(RM) $(LIBSNAPPY_OBJS)
	$(RM) $(LIBNATIVE_OBJS)
	$(RM) $(LIBLIBZIP_OBJS)
	$(RM) $(LIBPNG17_OBJS)
	$(RM) $(LIBRG_ETCL_OBJS)
	$(RM) $(LIBVJSON_OBJS)
	$(RM) $(LIBUDIS86_OBJS)
	$(RM) $(LIBGLEW_OBJS) ext/glew/*.o
	$(RM) $(LIBKIRK_OBJS)
	$(RM) $(LIBCITYHASH_OBJS)
	$(RM) $(LIBSFMT19937_OBJS)
	$(RM) $(LIBXBRZ_OBJS)
	$(RM) $(LIBXXHASH_OBJS)
	$(RM) $(LIBSPIRV_OBJS)
	$(RM) $(LIBGLSLANG_OBJS)
	$(RM) $(LIBOGLCOMPILER_OBJS)
	$(RM) $(LIBOSDEPENDENT_OBJS)
	$(RM) $(LIBHLSL_OBJS)
	$(RM) $(LIBSPVREMAPPER_OBJS)
	$(RM) $(LIBARMIPS_OBJS)

test:
	./PPSSPPSDL

debug:
	gdb ./PPSSPPSDL


