#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-MacOSX
CND_DLIB_EXT=dylib
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/event.o \
	${OBJECTDIR}/event_scheduler.o \
	${OBJECTDIR}/genran/extreal.o \
	${OBJECTDIR}/genran/format.o \
	${OBJECTDIR}/genran/geturng.o \
	${OBJECTDIR}/genran/myexcept.o \
	${OBJECTDIR}/genran/newran1.o \
	${OBJECTDIR}/genran/newran2.o \
	${OBJECTDIR}/genran/nr_ex.o \
	${OBJECTDIR}/genran/simpstr.o \
	${OBJECTDIR}/genran/str.o \
	${OBJECTDIR}/genran/test_lg.o \
	${OBJECTDIR}/genran/test_out.o \
	${OBJECTDIR}/genran/tryrand.o \
	${OBJECTDIR}/genran/tryrand1.o \
	${OBJECTDIR}/genran/tryrand2.o \
	${OBJECTDIR}/genran/tryrand3.o \
	${OBJECTDIR}/genran/tryrand4.o \
	${OBJECTDIR}/genran/tryrand5.o \
	${OBJECTDIR}/genran/tryrand6.o \
	${OBJECTDIR}/genran/tryurng.o \
	${OBJECTDIR}/genran/tryurng1.o \
	${OBJECTDIR}/genran/utility.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/network.o \
	${OBJECTDIR}/overnet.o \
	${OBJECTDIR}/util.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/eventsim

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/eventsim: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/eventsim ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/event.o: event.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/event.o event.cpp

${OBJECTDIR}/event_scheduler.o: event_scheduler.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/event_scheduler.o event_scheduler.cpp

${OBJECTDIR}/genran/extreal.o: genran/extreal.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/extreal.o genran/extreal.cpp

${OBJECTDIR}/genran/format.o: genran/format.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/format.o genran/format.cpp

${OBJECTDIR}/genran/geturng.o: genran/geturng.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/geturng.o genran/geturng.cpp

${OBJECTDIR}/genran/myexcept.o: genran/myexcept.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/myexcept.o genran/myexcept.cpp

${OBJECTDIR}/genran/newran1.o: genran/newran1.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/newran1.o genran/newran1.cpp

${OBJECTDIR}/genran/newran2.o: genran/newran2.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/newran2.o genran/newran2.cpp

${OBJECTDIR}/genran/nr_ex.o: genran/nr_ex.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/nr_ex.o genran/nr_ex.cpp

${OBJECTDIR}/genran/simpstr.o: genran/simpstr.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/simpstr.o genran/simpstr.cpp

${OBJECTDIR}/genran/str.o: genran/str.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/str.o genran/str.cpp

${OBJECTDIR}/genran/test_lg.o: genran/test_lg.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/test_lg.o genran/test_lg.cpp

${OBJECTDIR}/genran/test_out.o: genran/test_out.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/test_out.o genran/test_out.cpp

${OBJECTDIR}/genran/tryrand.o: genran/tryrand.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand.o genran/tryrand.cpp

${OBJECTDIR}/genran/tryrand1.o: genran/tryrand1.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand1.o genran/tryrand1.cpp

${OBJECTDIR}/genran/tryrand2.o: genran/tryrand2.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand2.o genran/tryrand2.cpp

${OBJECTDIR}/genran/tryrand3.o: genran/tryrand3.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand3.o genran/tryrand3.cpp

${OBJECTDIR}/genran/tryrand4.o: genran/tryrand4.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand4.o genran/tryrand4.cpp

${OBJECTDIR}/genran/tryrand5.o: genran/tryrand5.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand5.o genran/tryrand5.cpp

${OBJECTDIR}/genran/tryrand6.o: genran/tryrand6.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryrand6.o genran/tryrand6.cpp

${OBJECTDIR}/genran/tryurng.o: genran/tryurng.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryurng.o genran/tryurng.cpp

${OBJECTDIR}/genran/tryurng1.o: genran/tryurng1.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/tryurng1.o genran/tryurng1.cpp

${OBJECTDIR}/genran/utility.o: genran/utility.cpp 
	${MKDIR} -p ${OBJECTDIR}/genran
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/genran/utility.o genran/utility.cpp

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/network.o: network.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/network.o network.cpp

${OBJECTDIR}/overnet.o: overnet.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/overnet.o overnet.cpp

${OBJECTDIR}/util.o: util.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/util.o util.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/eventsim

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
