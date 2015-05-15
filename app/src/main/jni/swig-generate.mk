#
# SWIG extension for Android build system
#	

ifndef SWIG_PACKAGE
	$(error SWIG_PACKAGE is not defined.)
endif

SWIG_OUTDIR := $(LOCAL_PATH)/../java/$(subst .,/,$(SWIG_PACKAGE))

ifndef SWIG_LANG
	SWIG_MODE := -c++
endif

ifeq ($(SWIG_LANG),cxx)
	SWIG_MODE := -c++
else
	SWIG_MODE := c
endif

LOCAL_SRC_FILES += $(foreach INTERFACE,\
	$(SWIG_INTERFACES),\
	$(basename $(INTERFACE))_wrap.$(SWIG_LANG))

LOCAL_CPP_EXTENSION +=.cxx

%_wrap.$(SWIG_LANG) : %.i
	$(call host-mkdir,$(SWIG_OUTDIR))
	/Android/swigwin-3.0.5/swig -java \
	$(SWIG_MODE) \
	-package $(SWIG_PACKAGE) \
	-outdir $(SWIG_OUTDIR) \
	$<
