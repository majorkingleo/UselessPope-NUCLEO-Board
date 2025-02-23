bin_PROGRAMS=\
	sim-NUCLEO-H753ZI-FlashTest
	
AM_CPPFLAGS = \
	-I$(top_srcdir)/../cpputils/cpputils/cpputilsshared  \
	-I$(top_srcdir)/../cpputils/cpputils/cpputilsshared/cpputilsformat \
	-I$(top_srcdir)/../cpputils/cpputils/io \
	-I$(top_srcdir)/../bslib/inc \
	-I$(top_srcdir)/../bslib/GPIO/inc \
	-I$(top_srcdir)/../bslib/HWCoding/inc \
	-I$(top_srcdir)/../bslib/LED/inc \
	-I$(top_srcdir)/../bslib/Publisher/inc \
	-I$(top_srcdir)/../bslib/StringSink/inc \
	-I$(top_srcdir)/../bslib/VersionNumber/inc \
	-I$(top_srcdir)/../bslib/PowerObserver/inc \
	-I$(top_srcdir)/../bslib/Container/inc \
	-I$(top_srcdir)/../bslib/Utility_Interfaces/inc \
	-I$(top_srcdir)/../bslib/JukeBox/inc \
	-I$(top_srcdir)/../bslib/Buzzer_Interface/inc \
	-I$(top_srcdir)/../wlib/inc \
	-I$(top_srcdir)/../wlib/BLOB/inc \
	-I$(top_srcdir)/../wlib/CRC/inc \
	-I$(top_srcdir)/../wlib/Callback/inc \
	-I$(top_srcdir)/../wlib/Container/inc \
	-I$(top_srcdir)/../wlib/Publisher/inc \
	-I$(top_srcdir)/../wlib/HASH/inc \
	-I$(top_srcdir)/os/inc \
	-I$(top_srcdir)/../ex-math/inc \
	-I$(top_srcdir)/../ex-math/statistics/inc \
	-I$(top_srcdir)/../ex-math/intervals/inc \
	-I$(top_srcdir)/../NUCLEO-H753ZI-FlashTest/bsp/inc \
	-I$(top_srcdir)/../simpleflashfs/simpleflashfs/src_2face \
	-I$(top_srcdir)/../simpleflashfs/simpleflashfs/src \
	-I$(top_srcdir)/../simpleflashfs/simpleflashfs/src/sim_pc \
	-I$(top_srcdir)/../SerialCommandParser/inc \
	-std=gnu++23 \
	-fstack-protector \
	-D_GLIBCXX_USE_CXX11_ABI=1 \
	-DSIMULATOR \
	-D_XOPEN_SOURCE=700 # for cyggwin fileno()

noinst_LIBRARIES=libcpputilsio.a \
	libcpputilsshared.a \
	libcpputilsformat.a \
	libwlib.a \
	libbslib.a \
	libsimpleflashfs.a \
	libserialcommandparser.a

libcpputilsio_a_SOURCES= \
	../cpputils/cpputils/io/ColoredOutput.h \
	../cpputils/cpputils/io/ColoredOutput.cc \
	../cpputils/cpputils/io/CpputilsDebug.cc \
	../cpputils/cpputils/io/CpputilsDebug.h \
	../cpputils/cpputils/io/DetectLocale.h \
	../cpputils/cpputils/io/DetectLocale.cc \
	../cpputils/cpputils/io/OutDebug.h \
	../cpputils/cpputils/io/OutDebug.cc \
	../cpputils/cpputils/io/read_file.h \
	../cpputils/cpputils/io/read_file.cc


libcpputilsshared_a_SOURCES= \
		../cpputils/cpputils/cpputilsshared/arg.h \
		../cpputils/cpputils/cpputilsshared/arg.cc \
		../cpputils/cpputils/cpputilsshared/backtrace.h \
		../cpputils/cpputils/cpputilsshared/backtrace.cc \
		../cpputils/cpputils/cpputilsshared/cppdir.h \
		../cpputils/cpputils/cpputilsshared/cppdir.cc \
		../cpputils/cpputils/cpputilsshared/file_option.h \
		../cpputils/cpputils/cpputilsshared/file_option.cc \
		../cpputils/cpputils/cpputilsshared/iterator.h \
		../cpputils/cpputils/cpputilsshared/leoini.h \
		../cpputils/cpputils/cpputilsshared/leoini.cc \
		../cpputils/cpputils/cpputilsshared/map_utils.h \
		../cpputils/cpputils/cpputilsshared/pairs.h \
		../cpputils/cpputils/cpputilsshared/range.h \
		../cpputils/cpputils/cpputilsshared/ref.h \
		../cpputils/cpputils/cpputilsshared/static_debug_exception.h \
		../cpputils/cpputils/cpputilsshared/static_debug_exception.cc \
		../cpputils/cpputils/cpputilsshared/stderr_exception.h \
		../cpputils/cpputils/cpputilsshared/stderr_exception.cc \
		../cpputils/cpputils/cpputilsshared/string_utils.h \
		../cpputils/cpputils/cpputilsshared/string_utils.cc \
		../cpputils/cpputils/cpputilsshared/utf8.h \
		../cpputils/cpputils/cpputilsshared/utf8/core.h \
		../cpputils/cpputils/cpputilsshared/utf8/checked.h \
		../cpputils/cpputils/cpputilsshared/utf8/unchecked.h \
		../cpputils/cpputils/cpputilsshared/utf8/cpp11.h \
		../cpputils/cpputils/cpputilsshared/utf8/cpp17.h \
		../cpputils/cpputils/cpputilsshared/utf8_util.h \
		../cpputils/cpputils/cpputilsshared/utf8_util.cc \
		../cpputils/cpputils/cpputilsshared/xml.h \
		../cpputils/cpputils/cpputilsshared/xml.cc \
		../cpputils/cpputils/cpputilsshared/CyclicArray.h \
		../cpputils/cpputils/cpputilsshared/static_vector.h \
		../cpputils/cpputils/cpputilsshared/static_list.h \
		../cpputils/cpputils/cpputilsshared/static_string.h \
		../cpputils/cpputils/cpputilsshared/string_adapter.h

libcpputilsformat_a_SOURCES= \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/format.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/format1.cc \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/format2.cc \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/format2.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/format1.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/cformat.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/cformat.cc \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/wformat2.cc \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/wformat2.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/cwformat.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/cwformat.cc \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/static_format.h \
	../cpputils/cpputils/cpputilsshared/cpputilsformat/static_format.cc

	
libwlib_a_SOURCES=\
	../wlib/Publisher/src/wlib-Publisher.cpp

libbslib_a_SOURCES=\
	../bslib/StringSink/src/bslib-StringSink.cpp
	
libsimpleflashfs_a_SOURCES=\
	../simpleflashfs/simpleflashfs/src_2face/H7TwoFace.cc \
	../simpleflashfs/simpleflashfs/src_2face/H7TwoFace.h \
	../simpleflashfs/simpleflashfs/src_2face/H7TwoFaceConfig.h \
	../simpleflashfs/simpleflashfs/src_2face/SimpleFlashFsFileBuffer.cc \
	../simpleflashfs/simpleflashfs/src_2face/SimpleFlashFsFileBuffer.h \
	../simpleflashfs/simpleflashfs/src_2face/SimpleFlashFsNoDel.h \
	../simpleflashfs/simpleflashfs/src_2face/SimpleIni.h \
	../simpleflashfs/simpleflashfs/src_2face/SimpleIni.cc \
	../simpleflashfs/simpleflashfs/src/crc/crc.c \
	../simpleflashfs/simpleflashfs/src/sim_pc/SimFlashMemoryPc.cc \
	../simpleflashfs/simpleflashfs/src/sim_pc/SimFlashMemoryPc.h \
	../simpleflashfs/simpleflashfs/src/sim_pc/SimSTM32InternalFlashPc.cc \
	../simpleflashfs/simpleflashfs/src/sim_pc/SimSTM32InternalFlashPc.h \
	../simpleflashfs/simpleflashfs/src/SimpleFlashFsConstants.cc \
	../simpleflashfs/simpleflashfs/src/SimpleFlashFsConstants.h
	
libserialcommandparser_a_SOURCES= \
	../SerialCommandParser/src/serial_command_parser.cpp
	
sim_NUCLEO_H753ZI_FlashTest_SOURCES= \
	../NUCLEO-H753ZI-FlashTest/app/src/main.cpp \
	../NUCLEO-H753ZI-FlashTest/app/src/CppUtilsUartDebug.cpp \
	../NUCLEO-H753ZI-FlashTest/app/src/LockedDebugStream.cpp \
	../NUCLEO-H753ZI-FlashTest/app/src/task_status_led.cpp \
	bsp/src/sim_bsp_uart_usb.cpp \
	bsp/src/sim_bsp_led.cpp \
	bsp/src/sim_analog_value_publisher.cpp \
	bsp/src/sim_internal_fs.cpp \
	os/src/sim_os.cpp
		
sim_NUCLEO_H753ZI_FlashTest_LDADD = libcpputilsformat.a \
	libbslib.a \	
	libsimpleflashfs.a \
	libserialcommandparser.a \
	libwlib.a \
	libcpputilsio.a \
	libcpputilsshared.a
				 
LIBS=
    
AM_LDFLAGS=
    
if MINGW
#AM_LDFLAGS += -mwindows
AM_LDFLAGS +=-Wl,--stack,1048576
LIBS += -liconv -lws2_32
# with -std=c++11 WIN32 is disabled (_WIN32) would be the valid one, so we define it in mingw.
AM_CPPFLAGS += -DWIN32 -DUNICODE \
	-I$(top_srcdir)/libco
	
sim_NUCLEO_H753ZI_FlashTest_SOURCES+= \
	libco/libco.c
	
else
#AM_LDFLAGS += -lX11
endif

if CYGWIN
LIBS += -liconv 
endif

    

