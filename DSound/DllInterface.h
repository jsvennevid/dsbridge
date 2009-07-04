#ifndef dsbridge_DllInterface_h
#define dsbridge_DllInterface_h

/*

Copyright 2009 Jesper Svennevid

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <windows.h>
#include <audiodefs.h>

typedef struct IUnknown *LPUNKNOWN;

typedef struct IDirectSound *LPDIRECTSOUND;
typedef struct IDirectSound8 *LPDIRECTSOUND8;
typedef struct IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;
typedef struct IDirectSoundCapture *LPDIRECTSOUNDCAPTURE;
typedef struct IDirectSoundCapture8 *LPDIRECTSOUNDCAPTURE8;
typedef struct IDirectSoundFullDuplex *LPDIRECTSOUNDFULLDUPLEX;
typedef struct IDirectSoundCaptureBuffer8 *LPDIRECTSOUNDCAPTUREBUFFER8;
typedef struct IDirectSoundBuffer8 *LPDIRECTSOUNDBUFFER8;

typedef struct _DSCBUFFERDESC *LPDSCBUFFERDESC;
typedef const struct _DSCBUFFERDESC *LPCDSCBUFFERDESC;

typedef struct _DSBUFFERDESC *LPDSBUFFERDESC;
typedef const struct _DSBUFFERDESC *LPCDSBUFFERDESC;

typedef BOOL (CALLBACK *LPDSENUMCALLBACKA)(LPGUID, LPCSTR, LPCSTR, LPVOID);
typedef BOOL (CALLBACK *LPDSENUMCALLBACKW)(LPGUID, LPCWSTR, LPCWSTR, LPVOID);

#endif
