
bqaudiostream
=============

A small library wrapping various audio file read/write implementations
in C++.

Covers CoreAudio (on Mac), MediaFoundation (on Windows), libsndfile,
Ogg/Vorbis, Opus, and MiniMP3. Also includes a small WAV file
reader/writer so that there is always some implementation
available. Suitable for Windows, Mac, and Linux.

C++ standard required: C++98 (does not use C++11)

 * Depends on: [bqvec](https://hg.sr.ht/~breakfastquay/bqvec) [bqresample](https://hg.sr.ht/~breakfastquay/bqresample) [bqthingfactory](https://hg.sr.ht/~breakfastquay/bqthingfactory)

 * See also: [bqfft](https://hg.sr.ht/~breakfastquay/bqfft) [bqaudioio](https://hg.sr.ht/~breakfastquay/bqaudioio)

Copyright 2007-2021 Particular Programs Ltd.  Under a permissive
BSD/MIT-style licence: see the file COPYING for details.

