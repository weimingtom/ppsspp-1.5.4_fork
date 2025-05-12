# ppsspp-1.5.4_fork
Try to cross compile PPSSPP 1.5.4 GLES no X11 version

## Original Readme  
* https://github.com/hrydgard/ppsspp/blob/v1.5.4/README.md
* https://github.com/hrydgard/ppsspp/blob/v1.5.4/history.md  

## Original code compare
* ppsspp-1.5.4.zip  
No code changed, but not include dependencies  
see https://github.com/hrydgard/ppsspp/releases/tag/v1.5.4   
see https://github.com/hrydgard/ppsspp/archive/refs/tags/v1.5.4.zip  
* ppsspp_v1.5.4_min.tar.gz  
Some code changed, but includes dependencies  

## (WIP) How to build for PC xubuntu 20.04  
* sudo apt install libsdl2-dev cmake
* make clean && make -j8  
* (or use cmake) cd build; make -f ppsspp_v2.mk clean; cmake ..; make -j8  

## (WIP) How to build for Trimui Smart Pro
* Modify Makefile for cross compile toolchain  
* make MIYOO=2 clean && make MIYOO=2 -j8
* (or use cmake) sudo apt install cmake
* (or use cmake) cd build; make -f ppsspp_v2.mk clean; make -f ppsspp_v2.mk; make -j8  
* (or use cmake) cd build; make -f ppsspp_v2.mk clean; . ./ppsspp_v2.sh; make -j8  

## Bugs  
* Too slow
* imouto.iso (实妹相伴的大泉君psp移植版) not work (black screen and loop, not exited)  
* Toheart2 psp crash  

## TODO  
* Change to -O3 -g0
* Try to port to Raspberry Pi Zero 2 W and Waveshare GPM280Z2  
