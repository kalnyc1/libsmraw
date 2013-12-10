/*
 * Mounts a storage media (split) RAW image file
 *
 * Copyright (C) 2010-2013, Joachim Metz <joachim.metz@gmail.com>
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
#include <file_stream.h>
#include <memory.h>
#include <types.h>

#if defined( HAVE_ERRNO_H )
#include <errno.h>
#endif

#if defined( HAVE_UNISTD_H )
#include <unistd.h>
#endif

#if defined( HAVE_STDLIB_H ) || defined( WINAPI )
#include <stdlib.h>
#endif

#if !defined( WINAPI ) || defined( USE_CRT_FUNCTIONS )
#if defined( TIME_WITH_SYS_TIME )
#include <sys/time.h>
#include <time.h>
#elif defined( HAVE_SYS_TIME_H )
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#if defined( HAVE_LIBFUSE ) || defined( HAVE_LIBOSXFUSE )
#define FUSE_USE_VERSION	26

#if defined( HAVE_LIBFUSE )
#include <fuse.h>

#elif defined( HAVE_LIBOSXFUSE )
#include <osxfuse/fuse.h>
#endif

#elif defined( HAVE_LIBDOKAN )
#include <dokan.h>
#endif

#include "mount_handle.h"
#include "smrawoutput.h"
#include "smrawtools_libcerror.h"
#include "smrawtools_libclocale.h"
#include "smrawtools_libcnotify.h"
#include "smrawtools_libcstring.h"
#include "smrawtools_libcsystem.h"
#include "smrawtools_libsmraw.h"

mount_handle_t *smrawmount_mount_handle = NULL;
int smrawmount_abort                    = 0;

/* Prints the executable usage information
 */
void usage_fprint(
      FILE *stream )
{
	if( stream == NULL )
	{
		return;
	}
	fprintf( stream, "Use smrawmount to mount a storage media (split) RAW (image) file\n\n" );

	fprintf( stream, "Usage: smrawmount [ -X extended_options ] [ -hvV ] smraw_files\n"
	                 "                  mount_point\n\n" );

	fprintf( stream, "\tsmraw_files: the first or the entire set of SMRAW segment files\n\n" );
	fprintf( stream, "\tmount_point: the directory to serve as mount point\n\n" );

	fprintf( stream, "\t-h:          shows this help\n" );
	fprintf( stream, "\t-v:          verbose output to stderr\n"
	                 "\t             smrawmount will remain running in the foreground\n" );
	fprintf( stream, "\t-V:          print version\n" );
	fprintf( stream, "\t-X:          extended options to pass to sub system\n" );
}

/* Signal handler for smrawmount
 */
void smrawmount_signal_handler(
      libcsystem_signal_t signal LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function   = "smrawmount_signal_handler";

	LIBCSYSTEM_UNREFERENCED_PARAMETER( signal )

	smrawmount_abort = 1;

	if( smrawmount_mount_handle != NULL )
	{
		if( mount_handle_signal_abort(
		     smrawmount_mount_handle,
		     &error ) != 1 )
		{
			libcnotify_printf(
			 "%s: unable to signal mount handle to abort.\n",
			 function );

			libcnotify_print_error_backtrace(
			 error );
			libcerror_error_free(
			 &error );
		}
	}
	/* Force stdin to close otherwise any function reading it will remain blocked
	 */
	if( libcsystem_file_io_close(
	     0 ) != 0 )
	{
		libcnotify_printf(
		 "%s: unable to close stdin.\n",
		 function );
	}
}

#if defined( HAVE_LIBFUSE ) || defined( HAVE_LIBOSXFUSE )

#if ( SIZEOF_OFF_T != 8 ) && ( SIZEOF_OFF_T != 4 )
#error Size of off_t not supported
#endif

static char *smrawmount_fuse_path_prefix         = "/raw";
static size_t smrawmount_fuse_path_prefix_length = 4;

/* Opens a file or directory
 * Returns 0 if successful or a negative errno value otherwise
 */
int smrawmount_fuse_open(
     const char *path,
     struct fuse_file_info *file_info )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_fuse_open";
	size_t path_length       = 0;
	int result               = 0;

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	if( file_info == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file info.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	path_length = libcstring_narrow_string_length(
	               path );

	if( ( path_length <= smrawmount_fuse_path_prefix_length )
         || ( path_length > ( smrawmount_fuse_path_prefix_length + 3 ) )
	 || ( libcstring_narrow_string_compare(
	       path,
	       smrawmount_fuse_path_prefix,
	       smrawmount_fuse_path_prefix_length ) != 0 ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %s.",
		 function,
		 path );

		result = -ENOENT;

		goto on_error;
	}
	if( ( file_info->flags & 0x03 ) != O_RDONLY )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_UNSUPPORTED_VALUE,
		 "%s: write access currently not supported.",
		 function );

		result = -EACCES;

		goto on_error;
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Reads a buffer of data at the specified offset
 * Returns number of bytes read if successful or a negative errno value otherwise
 */
int smrawmount_fuse_read(
     const char *path,
     char *buffer,
     size_t size,
     off_t offset,
     struct fuse_file_info *file_info )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_fuse_read";
	size_t path_length       = 0;
	ssize_t read_count       = 0;
	int input_handle_index   = 0;
	int result               = 0;
	int string_index         = 0;

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	if( size > (size_t) INT_MAX )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid size value exceeds maximum.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	if( file_info == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file info.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	path_length = libcstring_narrow_string_length(
	               path );

	if( ( path_length <= smrawmount_fuse_path_prefix_length )
         || ( path_length > ( smrawmount_fuse_path_prefix_length + 3 ) )
	 || ( libcstring_narrow_string_compare(
	       path,
	       smrawmount_fuse_path_prefix,
	       smrawmount_fuse_path_prefix_length ) != 0 ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %s.",
		 function,
		 path );

		result = -ENOENT;

		goto on_error;
	}
	string_index = (int) smrawmount_fuse_path_prefix_length;

	input_handle_index = path[ string_index++ ] - '0';

	if( string_index < (int) path_length )
	{
		input_handle_index *= 10;
		input_handle_index += path[ string_index++ ] - '0';
	}
	if( string_index < (int) path_length )
	{
		input_handle_index *= 10;
		input_handle_index += path[ string_index++ ] - '0';
	}
	input_handle_index -= 1;

	if( input_handle_index != 0 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid input handle index value out of bounds.",
		 function );

		result = -ERANGE;

		goto on_error;
	}
	if( mount_handle_seek_offset(
	     smrawmount_mount_handle,
	     (off64_t) offset,
	     SEEK_SET,
	     &error ) == -1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek offset in mount handle.",
		 function );

		result = -EIO;

		goto on_error;
	}
	read_count = mount_handle_read_buffer(
	              smrawmount_mount_handle,
	              (uint8_t *) buffer,
	              size,
	              &error );

	if( read_count == -1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read from mount handle.",
		 function );

		result = -EIO;

		goto on_error;
	}
	return( (int) read_count );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Reads a directory
 * Returns 0 if successful or a negative errno value otherwise
 */
int smrawmount_fuse_readdir(
     const char *path,
     void *buffer,
     fuse_fill_dir_t filler,
     off_t offset LIBCSYSTEM_ATTRIBUTE_UNUSED,
     struct fuse_file_info *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	char smrawmount_fuse_path[ 10 ];

	libcerror_error_t *error    = NULL;
	static char *function       = "smrawmount_fuse_readdir";
	size_t path_length          = 0;
	int input_handle_index      = 0;
	int number_of_input_handles = 0;
	int result                  = 0;
	int string_index            = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( offset )
	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	path_length = libcstring_narrow_string_length(
	               path );

	if( ( path_length != 1 )
	 || ( path[ 0 ] != '/' ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %s.",
		 function,
		 path );

		result = -ENOENT;

		goto on_error;
	}
	if( libcstring_narrow_string_copy(
	     smrawmount_fuse_path,
	     smrawmount_fuse_path_prefix,
	     smrawmount_fuse_path_prefix_length ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy path prefix.",
		 function );

		result = -errno;

		goto on_error;
	}
	if( mount_handle_get_number_of_input_handles(
	     smrawmount_mount_handle,
	     &number_of_input_handles,
	     &error ) != 1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of input handles.",
		 function );

		result = -EIO;

		goto on_error;
	}
	if( ( number_of_input_handles < 0 )
	 || ( number_of_input_handles > 99 ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported number of input handles.",
		 function );

		result = -ENOENT;

		goto on_error;
	}
	if( filler(
	     buffer,
	     ".",
	     NULL,
	     0 ) == 1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set directory entry.",
		 function );

		result = -EIO;

		goto on_error;
	}
	if( filler(
	     buffer,
	     "..",
	     NULL,
	     0 ) == 1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set directory entry.",
		 function );

		result = -EIO;

		goto on_error;
	}
	for( input_handle_index = 1;
	     input_handle_index <= number_of_input_handles;
	     input_handle_index++ )
	{
		string_index = smrawmount_fuse_path_prefix_length;

		if( input_handle_index >= 100 )
		{
			smrawmount_fuse_path[ string_index++ ] = '0' + (char) ( input_handle_index / 100 );
		}
		if( input_handle_index >= 10 )
		{
			smrawmount_fuse_path[ string_index++ ] = '0' + (char) ( input_handle_index / 10 );
		}
		smrawmount_fuse_path[ string_index++ ] = '0' + (char) ( input_handle_index % 10 );
		smrawmount_fuse_path[ string_index++ ] = 0;

		if( filler(
		     buffer,
		     &( smrawmount_fuse_path[ 1 ] ),
		     NULL,
		     0 ) == 1 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to set directory entry.",
			 function );

			result = -EIO;

			goto on_error;
		}
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Retrieves the file stat info
 * Returns 0 if successful or a negative errno value otherwise
 */
int smrawmount_fuse_getattr(
     const char *path,
     struct stat *stat_info )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_fuse_getattr";
	size64_t media_size      = 0;
	size_t path_length       = 0;
	int input_handle_index   = 0;
	int result               = -ENOENT;
	int string_index         = 0;

#if defined( HAVE_TIME )
	time_t timestamp         = 0;
#endif

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	if( stat_info == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid stat info.",
		 function );

		result = -EINVAL;

		goto on_error;
	}
	if( memory_set(
	     stat_info,
	     0,
	     sizeof( struct stat ) ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear stat info.",
		 function );

		result = errno;

		goto on_error;
	}
	path_length = libcstring_narrow_string_length(
	               path );

	if( path_length == 1 )
	{
		if( path[ 0 ] == '/' )
		{
			stat_info->st_mode  = S_IFDIR | 0755;
			stat_info->st_nlink = 2;

			result = 0;
		}
	}
	else if( ( path_length > smrawmount_fuse_path_prefix_length )
	      && ( path_length <= ( smrawmount_fuse_path_prefix_length + 3 ) ) )
	{
		if( libcstring_narrow_string_compare(
		     path,
		     smrawmount_fuse_path_prefix,
		     smrawmount_fuse_path_prefix_length ) == 0 )
		{
			string_index = smrawmount_fuse_path_prefix_length;

			input_handle_index = path[ string_index++ ] - '0';

			if( string_index < (int) path_length )
			{
				input_handle_index *= 10;
				input_handle_index += path[ string_index++ ] - '0';
			}
			if( string_index < (int) path_length )
			{
				input_handle_index *= 10;
				input_handle_index += path[ string_index++ ] - '0';
			}
			input_handle_index -= 1;

			stat_info->st_mode  = S_IFREG | 0444;
			stat_info->st_nlink = 1;

			if( input_handle_index != 0 )
			{
				libcerror_error_set(
				 &error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid input handle index value out of bounds.",
				 function );

				result = -ERANGE;

				goto on_error;
			}
			if( mount_handle_get_media_size(
			     smrawmount_mount_handle,
			     &media_size,
			     &error ) != 1 )
			{
				libcerror_error_set(
				 &error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
				 "%s: unable to retrieve media size.",
				 function );

				result = -EIO;

				goto on_error;
			}
#if SIZEOF_OFF_T == 4
			if( media_size > (size64_t) UINT32_MAX )
			{
				libcerror_error_set(
				 &error,
				 LIBCERROR_ERROR_DOMAIN_RUNTIME,
				 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
				 "%s: invalid media size value out of bounds.",
				 function );

				result = -ERANGE;

				goto on_error;
			}
#endif
			stat_info->st_size = (off_t) media_size;

			result = 0;
		}
	}
	if( result == 0 )
	{
#if defined( HAVE_TIME )
		if( time( &timestamp ) == (time_t) -1 )
		{
			timestamp = 0;
		}
		stat_info->st_atime = timestamp;
		stat_info->st_mtime = timestamp;
		stat_info->st_ctime = timestamp;
#else
		stat_info->st_atime = 0;
		stat_info->st_mtime = 0;
		stat_info->st_ctime = 0;
#endif
#if defined( HAVE_GETEUID )
		stat_info->st_uid = geteuid();
#else
		stat_info->st_uid = 0;
#endif
#if defined( HAVE_GETEGID )
		stat_info->st_gid = getegid();
#else
		stat_info->st_gid = 0;
#endif
	}
	return( result );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Cleans up when fuse is done
 */
void smrawmount_fuse_destroy(
      void *private_data LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_fuse_destroy";

	LIBCSYSTEM_UNREFERENCED_PARAMETER( private_data )

	if( smrawmount_mount_handle != NULL )
	{
		if( mount_handle_free(
		     &smrawmount_mount_handle,
		     &error ) != 1 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_FINALIZE_FAILED,
			 "%s: unable to free mount handle.",
			 function );

			goto on_error;
		}
	}
	return;

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return;
}

#elif defined( HAVE_LIBDOKAN )

static wchar_t *smrawmount_dokan_path_prefix      = L"\\RAW";
static size_t smrawmount_dokan_path_prefix_length = 4;

/* Opens a file or directory
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_CreateFile(
               const wchar_t *path,
               DWORD desired_access,
               DWORD share_mode LIBCSYSTEM_ATTRIBUTE_UNUSED,
               DWORD creation_disposition,
               DWORD attribute_flags LIBCSYSTEM_ATTRIBUTE_UNUSED,
               DOKAN_FILE_INFO *file_info )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_CreateFile";
	size_t path_length       = 0;
	int result               = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( share_mode )
	LIBCSYSTEM_UNREFERENCED_PARAMETER( attribute_flags )

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( ( desired_access & GENERIC_WRITE ) != 0 )
	{
		return( -ERROR_WRITE_PROTECT );
	}
	/* Ignore the share_mode
	 */
	if( creation_disposition == CREATE_NEW )
	{
		return( -ERROR_FILE_EXISTS );
	}
	else if( creation_disposition == CREATE_ALWAYS )
	{
		return( -ERROR_ALREADY_EXISTS );
	}
	else if( creation_disposition == OPEN_ALWAYS )
	{
		return( -ERROR_FILE_NOT_FOUND );
	}
	else if( creation_disposition == TRUNCATE_EXISTING )
	{
		return( -ERROR_FILE_NOT_FOUND );
	}
	else if( creation_disposition != OPEN_EXISTING )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid creation disposition.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( file_info == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file info.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	path_length = libcstring_wide_string_length(
	               path );

	if( path_length == 1 )
	{
		if( path[ 0 ] != (wchar_t) '\\' )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported path: %ls.",
			 function,
			 path );

			result = -ERROR_FILE_NOT_FOUND;

			goto on_error;
		}
	}
	else
	{
		if( ( path_length <= smrawmount_dokan_path_prefix_length )
		 || ( path_length > ( smrawmount_dokan_path_prefix_length + 3 ) )
		 || ( libcstring_wide_string_compare(
		       path,
		       smrawmount_dokan_path_prefix,
		       smrawmount_dokan_path_prefix_length ) != 0 ) )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported path: %ls.",
			 function,
			 path );

			result = -ERROR_FILE_NOT_FOUND;

			goto on_error;
		}
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Opens a directory
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_OpenDirectory(
               const wchar_t *path,
               DOKAN_FILE_INFO *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_OpenDirectory";
	size_t path_length       = 0;
	int result               = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	path_length = libcstring_wide_string_length(
	               path );

	if( ( path_length != 1 )
	 || ( path[ 0 ] != (wchar_t) '\\' ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %ls.",
		 function,
		 path );

		result = -ERROR_FILE_NOT_FOUND;

		goto on_error;
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Closes a file or direcotry
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_CloseFile(
               const wchar_t *path,
               DOKAN_FILE_INFO *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_CloseFile";
	int result               = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Reads a buffer of data at the specified offset
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_ReadFile(
               const wchar_t *path,
               void *buffer,
               DWORD number_of_bytes_to_read,
               DWORD *number_of_bytes_read,
               LONGLONG offset,
               DOKAN_FILE_INFO *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_ReadFile";
	size_t path_length       = 0;
	ssize_t read_count       = 0;
	int input_handle_index   = 0;
	int result               = 0;
	int string_index         = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( number_of_bytes_to_read > (DWORD) INT32_MAX )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid number of bytes to read value exceeds maximum.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( number_of_bytes_read == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid number of bytes read.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	path_length = libcstring_wide_string_length(
	               path );

	if( ( path_length <= smrawmount_dokan_path_prefix_length )
         || ( path_length > ( smrawmount_dokan_path_prefix_length + 3 ) )
	 || ( libcstring_wide_string_compare(
	       path,
	       smrawmount_dokan_path_prefix,
	       smrawmount_dokan_path_prefix_length ) != 0 ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %ls.",
		 function,
		 path );

		result = -ERROR_FILE_NOT_FOUND;

		goto on_error;
	}
	string_index = (int) smrawmount_dokan_path_prefix_length;

	input_handle_index = path[ string_index++ ] - (wchar_t) '0';

	if( string_index < (int) path_length )
	{
		input_handle_index *= 10;
		input_handle_index += path[ string_index++ ] - (wchar_t) '0';
	}
	if( string_index < (int) path_length )
	{
		input_handle_index *= 10;
		input_handle_index += path[ string_index++ ] - (wchar_t) '0';
	}
	input_handle_index -= 1;

	if( input_handle_index != 0 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
		 "%s: invalid input handle index value out of bounds.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( mount_handle_seek_offset(
	     smrawmount_mount_handle,
	     (off64_t) offset,
	     SEEK_SET,
	     &error ) == -1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_SEEK_FAILED,
		 "%s: unable to seek offset in mount handle.",
		 function );

		result = -ERROR_SEEK_ON_DEVICE;

		goto on_error;
	}
	read_count = mount_handle_read_buffer(
		      smrawmount_mount_handle,
		      (uint8_t *) buffer,
		      (size_t) number_of_bytes_to_read,
		      &error );

	if( read_count == -1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_IO,
		 LIBCERROR_IO_ERROR_READ_FAILED,
		 "%s: unable to read from mount handle.",
		 function );

		result = -ERROR_READ_FAULT;

		goto on_error;
	}
	if( read_count > (size_t) INT32_MAX )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_VALUE_EXCEEDS_MAXIMUM,
		 "%s: invalid read count value exceeds maximum.",
		 function );

		result = -ERROR_READ_FAULT;

		goto on_error;
	}
	/* Dokan does not require the read function to return ERROR_HANDLE_EOF
	 */
	*number_of_bytes_read = (DWORD) read_count;

	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Reads a directory
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_FindFiles(
               const wchar_t *path,
               PFillFindData fill_find_data,
               DOKAN_FILE_INFO *file_info )
{
	WIN32_FIND_DATAW find_data;

	wchar_t smrawmount_dokan_path[ 10 ];

	libcerror_error_t *error    = NULL;
	static char *function       = "smrawmount_dokan_FindFiles";
	size64_t media_size         = 0;
	size_t path_length          = 0;
	int input_handle_index      = 0;
	int number_of_input_handles = 0;
	int result                  = 0;
	int string_index            = 0;

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	path_length = libcstring_wide_string_length(
	               path );

	if( ( path_length != 1 )
	 || ( path[ 0 ] != (wchar_t) '\\' ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported path: %ls.",
		 function,
		 path );

		result = -ERROR_FILE_NOT_FOUND;

		goto on_error;
	}
	if( libcstring_wide_string_copy(
	     smrawmount_dokan_path,
	     smrawmount_dokan_path_prefix,
	     smrawmount_dokan_path_prefix_length ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy path prefix.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( mount_handle_get_number_of_input_handles(
	     smrawmount_mount_handle,
	     &number_of_input_handles,
	     &error ) != 1 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
		 "%s: unable to retrieve number of input handles.",
		 function );

		result = -EIO;

		goto on_error;
	}
	if( ( number_of_input_handles < 0 )
	 || ( number_of_input_handles > 99 ) )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
		 "%s: unsupported number of input handles.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( memory_set(
	     &find_data,
	     0,
	     sizeof( WIN32_FIND_DATAW ) ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear find data.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( libcstring_wide_string_copy(
	     find_data.cFileName,
	     L".",
	     1 ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy filename.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( libcstring_wide_string_copy(
	     find_data.cAlternateFileName,
	     L".",
	     1 ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy alternate filename.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	find_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
/* TODO set timestamps
	find_data.ftCreationTime   = { 0, 0 };
	find_data.ftLastAccessTime = { 0, 0 };
	find_data.ftLastWriteTime  = { 0, 0 };
*/

	if( fill_find_data(
	     &find_data,
	     file_info ) != 0 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set directory entry.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( memory_set(
	     &find_data,
	     0,
	     sizeof( WIN32_FIND_DATAW ) ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_SET_FAILED,
		 "%s: unable to clear find data.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( libcstring_wide_string_copy(
	     find_data.cFileName,
	     L"..",
	     2 ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy filename.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	if( libcstring_wide_string_copy(
	     find_data.cAlternateFileName,
	     L"..",
	     2 ) == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_MEMORY,
		 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
		 "%s: unable to copy alternate filename.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	find_data.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
/* TODO set timestamps
	find_data.ftCreationTime   = { 0, 0 };
	find_data.ftLastAccessTime = { 0, 0 };
	find_data.ftLastWriteTime  = { 0, 0 };
*/

	if( fill_find_data(
	     &find_data,
	     file_info ) != 0 )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_RUNTIME,
		 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
		 "%s: unable to set directory entry.",
		 function );

		result = -ERROR_GEN_FAILURE;

		goto on_error;
	}
	for( input_handle_index = 1;
	     input_handle_index <= number_of_input_handles;
	     input_handle_index++ )
	{
		string_index = (int) smrawmount_dokan_path_prefix_length;

		if( input_handle_index >= 100 )
		{
			smrawmount_dokan_path[ string_index++ ] = (wchar_t) ( '0' + ( input_handle_index / 100 ) );
		}
		if( input_handle_index >= 10 )
		{
			smrawmount_dokan_path[ string_index++ ] = (wchar_t) ( '0' + ( input_handle_index / 10 ) );
		}
		smrawmount_dokan_path[ string_index++ ] = (wchar_t) ( '0' + ( input_handle_index % 10 ) );
		smrawmount_dokan_path[ string_index++ ] = 0;

		if( input_handle_index != 1 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid input handle index value out of bounds.",
			 function );

			result = -ERROR_BAD_ARGUMENTS;

			goto on_error;
		}
		if( mount_handle_get_media_size(
		     smrawmount_mount_handle,
		     &media_size,
		     &error ) != 1 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve media size.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
		if( memory_set(
		     &find_data,
		     0,
		     sizeof( WIN32_FIND_DATAW ) ) == NULL )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_SET_FAILED,
			 "%s: unable to clear find data.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
		if( libcstring_wide_string_copy(
		     find_data.cFileName,
		     &( smrawmount_dokan_path[ 1 ] ),
		     string_index - 2 ) == NULL )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
			 "%s: unable to copy filename.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
		if( libcstring_wide_string_copy(
		     find_data.cAlternateFileName,
		     &( smrawmount_dokan_path[ 1 ] ),
		     string_index - 2 ) == NULL )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
			 "%s: unable to copy alternate filename.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
		find_data.dwFileAttributes = FILE_ATTRIBUTE_READONLY;
/* TODO set timestamps
		find_data.ftCreationTime   = { 0, 0 };
		find_data.ftLastAccessTime = { 0, 0 };
		find_data.ftLastWriteTime  = { 0, 0 };
*/
		find_data.nFileSizeHigh    = (DWORD) ( media_size >> 32 );
		find_data.nFileSizeLow     = (DWORD) ( media_size & 0xffffffffUL );

		if( fill_find_data(
		     &find_data,
		     file_info ) != 0 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_SET_FAILED,
			 "%s: unable to set directory entry.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

int __stdcall smrawmount_dokan_GetFileInformation(
               const wchar_t *path,
               BY_HANDLE_FILE_INFORMATION *file_information,
               DOKAN_FILE_INFO *file_info )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_GetFileInformation";
	size64_t media_size      = 0;
	size_t path_length       = 0;
	int input_handle_index   = 0;
	int result               = 0;
	int string_index         = 0;

	if( path == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid path.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	if( file_info == NULL )
	{
		libcerror_error_set(
		 &error,
		 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
		 LIBCERROR_ARGUMENT_ERROR_INVALID_VALUE,
		 "%s: invalid file info.",
		 function );

		result = -ERROR_BAD_ARGUMENTS;

		goto on_error;
	}
	path_length = libcstring_wide_string_length(
	               path );

	if( path_length == 1 )
	{
		if( path[ 0 ] != (wchar_t) '\\' )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported path: %ls.",
			 function,
			 path );

			result = -ERROR_FILE_NOT_FOUND;

			goto on_error;
		}
		file_information->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
/* TODO set timestamps
		file_information->ftCreationTime   = { 0, 0 };
		file_information->ftLastAccessTime = { 0, 0 };
		file_information->ftLastWriteTime  = { 0, 0 };
*/
	}
	else
	{
		if( ( path_length <= smrawmount_dokan_path_prefix_length )
		 || ( path_length > ( smrawmount_dokan_path_prefix_length + 3 ) )
		 || ( libcstring_wide_string_compare(
		       path,
		       smrawmount_dokan_path_prefix,
		       smrawmount_dokan_path_prefix_length ) != 0 ) )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_ARGUMENTS,
			 LIBCERROR_ARGUMENT_ERROR_UNSUPPORTED_VALUE,
			 "%s: unsupported path: %ls.",
			 function,
			 path );

			result = -ERROR_FILE_NOT_FOUND;

			goto on_error;
		}
		string_index = (int) smrawmount_dokan_path_prefix_length;

		input_handle_index = path[ string_index++ ] - (wchar_t) '0';

		if( string_index < (int) path_length )
		{
			input_handle_index *= 10;
			input_handle_index += path[ string_index++ ] - (wchar_t) '0';
		}
		if( string_index < (int) path_length )
		{
			input_handle_index *= 10;
			input_handle_index += path[ string_index++ ] - (wchar_t) '0';
		}
		input_handle_index -= 1;

		if( input_handle_index != 0 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_VALUE_OUT_OF_BOUNDS,
			 "%s: invalid input handle index value out of bounds.",
			 function );

			result = -ERROR_BAD_ARGUMENTS;

			goto on_error;
		}
		if( mount_handle_get_media_size(
		     smrawmount_mount_handle,
		     &media_size,
		     &error ) != 1 )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_RUNTIME,
			 LIBCERROR_RUNTIME_ERROR_GET_FAILED,
			 "%s: unable to retrieve media size.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
		file_information->dwFileAttributes = FILE_ATTRIBUTE_READONLY;
/* TODO set timestamps
		file_information->ftCreationTime   = { 0, 0 };
		file_information->ftLastAccessTime = { 0, 0 };
		file_information->ftLastWriteTime  = { 0, 0 };
*/
		file_information->nFileSizeHigh    = (DWORD) ( media_size >> 32 );
		file_information->nFileSizeLow     = (DWORD) ( media_size & 0xffffffffUL );
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Retrieves the volume information
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_GetVolumeInformation(
               wchar_t *volume_name,
               DWORD volume_name_size,
               DWORD *volume_serial_number,
               DWORD *maximum_filename_length,
               DWORD *file_system_flags,
               wchar_t *file_system_name,
               DWORD file_system_name_size,
               DOKAN_FILE_INFO *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	libcerror_error_t *error = NULL;
	static char *function    = "smrawmount_dokan_GetVolumeInformation";
	int result               = 0;

	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	if( ( volume_name != NULL )
	 && ( volume_name_size > (DWORD) ( sizeof( wchar_t ) * 4 ) ) )
	{
		/* Using wcsncpy seems to cause strange behavior here
		 */
		if( memory_copy(
		     volume_name,
		     L"RAW",
		     sizeof( wchar_t ) * 4 ) == NULL )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
			 "%s: unable to copy volume name.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
	}
	if( volume_serial_number != NULL )
	{
		/* If this value contains 0 it can crash the system is this an issue in Dokan?
		 */
		*volume_serial_number = 0x19831116;
	}
	if( maximum_filename_length != NULL )
	{
		*maximum_filename_length = 256;
	}
	if( file_system_flags != NULL )
	{
		*file_system_flags = FILE_CASE_SENSITIVE_SEARCH
		                   | FILE_CASE_PRESERVED_NAMES
		                   | FILE_UNICODE_ON_DISK
		                   | FILE_READ_ONLY_VOLUME;
	}
	if( ( file_system_name != NULL )
	 && ( file_system_name_size > (DWORD) ( sizeof( wchar_t ) * 6 ) ) )
	{
		/* Using wcsncpy seems to cause strange behavior here
		 */
		if( memory_copy(
		     file_system_name,
		     L"Dokan",
		     sizeof( wchar_t ) * 6 ) == NULL )
		{
			libcerror_error_set(
			 &error,
			 LIBCERROR_ERROR_DOMAIN_MEMORY,
			 LIBCERROR_MEMORY_ERROR_COPY_FAILED,
			 "%s: unable to copy file system name.",
			 function );

			result = -ERROR_GEN_FAILURE;

			goto on_error;
		}
	}
	return( 0 );

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
	return( result );
}

/* Unmount the image
 * Returns 0 if successful or a negative error code otherwise
 */
int __stdcall smrawmount_dokan_Unmount(
               DOKAN_FILE_INFO *file_info LIBCSYSTEM_ATTRIBUTE_UNUSED )
{
	static char *function = "smrawmount_dokan_Unmount";

	LIBCSYSTEM_UNREFERENCED_PARAMETER( file_info )

	return( 0 );
}

#endif

/* The main program
 */
#if defined( LIBCSTRING_HAVE_WIDE_SYSTEM_CHARACTER )
int wmain( int argc, wchar_t * const argv[] )
#else
int main( int argc, char * const argv[] )
#endif
{
	libcstring_system_character_t * const *argv_filenames  = NULL;

	libsmraw_error_t *error                                = NULL;
	libcstring_system_character_t *mount_point             = NULL;
	libcstring_system_character_t *option_extended_options = NULL;
	libcstring_system_character_t *program                 = _LIBCSTRING_SYSTEM_STRING( "smrawmount" );
	libcstring_system_integer_t option                     = 0;
	int number_of_filenames                                = 0;
	int result                                             = 0;
	int verbose                                            = 0;

#if !defined( LIBCSYSTEM_HAVE_GLOB )
	libcsystem_glob_t *glob                                = NULL;
#endif

#if defined( HAVE_LIBFUSE ) || defined( HAVE_LIBOSXFUSE )
	struct fuse_operations smrawmount_fuse_operations;

	struct fuse_args smrawmount_fuse_arguments             = FUSE_ARGS_INIT(0, NULL);
	struct fuse_chan *smrawmount_fuse_channel              = NULL;
	struct fuse *smrawmount_fuse_handle                    = NULL;

#elif defined( HAVE_LIBDOKAN )
	DOKAN_OPERATIONS smrawmount_dokan_operations;
	DOKAN_OPTIONS smrawmount_dokan_options;
#endif

	libcnotify_stream_set(
	 stderr,
	 NULL );
	libcnotify_verbose_set(
	 1 );

	if( libclocale_initialize(
             "smrawtools",
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize locale values.\n" );

		goto on_error;
	}
        if( libcsystem_initialize(
             _IONBF,
             &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize system values.\n" );

		goto on_error;
	}
	smrawoutput_version_fprint(
	 stdout,
	 program );

	while( ( option = libcsystem_getopt(
	                   argc,
	                   argv,
	                   _LIBCSTRING_SYSTEM_STRING( "hvVX:" ) ) ) != (libcstring_system_integer_t) -1 )
	{
		switch( option )
		{
			case (libcstring_system_integer_t) '?':
			default:
				fprintf(
				 stderr,
				 "Invalid argument: %" PRIs_LIBCSTRING_SYSTEM "\n",
				 argv[ optind - 1 ] );

				usage_fprint(
				 stdout );

				return( EXIT_FAILURE );

			case (libcstring_system_integer_t) 'h':
				usage_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			case (libcstring_system_integer_t) 'v':
				verbose = 1;

				break;

			case (libcstring_system_integer_t) 'V':
				smrawoutput_copyright_fprint(
				 stdout );

				return( EXIT_SUCCESS );

			case (libcstring_system_integer_t) 'X':
				option_extended_options = optarg;

				break;
		}
	}
	if( optind == argc )
	{
		fprintf(
		 stderr,
		 "Missing SMRAW image file(s).\n" );


		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	if( ( optind + 1 ) == argc )
	{
		fprintf(
		 stderr,
		 "Missing mount point.\n" );

		usage_fprint(
		 stdout );

		return( EXIT_FAILURE );
	}
	mount_point = argv[ argc - 1 ];

	libcnotify_verbose_set(
	 verbose );
	libsmraw_notify_set_stream(
	 stderr,
	 NULL );
	libsmraw_notify_set_verbose(
	 verbose );

#if !defined( LIBCSYSTEM_HAVE_GLOB )
	if( libcsystem_glob_initialize(
	     &glob,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize glob.\n" );

		goto on_error;
	}
	if( libcsystem_glob_resolve(
	     glob,
	     &( argv[ optind ] ),
	     argc - optind - 1,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to resolve glob.\n" );

		goto on_error;
	}
	argv_filenames      = glob->result;
	number_of_filenames = glob->number_of_results;
#else
	argv_filenames      = &( argv[ optind ] );
	number_of_filenames = argc - optind - 1;
#endif

	if( mount_handle_initialize(
	     &smrawmount_mount_handle,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to initialize mount handle.\n" );

		goto on_error;
	}
	if( mount_handle_open_input(
	     smrawmount_mount_handle,
	     argv_filenames,
	     number_of_filenames,
	     &error ) != 1 )
	{
		fprintf(
		 stderr,
		 "Unable to open SMRAW file(s).\n" );

		goto on_error;
	}
#if defined( HAVE_LIBFUSE ) || defined( HAVE_LIBOSXFUSE )
	if( memory_set(
	     &smrawmount_fuse_operations,
	     0,
	     sizeof( struct fuse_operations ) ) == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to clear fuse operations.\n" );

		goto on_error;
	}
	if( option_extended_options != NULL )
	{
		/* This argument is required but ignored
		 */
		if( fuse_opt_add_arg(
		     &smrawmount_fuse_arguments,
		     "" ) != 0 )
		{
			fprintf(
			 stderr,
			 "Unable add fuse arguments.\n" );

			goto on_error;
		}
		if( fuse_opt_add_arg(
		     &smrawmount_fuse_arguments,
		     "-o" ) != 0 )
		{
			fprintf(
			 stderr,
			 "Unable add fuse arguments.\n" );

			goto on_error;
		}
		if( fuse_opt_add_arg(
		     &smrawmount_fuse_arguments,
		     option_extended_options ) != 0 )
		{
			fprintf(
			 stderr,
			 "Unable add fuse arguments.\n" );

			goto on_error;
		}
	}
	smrawmount_fuse_operations.open    = &smrawmount_fuse_open;
	smrawmount_fuse_operations.read    = &smrawmount_fuse_read;
	smrawmount_fuse_operations.readdir = &smrawmount_fuse_readdir;
	smrawmount_fuse_operations.getattr = &smrawmount_fuse_getattr;
	smrawmount_fuse_operations.destroy = &smrawmount_fuse_destroy;

	smrawmount_fuse_channel = fuse_mount(
	                           mount_point,
	                           &smrawmount_fuse_arguments );

	if( smrawmount_fuse_channel == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to create fuse channel.\n" );

		goto on_error;
	}
	smrawmount_fuse_handle = fuse_new(
	                          smrawmount_fuse_channel,
	                          &smrawmount_fuse_arguments,
	                          &smrawmount_fuse_operations,
	                          sizeof( struct fuse_operations ),
	                          smrawmount_mount_handle );
	
	if( smrawmount_fuse_handle == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to create fuse handle.\n" );

		goto on_error;
	}
	if( verbose == 0 )
	{
		if( fuse_daemonize(
		     0 ) != 0 )
		{
			fprintf(
			 stderr,
			 "Unable to daemonize fuse.\n" );

			goto on_error;
		}
	}
	result = fuse_loop(
	          smrawmount_fuse_handle );

	if( result != 0 )
	{
		fprintf(
		 stderr,
		 "Unable to run fuse loop.\n" );

		goto on_error;
	}
	fuse_destroy(
	 smrawmount_fuse_handle );

	fuse_opt_free_args(
	 &smrawmount_fuse_arguments );

	return( EXIT_SUCCESS );

#elif defined( HAVE_LIBDOKAN )
	if( memory_set(
	     &smrawmount_dokan_operations,
	     0,
	     sizeof( DOKAN_OPERATIONS ) ) == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to clear dokan operations.\n" );

		goto on_error;
	}
	if( memory_set(
	     &smrawmount_dokan_options,
	     0,
	     sizeof( DOKAN_OPTIONS ) ) == NULL )
	{
		fprintf(
		 stderr,
		 "Unable to clear dokan options.\n" );

		goto on_error;
	}
	smrawmount_dokan_options.Version     = 600;
	smrawmount_dokan_options.ThreadCount = 0;
	smrawmount_dokan_options.MountPoint  = mount_point;

	if( verbose != 0 )
	{
		smrawmount_dokan_options.Options |= DOKAN_OPTION_STDERR;
#if defined( HAVE_DEBUG_OUTPUT )
		smrawmount_dokan_options.Options |= DOKAN_OPTION_DEBUG;
#endif
	}
/* This will only affect the drive properties
	smrawmount_dokan_options.Options |= DOKAN_OPTION_REMOVABLE;
*/
	smrawmount_dokan_options.Options |= DOKAN_OPTION_KEEP_ALIVE;

	smrawmount_dokan_operations.CreateFile           = &smrawmount_dokan_CreateFile;
	smrawmount_dokan_operations.OpenDirectory        = &smrawmount_dokan_OpenDirectory;
	smrawmount_dokan_operations.CreateDirectory      = NULL;
	smrawmount_dokan_operations.Cleanup              = NULL;
	smrawmount_dokan_operations.CloseFile            = &smrawmount_dokan_CloseFile;
	smrawmount_dokan_operations.ReadFile             = &smrawmount_dokan_ReadFile;
	smrawmount_dokan_operations.WriteFile            = NULL;
	smrawmount_dokan_operations.FlushFileBuffers     = NULL;
	smrawmount_dokan_operations.GetFileInformation   = &smrawmount_dokan_GetFileInformation;
	smrawmount_dokan_operations.FindFiles            = &smrawmount_dokan_FindFiles;
	smrawmount_dokan_operations.FindFilesWithPattern = NULL;
	smrawmount_dokan_operations.SetFileAttributes    = NULL;
	smrawmount_dokan_operations.SetFileTime          = NULL;
	smrawmount_dokan_operations.DeleteFile           = NULL;
	smrawmount_dokan_operations.DeleteDirectory      = NULL;
	smrawmount_dokan_operations.MoveFile             = NULL;
	smrawmount_dokan_operations.SetEndOfFile         = NULL;
	smrawmount_dokan_operations.SetAllocationSize    = NULL;
	smrawmount_dokan_operations.LockFile             = NULL;
	smrawmount_dokan_operations.UnlockFile           = NULL;
	smrawmount_dokan_operations.GetFileSecurity      = NULL;
	smrawmount_dokan_operations.SetFileSecurity      = NULL;
	smrawmount_dokan_operations.GetDiskFreeSpace     = NULL;
	smrawmount_dokan_operations.GetVolumeInformation = &smrawmount_dokan_GetVolumeInformation;
	smrawmount_dokan_operations.Unmount              = &smrawmount_dokan_Unmount;

	result = DokanMain(
	          &smrawmount_dokan_options,
	          &smrawmount_dokan_operations );

	switch( result )
	{
		case DOKAN_SUCCESS:
			break;

		case DOKAN_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: generic error\n" );
			break;

		case DOKAN_DRIVE_LETTER_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: bad drive letter\n" );
			break;

		case DOKAN_DRIVER_INSTALL_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: unable to load driver\n" );
			break;

		case DOKAN_START_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: driver error\n" );
			break;

		case DOKAN_MOUNT_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: unable to assign drive letter\n" );
			break;

		case DOKAN_MOUNT_POINT_ERROR:
			fprintf(
			 stderr,
			 "Unable to run dokan main: mount point error\n" );
			break;

		default:
			fprintf(
			 stderr,
			 "Unable to run dokan main: unknown error: %d\n",
			 result );
			break;
	}
	return( EXIT_SUCCESS );

#else
	fprintf(
	 stderr,
	 "No sub system to mount SMRAW format.\n" );

	return( EXIT_FAILURE );
#endif

on_error:
	if( error != NULL )
	{
		libcnotify_print_error_backtrace(
		 error );
		libcerror_error_free(
		 &error );
	}
#if defined( HAVE_LIBFUSE ) || defined( HAVE_LIBOSXFUSE )
	if( smrawmount_fuse_handle != NULL )
	{
		fuse_destroy(
		 smrawmount_fuse_handle );
	}
	fuse_opt_free_args(
	 &smrawmount_fuse_arguments );
#endif
	if( smrawmount_mount_handle != NULL )
	{
		mount_handle_free(
		 &smrawmount_mount_handle,
		 NULL );
	}
	return( EXIT_FAILURE );
}

