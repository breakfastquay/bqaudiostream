
/* Copyright Chris Cannam - All Rights Reserved */
#ifndef _TURBOT_FAKE_DXTRANS_H_
#define _TURBOT_FAKE_DXTRANS_H_

// Work around an omission in the Windows platform SDK -- the
// SampleGrabber DirectShow interface requires including a header
// called qedit.h, which fails to compile if it cannot find another
// header called dxtrans.h that is not included in the Windows SDK
// and is apparently no longer in the DirectX SDK either, possibly
// by accident.  (Present in Aug 2007 release, absent in Nov 2007,
// according to hearsay.)

// This faked-up dxtrans.h might possibly work around the problem.

#ifdef __MSVC__

#pragma message("Please ignore any message about installing the DirectX 9 SDK that may follow this line...")

#define __IDxtCompositor_INTERFACE_DEFINED__
#define __IDxtAlphaSetter_INTERFACE_DEFINED__
#define __IDxtJpeg_INTERFACE_DEFINED__
#define __IDxtKey_INTERFACE_DEFINED__

#endif

#endif
