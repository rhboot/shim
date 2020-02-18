#
# version.mk
# Peter Jones, 2019-09-04 16:35
#

VERSION		?= 15
ifneq ($(origin RELEASE),undefined)
DASHRELEASE	?= -$(RELEASE)
else
DASHRELEASE	?=
endif

export VERSION DASHRELEASE

# vim:ft=make
#
