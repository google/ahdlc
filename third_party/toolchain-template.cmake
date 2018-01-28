#This one is important
SET(CMAKE_SYSTEM_NAME Linux)
#this one not so much
SET(CMAKE_SYSTEM_VERSION 1)

# specify the cross compiler
SET(CMAKE_C_COMPILER   /home/cmorar/toolchains/marvell-gcc_4.6/bin/arm-marvell-linux-uclibcgnueabi-gcc)
SET(CMAKE_CXX_COMPILER /home/cmorar/toolchains/marvell-gcc_4.6/bin/arm-marvell-linux-uclibcgnueabi-g++)
SET(CMAKE_STRIP /home/cmorar/toolchains/marvell-gcc_4.6/bin/arm-marvell-linux-uclibcgnueabi-strip)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /home/cmorar/toolchains/marvell-gcc_4.6/)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
