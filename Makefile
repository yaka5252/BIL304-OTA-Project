CONTIKI_PROJECT = udp-client udp-server
# Dosya sistemi (Coffee File System) modülünü projeye dahil et
MODULES += os/storage/cfs
PROJECT_SOURCEFILES += ota-metadata.c
all: $(CONTIKI_PROJECT)

CONTIKI=../..
include $(CONTIKI)/Makefile.include
