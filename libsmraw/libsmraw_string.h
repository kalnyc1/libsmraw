/*
 * Character type string functions
 *
 * Copyright (c) 2010, Joachim Metz <jbmetz@users.sourceforge.net>
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

#if !defined( _LIBSMRAW_STRING_H )
#define _LIBSMRAW_STRING_H

#include <common.h>
#include <types.h>

#include <libcstring.h>
#include <liberror.h>

#if defined( _cplusplus )
extern "C" {
#endif

int libsmraw_string_copy_to_64bit_decimal(
     libcstring_character_t *string,
     size_t string_size,
     uint64_t *value_64bit,
     liberror_error_t **error );

int libsmraw_string_copy_to_64bit_hexadecimal(
     libcstring_character_t *string,
     size_t string_size,
     uint64_t *value_64bit,
     liberror_error_t **error );

#if defined( _cplusplus )
}
#endif

#endif

