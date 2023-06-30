/* gsasl-version.h --- Header file for GNU SASL Library version symbols.
 * Copyright (C) 2002-2022 Simon Josefsson
 *
 * This file is part of GNU SASL Library.
 *
 * GNU SASL Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * GNU SASL Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License License along with GNU SASL Library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef GSASL_VERSION_H
# define GSASL_VERSION_H

/**
 * SECTION:gsasl-version
 * @title: gsasl-version.h
 * @short_description: version symbols
 *
 * The gsasl-version.h file contains version symbols.  It should not
 * be included directly, only via gsasl.h.
 */

/**
 * GSASL_VERSION
 *
 * Pre-processor symbol with a string that describe the header file
 * version number.  Used together with gsasl_check_version() to
 * verify header file and run-time library consistency.
 */
# define GSASL_VERSION "2.2.0"

/**
 * GSASL_VERSION_MAJOR
 *
 * Pre-processor symbol with a decimal value that describe the major
 * level of the header file version number.  For example, when the
 * header version is 1.2.3 this symbol will be 1.
 *
 * Since: 1.1
 */
# define GSASL_VERSION_MAJOR 2

/**
 * GSASL_VERSION_MINOR
 *
 * Pre-processor symbol with a decimal value that describe the minor
 * level of the header file version number.  For example, when the
 * header version is 1.2.3 this symbol will be 2.
 *
 * Since: 1.1
 */
# define GSASL_VERSION_MINOR 2

/**
 * GSASL_VERSION_PATCH
 *
 * Pre-processor symbol with a decimal value that describe the patch
 * level of the header file version number.  For example, when the
 * header version is 1.2.3 this symbol will be 3.
 *
 * Since: 1.1
 */
# define GSASL_VERSION_PATCH 0

/**
 * GSASL_VERSION_NUMBER
 *
 * Pre-processor symbol with a hexadecimal value describing the
 * header file version number.  For example, when the header version
 * is 1.2.3 this symbol will have the value 0x010203.
 *
 * Since: 1.1
 */
# define GSASL_VERSION_NUMBER 0x020200

#endif /* GSASL_VERSION_H */
