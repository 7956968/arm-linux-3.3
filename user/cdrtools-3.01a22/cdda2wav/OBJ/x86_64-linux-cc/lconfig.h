/* lconfig.h.  Generated automatically by configure.  */
#if 0
/* @(#)lconfig.h.in	1.9 09/08/08 Copyright 1998-2003 Heiko Eissfeldt, Copyright 2006-2009 J. Schilling */
#endif
/* #undef HAVE_SYS_CDIO_H */		/* if we should use sys/cdio.h */

/* #undef HAVE_SYS_CDRIO_H */		/* if we should use sys/cdrio.h */

/* #undef HAVE_SUNDEV_SRREG_H */	/* if we should use sundev/srreg.h */

/* #undef HAVE_SYS_AUDIOIO_H */	/* if we should use sys/audioio.h */

/* #undef HAVE_SUN_AUDIOIO_H */	/* if we should use sun/audioio.h */

/* #undef HAVE_SOUNDCARD_H */		/* if we should use soundcard.h */

#define HAVE_SYS_SOUNDCARD_H 1	/* if we should use sys/soundcard.h */

#define HAVE_LINUX_SOUNDCARD_H 1	/* if we should use linux/soundcard.h */

/* #undef HAVE_MACHINE_SOUNDCARD_H */	/* if we should use machine/soundcard.h */

/* #undef HAVE_SYS_ASOUNDLIB_H */	/* if we should use sys/asoundlib.h */

/* #undef HAVE_MMSYSTEM_H */		/* if we should use mmsystem.h */

#if	defined HAVE_SOUNDCARD_H || defined HAVE_SYS_SOUNDCARD_H  || defined HAVE_LINUX_SOUNDCARD_H || defined HAVE_MACHINE_SOUNDCARD_H
#define HAVE_OSS	1
#endif

#if	defined HAVE_WINDOWS_H && defined HAVE_MMSYSTEM_H
#define HAVE_WINSOUND	1
#endif

#if	defined HAVE_OS2_H && defined HAVE_OS2ME_H
#define HAVE_OS2SOUND	1
#endif
