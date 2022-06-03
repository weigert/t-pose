######################################
#    tpose header files installer    #
######################################

# Install Location
INCLUDEPATH = /usr/local/include

# List of File-Names to Install
FILES_INCLUDE = poisson
FILES = tpose utility triangulation multiview io 

install:
			@echo "Copying t-pose Header Files ...";
			@if [ ! -d "/usr/local/include/tpose" ]; then mkdir $(INCLUDEPATH)/tpose; fi;

			@$(foreach var,$(FILES), cp source/$(var).hpp $(INCLUDEPATH)/tpose/$(var);)

#Include Files
			@echo "Copying Include Headers...";
			@if [ ! -d "/usr/local/include/tpose/include" ]; then mkdir $(INCLUDEPATH)/tpose/include; fi;
			@$(foreach var,$(FILES_INCLUDE), cp source/include/$(var).hpp $(INCLUDEPATH)/tpose/include/$(var);)

			@echo "Done";

all: install
