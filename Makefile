
# Place in AUDIOSTREAM_DEFINES the relevant options for the audio file
# I/O libraries you have.
#
# Available options are
#
#  -DHAVE_LIBSNDFILE   * Read various formats (wav, aiff, ogg etc)
#                      * Write wav files
#  -DHAVE_OGGZ -DHAVE_FISHSOUND
#                      * Read Ogg/Vorbis files using oggz
#  -DHAVE_OPUS         * Read Opus files using libopus
#  -DHAVE_MINIMP3      * Read mp3 files using minimp3
#  -DHAVE_MEDIAFOUNDATION
#                      * Read various formats using MediaFoundation on Windows
#  -DHAVE_COREAUDIO    * Read various formats using CoreAudio on macOS/iOS
#
# If HAVE_LIBSNDFILE is not defined, a simple built-in Wav file writer
# will also be provided, as none of the other libraries have write
# support included here.

AUDIOSTREAM_DEFINES := -DHAVE_LIBSNDFILE -DHAVE_OGGZ -DHAVE_FISHSOUND -DHAVE_OPUS


# Add any related includes and libraries here
#
THIRD_PARTY_INCLUDES	:= -I/usr/include/opus
THIRD_PARTY_LIBS	:=


# If you are including a set of bq libraries into a project, you can
# override variables for all of them (including all of the above) in
# the following file, which all bq* Makefiles will include if found

-include ../Makefile.inc-bq


# This project-local Makefile describes the source files and contains
# no routinely user-modifiable parts

include build/Makefile.inc
