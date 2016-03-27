/*
 * Output functions
 *
 * Copyright (c) 2006-2010, Joachim Metz <forensics@hoffmannbv.nl>,
 * Hoffmann Investigations.
 *
 * Refer to AUTHORS for acknowledgements.
 *
 * This software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this software.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <memory.h>
#include <types.h>

#include <liberror.h>

#if defined( HAVE_STDLIB_H ) || defined( WINAPI )
#include <stdlib.h>
#endif

/* If libtool DLL support is enabled set LIBSMIO_DLL_IMPORT
 * before including libsmio.h
 */
#if defined( _WIN32 ) && defined( DLL_EXPORT )
#define LIBSMIO_DLL_IMPORT
#endif

#include <libsmio.h>

#if defined( HAVE_LOCAL_LIBUNA )
#include <libuna_definitions.h>
#elif defined( HAVE_LIBUNA_H )
#include <libuna.h>
#endif

#if defined( HAVE_LOCAL_LIBBFIO )
#include <libbfio_definitions.h>
#elif defined( HAVE_LIBBFIO )
#include <libbfio.h>
#endif

#if defined( HAVE_ZLIB_H )
#include <zlib.h>
#endif

#if defined( HAVE_OPENSSL_OPENSSLV_H )
#include <openssl/opensslv.h>
#endif

#if defined( HAVE_UUID_UUID_H )
#include <uuid/uuid.h>
#endif

#if defined( HAVE_AFFLIB_H )
#include <afflib.h>
#endif

#if defined( HAVE_LOCAL_LIBEWF )
#include <libewf_definitions.h>
#elif defined( HAVE_LIBEWF_H )
#include <libewf.h>
#endif

#include <libsystem.h>

#include "smiooutput.h"

/* Print the version information to a stream
 */
void smiooutput_version_fprint(
      FILE *stream,
      const libsystem_character_t *program )
{
	static char *function = "smiooutput_version_fprint";

	if( stream == NULL )
	{
		libsystem_notify_printf(
		 "%s: invalid stream.\n",
		 function );

		return;
	}
	if( program == NULL )
	{
		libsystem_notify_printf(
		 "%s: invalid program name.\n",
		 function );

		return;
	}
	fprintf(
	 stream,
	 "%" PRIs_LIBSYSTEM " %s (libsmio %s",
	 program,
	 LIBSMIO_VERSION_STRING,
	 LIBSMIO_VERSION_STRING );

	fprintf(
	 stream,
	 ", libuna %s",
	 LIBUNA_VERSION_STRING );

	fprintf(
	 stream,
	 ", libbfio %s",
	 LIBBFIO_VERSION_STRING );

#if defined( HAVE_LIBZ )
	fprintf(
	 stream,
	 ", zlib %s",
	 ZLIB_VERSION );
#endif

#if defined( HAVE_LIBCRYPTO )
	fprintf(
	 stream,
	 ", libcrypto %s",
	 SHLIB_VERSION_NUMBER );
#endif

#if defined( HAVE_LIBUUID )
	fprintf(
	 stream,
	 ", libuuid" );
#endif

#if defined( HAVE_AFFLIB )
	fprintf(
	 stream,
	 ", afflib" );

	/* TODO print version */
#endif

	fprintf(
	 stream,
	 ", libewf %s",
	 LIBEWF_VERSION_STRING );

	fprintf(
	 stream,
	 ")\n\n" );
}

/* Prints the executable version information
 */
void smiooutput_copyright_fprint(
      FILE *stream )
{
	static char *function = "smiooutput_copyright_fprint";

	if( stream == NULL )
	{
		libsystem_notify_printf(
		 "%s: invalid stream.\n",
		 function );

		return;
	}
	fprintf(
	 stream,
	 "Copyright (c) 2006-2010, Joachim Metz, Hoffmann Investigations <%s> and contributors.\n"
	 "This is free software; see the source for copying conditions. There is NO\n"
	 "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n",
	 PACKAGE_BUGREPORT );
}

