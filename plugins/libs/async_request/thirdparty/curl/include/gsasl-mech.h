/* gsasl-mech.h --- Header file for mechanism handling in GNU SASL Library.
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

#ifndef GSASL_MECH_H
# define GSASL_MECH_H

/**
 * SECTION:gsasl-mech
 * @title: gsasl-mech.h
 * @short_description: register new application-defined mechanism
 *
 * The builtin mechanisms should suffice for most applications.
 * Applications can register a new mechanism in the library using
 * application-supplied functions.  The mechanism will operate as the
 * builtin mechanisms, and the supplied functions will be invoked when
 * necessary.  The application uses the normal logic, e.g., calls
 * gsasl_client_start() followed by a sequence of calls to
 * gsasl_step() and finally gsasl_finish().
 */

/**
 * Gsasl_init_function:
 * @ctx: a %Gsasl libgsasl handle.
 *
 * The implementation of this function pointer should fail if the
 * mechanism for some reason is not available for further use.
 *
 * Return value: Returns %GSASL_OK iff successful.
 **/
typedef int (*Gsasl_init_function) (Gsasl * ctx);

/**
 * Gsasl_done_function:
 * @ctx: a %Gsasl libgsasl handle.
 *
 * The implementation of this function pointer deallocate all
 * resources associated with the mechanism.
 **/
typedef void (*Gsasl_done_function) (Gsasl * ctx);

/**
 * Gsasl_start_function:
 * @sctx: a %Gsasl_session session handle.
 * @mech_data: pointer to void* with mechanism-specific data.
 *
 * The implementation of this function should start a new
 * authentication process.
 *
 * Return value: Returns %GSASL_OK iff successful.
 **/
typedef int (*Gsasl_start_function) (Gsasl_session * sctx, void **mech_data);

/**
 * Gsasl_step_function:
 * @sctx: a %Gsasl_session session handle.
 * @mech_data: pointer to void* with mechanism-specific data.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: newly allocated output byte array.
 * @output_len: pointer to output variable with size of output byte array.
 *
 * The implementation of this function should perform one step of the
 * authentication process.
 *
 * This reads data from the other end (from @input and @input_len),
 * processes it (potentially invoking callbacks to the application),
 * and writes data to server (into newly allocated variable @output
 * and @output_len that indicate the length of @output).
 *
 * The contents of the @output buffer is unspecified if this functions
 * returns anything other than %GSASL_OK or %GSASL_NEEDS_MORE.  If
 * this function return %GSASL_OK or %GSASL_NEEDS_MORE, however, the
 * @output buffer is allocated by this function, and it is the
 * responsibility of caller to deallocate it by calling
 * gsasl_free(@output).
 *
 * Return value: Returns %GSASL_OK if authenticated terminated
 *   successfully, %GSASL_NEEDS_MORE if more data is needed, or error
 *   code.
 **/
typedef int (*Gsasl_step_function) (Gsasl_session * sctx, void *mech_data,
				    const char *input, size_t input_len,
				    char **output, size_t *output_len);

/**
 * Gsasl_finish_function:
 * @sctx: a %Gsasl_session session handle.
 * @mech_data: pointer to void* with mechanism-specific data.
 *
 * The implementation of this function should release all resources
 * associated with the particular authentication process.
 **/
typedef void (*Gsasl_finish_function) (Gsasl_session * sctx, void *mech_data);

/**
 * Gsasl_code_function:
 * @sctx: a %Gsasl_session session handle.
 * @mech_data: pointer to void* with mechanism-specific data.
 * @input: input byte array.
 * @input_len: size of input byte array.
 * @output: newly allocated output byte array.
 * @output_len: pointer to output variable with size of output byte array.
 *
 * The implementation of this function should perform data encoding or
 * decoding for the mechanism, after authentication has completed.
 * This might mean that data is integrity or privacy protected.
 *
 * The @output buffer is allocated by this function, and it is the
 * responsibility of caller to deallocate it by calling
 * gsasl_free(@output).
 *
 * Return value: Returns %GSASL_OK if encoding was successful,
 *   otherwise an error code.
 **/
typedef int (*Gsasl_code_function) (Gsasl_session * sctx, void *mech_data,
				    const char *input, size_t input_len,
				    char **output, size_t *output_len);

/**
 * Gsasl_mechanism_functions:
 * @init: a Gsasl_init_function().
 * @done: a Gsasl_done_function().
 * @start: a Gsasl_start_function().
 * @step: a Gsasl_step_function().
 * @finish: a Gsasl_finish_function().
 * @encode: a Gsasl_code_function().
 * @decode: a Gsasl_code_function().
 *
 * Holds all function pointers to implement a mechanism, in either
 * client or server mode.
 */
struct Gsasl_mechanism_functions
{
  Gsasl_init_function init;
  Gsasl_done_function done;
  Gsasl_start_function start;
  Gsasl_step_function step;
  Gsasl_finish_function finish;
  Gsasl_code_function encode;
  Gsasl_code_function decode;
};
typedef struct Gsasl_mechanism_functions Gsasl_mechanism_functions;

/**
 * Gsasl_mechanism:
 * @name: string holding name of mechanism, e.g., "PLAIN".
 * @client: client-side #Gsasl_mechanism_functions structure.
 * @server: server-side #Gsasl_mechanism_functions structure.
 *
 * Holds all implementation details about a mechanism.
 */
struct Gsasl_mechanism
{
  const char *name;

  struct Gsasl_mechanism_functions client;
  struct Gsasl_mechanism_functions server;
};
typedef struct Gsasl_mechanism Gsasl_mechanism;

/* Register new mechanism: register.c. */
extern _GSASL_API int gsasl_register (Gsasl * ctx,
				      const Gsasl_mechanism * mech);

#endif /* GSASL_MECH_H */
