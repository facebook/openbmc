#
#	Module:			makefile
#
#					Copyright (C) Altera Corporation 1998-2001
#
#	Description:	Makefile for JAM Bytecode Player
#

OBJS = \
	jbistub.obj \
	jbimain.obj \
	jbicomp.obj \
	jbijtag.obj


!IF "$(MEM_TRACKER)" != ""
!IF "$(STATIC_MEMORY_SIZE)" != ""
# MEMORY TRACKER ON, USE 'STATIC_MEMORY_SIZE' KB OF STATIC MEMORY
.c.obj::
	cl /W4 /c /O2 /ML /DWINNT /DMEM_TRACKER /DUSE_STATIC_MEMORY=$(STATIC_MEMORY_SIZE) $< 
!ELSE
# MEMORY TRACKER ON, USE DYNAMIC MEMORY
.c.obj::
	cl /W4 /c /O2 /ML /DWINNT /DMEM_TRACKER $< 
!ENDIF
!ELSE
!IF "$(STATIC_MEMORY_SIZE)" != ""
# MEMORY TRACKER OFF, USE 'STATIC_MEMORY_SIZE' KB OF STATIC MEMORY
.c.obj::
	cl /W4 /c /O2 /ML /DWINNT /DUSE_STATIC_MEMORY=$(STATIC_MEMORY_SIZE) $< 
!ELSE
# MEMORY TRACKER OFF, USE DYNAMIC MEMORY
.c.obj::
	cl /W4 /c /O2 /ML /DWINNT $< 
!ENDIF
!ENDIF

jbi.exe : $(OBJS)
	link $(OBJS) advapi32.lib /out:jbi.exe

# Dependencies:

jbistub.obj : \
	jbistub.c \
	jbiport.h \
	jbiexprt.h

jbimain.obj : \
	jbimain.c \
	jbiport.h \
	jbiexprt.h \
	jbijtag.h \
	jbicomp.h

jamcomp.obj : \
	jamcomp.c \
	jbiport.h \
	jbiexprt.h \
	jbicomp.h

jbijtag.obj : \
	jbijtag.c \
	jbiport.h \
	jbiexprt.h \
	jbijtag.h
