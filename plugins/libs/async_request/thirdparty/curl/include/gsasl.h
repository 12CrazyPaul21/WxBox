/* gsasl.h --- Header file for GNU SASL Library.
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

#ifndef GSASL_H
# define GSASL_H

/**
 * SECTION:gsasl
 * @title: gsasl.h
 * @short_description: main library interfaces
 *
 * The main library interfaces are declared in gsasl.h.
 */

# include <stdio.h>		/* FILE */
# include <stddef.h>		/* size_t */
# include <unistd.h>		/* ssize_t */

/* Get version symbols. */
# include <gsasl-version.h>

# ifndef _GSASL_API
#  if defined GSASL_BUILDING && defined HAVE_VISIBILITY && HAVE_VISIBILITY
#   define _GSASL_API __attribute__((__visibility__("default")))
#  elif defined GSASL_BUILDING && defined _MSC_VER && ! defined GSASL_STATIC
#   define _GSASL_API __declspec(dllexport)
#  elif defined _MSC_VER && ! defined GSASL_STATIC
#   define _GSASL_API __declspec(dllimport)
#  else
#   define _GSASL_API
#  endif
# endif

# ifdef __cplusplus
extern "C"
{
# endif

  /**
   * Gsasl:
   *
   * Handle to global library context.
   */
  typedef struct Gsasl Gsasl;

  /**
   * Gsasl_session:
   *
   * Handle to SASL session context.
   */
  typedef struct Gsasl_session Gsasl_session;

  /**
   * Gsasl_rc:
   * @GSASL_OK: Successful return code, guaranteed to be always 0.
   * @GSASL_NEEDS_MORE: Mechanism expects another round-trip.
   * @GSASL_UNKNOWN_MECHANISM: Application requested an unknown mechanism.
   * @GSASL_MECHANISM_CALLED_TOO_MANY_TIMES: Application requested too
   *   many round trips from mechanism.
   * @GSASL_MALLOC_ERROR: Memory allocation failed.
   * @GSASL_BASE64_ERROR: Base64 encoding/decoding failed.
   * @GSASL_CRYPTO_ERROR: Cryptographic error.
   * @GSASL_SASLPREP_ERROR: Failed to prepare internationalized string.
   * @GSASL_MECHANISM_PARSE_ERROR: Mechanism could not parse input.
   * @GSASL_AUTHENTICATION_ERROR: Authentication has failed.
   * @GSASL_INTEGRITY_ERROR: Application data integrity check failed.
   * @GSASL_NO_CLIENT_CODE: Library was built with client functionality.
   * @GSASL_NO_SERVER_CODE: Library was built with server functionality.
   * @GSASL_NO_CALLBACK: Application did not provide a callback.
   * @GSASL_NO_ANONYMOUS_TOKEN: Could not get required anonymous token.
   * @GSASL_NO_AUTHID: Could not get required authentication
   *   identity (username).
   * @GSASL_NO_AUTHZID: Could not get required authorization identity.
   * @GSASL_NO_PASSWORD: Could not get required password.
   * @GSASL_NO_PASSCODE: Could not get required SecurID PIN.
   * @GSASL_NO_PIN: Could not get required SecurID PIN.
   * @GSASL_NO_SERVICE: Could not get required service name.
   * @GSASL_NO_HOSTNAME: Could not get required hostname.
   * @GSASL_NO_CB_TLS_UNIQUE: Could not get required tls-unique CB.
   * @GSASL_NO_CB_TLS_EXPORTER: Could not get required tls-exporter CB.
   * @GSASL_NO_SAML20_IDP_IDENTIFIER: Could not get required SAML IdP.
   * @GSASL_NO_SAML20_REDIRECT_URL: Could not get required SAML
   *   redirect URL.
   * @GSASL_NO_OPENID20_REDIRECT_URL: Could not get required OpenID
   *   redirect URL.
   * @GSASL_GSSAPI_RELEASE_BUFFER_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_IMPORT_NAME_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_INIT_SEC_CONTEXT_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_ACCEPT_SEC_CONTEXT_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_UNWRAP_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_WRAP_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_ACQUIRE_CRED_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_DISPLAY_NAME_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_UNSUPPORTED_PROTECTION_ERROR: An unsupported
   *   quality-of-protection layer was requeted.
   * @GSASL_GSSAPI_ENCAPSULATE_TOKEN_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_DECAPSULATE_TOKEN_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_INQUIRE_MECH_FOR_SASLNAME_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_TEST_OID_SET_MEMBER_ERROR: GSS-API library call error.
   * @GSASL_GSSAPI_RELEASE_OID_SET_ERROR: GSS-API library call error.
   * @GSASL_SECURID_SERVER_NEED_ADDITIONAL_PASSCODE: SecurID mechanism
   *   needs an additional passcode.
   * @GSASL_SECURID_SERVER_NEED_NEW_PIN: SecurID mechanism
   *   needs an new PIN.
   *
   * Error codes for library functions.
   */
  typedef enum
  {
    GSASL_OK = 0,
    GSASL_NEEDS_MORE = 1,
    GSASL_UNKNOWN_MECHANISM = 2,
    GSASL_MECHANISM_CALLED_TOO_MANY_TIMES = 3,
    GSASL_MALLOC_ERROR = 7,
    GSASL_BASE64_ERROR = 8,
    GSASL_CRYPTO_ERROR = 9,
    GSASL_SASLPREP_ERROR = 29,
    GSASL_MECHANISM_PARSE_ERROR = 30,
    GSASL_AUTHENTICATION_ERROR = 31,
    GSASL_INTEGRITY_ERROR = 33,
    GSASL_NO_CLIENT_CODE = 35,
    GSASL_NO_SERVER_CODE = 36,
    GSASL_NO_CALLBACK = 51,
    GSASL_NO_ANONYMOUS_TOKEN = 52,
    GSASL_NO_AUTHID = 53,
    GSASL_NO_AUTHZID = 54,
    GSASL_NO_PASSWORD = 55,
    GSASL_NO_PASSCODE = 56,
    GSASL_NO_PIN = 57,
    GSASL_NO_SERVICE = 58,
    GSASL_NO_HOSTNAME = 59,
    GSASL_NO_CB_TLS_UNIQUE = 65,
    GSASL_NO_SAML20_IDP_IDENTIFIER = 66,
    GSASL_NO_SAML20_REDIRECT_URL = 67,
    GSASL_NO_OPENID20_REDIRECT_URL = 68,
    GSASL_NO_CB_TLS_EXPORTER = 69,
    /* Mechanism specific errors. */
    GSASL_GSSAPI_RELEASE_BUFFER_ERROR = 37,
    GSASL_GSSAPI_IMPORT_NAME_ERROR = 38,
    GSASL_GSSAPI_INIT_SEC_CONTEXT_ERROR = 39,
    GSASL_GSSAPI_ACCEPT_SEC_CONTEXT_ERROR = 40,
    GSASL_GSSAPI_UNWRAP_ERROR = 41,
    GSASL_GSSAPI_WRAP_ERROR = 42,
    GSASL_GSSAPI_ACQUIRE_CRED_ERROR = 43,
    GSASL_GSSAPI_DISPLAY_NAME_ERROR = 44,
    GSASL_GSSAPI_UNSUPPORTED_PROTECTION_ERROR = 45,
    GSASL_SECURID_SERVER_NEED_ADDITIONAL_PASSCODE = 48,
    GSASL_SECURID_SERVER_NEED_NEW_PIN = 49,
    GSASL_GSSAPI_ENCAPSULATE_TOKEN_ERROR = 60,
    GSASL_GSSAPI_DECAPSULATE_TOKEN_ERROR = 61,
    GSASL_GSSAPI_INQUIRE_MECH_FOR_SASLNAME_ERROR = 62,
    GSASL_GSSAPI_TEST_OID_SET_MEMBER_ERROR = 63,
    GSASL_GSSAPI_RELEASE_OID_SET_ERROR = 64
      /* When adding new values, note that integers are not necessarily
         assigned monotonously increasingly. */
  } Gsasl_rc;

  /**
   * Gsasl_property:
   * @GSASL_AUTHID: Authentication identity (username).
   * @GSASL_AUTHZID: Authorization identity.
   * @GSASL_PASSWORD: Password.
   * @GSASL_ANONYMOUS_TOKEN: Anonymous identifier.
   * @GSASL_SERVICE: Service name
   * @GSASL_HOSTNAME: Host name.
   * @GSASL_GSSAPI_DISPLAY_NAME: GSS-API credential principal name.
   * @GSASL_PASSCODE: SecurID passcode.
   * @GSASL_SUGGESTED_PIN: SecurID suggested PIN.
   * @GSASL_PIN: SecurID PIN.
   * @GSASL_REALM: User realm.
   * @GSASL_DIGEST_MD5_HASHED_PASSWORD: Pre-computed hashed DIGEST-MD5
   *   password, to avoid storing passwords in the clear.
   * @GSASL_QOPS: Set of quality-of-protection values.
   * @GSASL_QOP: Quality-of-protection value.
   * @GSASL_SCRAM_ITER: Number of iterations in password-to-key hashing.
   * @GSASL_SCRAM_SALT: Salt for password-to-key hashing.
   * @GSASL_SCRAM_SALTED_PASSWORD: Hex-encoded hashed/salted password.
   * @GSASL_SCRAM_SERVERKEY: Hex-encoded SCRAM ServerKey derived
   *   from users' passowrd.
   * @GSASL_SCRAM_STOREDKEY: Hex-encoded SCRAM StoredKey derived
   *   from users' passowrd.
   * @GSASL_CB_TLS_UNIQUE: Base64 encoded tls-unique channel binding.
   * @GSASL_CB_TLS_EXPORTER: Base64 encoded tls-exporter channel binding.
   * @GSASL_SAML20_IDP_IDENTIFIER: SAML20 user IdP URL.
   * @GSASL_SAML20_REDIRECT_URL: SAML 2.0 URL to access in browser.
   * @GSASL_OPENID20_REDIRECT_URL: OpenID 2.0 URL to access in browser.
   * @GSASL_OPENID20_OUTCOME_DATA: OpenID 2.0 authentication outcome data.
   * @GSASL_SAML20_AUTHENTICATE_IN_BROWSER: Request to perform SAML 2.0
   *   authentication in browser.
   * @GSASL_OPENID20_AUTHENTICATE_IN_BROWSER: Request to perform OpenID 2.0
   *   authentication in browser.
   * @GSASL_VALIDATE_SIMPLE: Request for simple validation.
   * @GSASL_VALIDATE_EXTERNAL: Request for validation of EXTERNAL.
   * @GSASL_VALIDATE_ANONYMOUS: Request for validation of ANONYMOUS.
   * @GSASL_VALIDATE_GSSAPI: Request for validation of GSSAPI/GS2.
   * @GSASL_VALIDATE_SECURID: Reqest for validation of SecurID.
   * @GSASL_VALIDATE_SAML20: Reqest for validation of SAML20.
   * @GSASL_VALIDATE_OPENID20: Reqest for validation of OpenID 2.0 login.
   *
   * Callback/property types.
   */
  typedef enum
  {
    /* Information properties, e.g., username. */
    GSASL_AUTHID = 1,
    GSASL_AUTHZID = 2,
    GSASL_PASSWORD = 3,
    GSASL_ANONYMOUS_TOKEN = 4,
    GSASL_SERVICE = 5,
    GSASL_HOSTNAME = 6,
    GSASL_GSSAPI_DISPLAY_NAME = 7,
    GSASL_PASSCODE = 8,
    GSASL_SUGGESTED_PIN = 9,
    GSASL_PIN = 10,
    GSASL_REALM = 11,
    GSASL_DIGEST_MD5_HASHED_PASSWORD = 12,
    GSASL_QOPS = 13,
    GSASL_QOP = 14,
    GSASL_SCRAM_ITER = 15,
    GSASL_SCRAM_SALT = 16,
    GSASL_SCRAM_SALTED_PASSWORD = 17,
    GSASL_SCRAM_SERVERKEY = 23,
    GSASL_SCRAM_STOREDKEY = 24,
    GSASL_CB_TLS_UNIQUE = 18,
    GSASL_SAML20_IDP_IDENTIFIER = 19,
    GSASL_SAML20_REDIRECT_URL = 20,
    GSASL_OPENID20_REDIRECT_URL = 21,
    GSASL_OPENID20_OUTCOME_DATA = 22,
    GSASL_CB_TLS_EXPORTER = 25,
    /* Client callbacks. */
    GSASL_SAML20_AUTHENTICATE_IN_BROWSER = 250,
    GSASL_OPENID20_AUTHENTICATE_IN_BROWSER = 251,
    /* Server validation callback properties. */
    GSASL_VALIDATE_SIMPLE = 500,
    GSASL_VALIDATE_EXTERNAL = 501,
    GSASL_VALIDATE_ANONYMOUS = 502,
    GSASL_VALIDATE_GSSAPI = 503,
    GSASL_VALIDATE_SECURID = 504,
    GSASL_VALIDATE_SAML20 = 505,
    GSASL_VALIDATE_OPENID20 = 506
  } Gsasl_property;

  /**
   * Gsasl_callback_function:
   * @ctx: libgsasl handle.
   * @sctx: session handle, may be NULL.
   * @prop: enumerated value of Gsasl_property type.
   *
   * Prototype of function that the application should implement.  Use
   * gsasl_callback_set() to inform the library about your callback
   * function.
   *
   * It is called by the SASL library when it need some information
   * from the application.  Depending on the value of @prop, it should
   * either set some property (e.g., username or password) using
   * gsasl_property_set(), or it should extract some properties (e.g.,
   * authentication and authorization identities) using
   * gsasl_property_fast() and use them to make a policy decision,
   * perhaps returning GSASL_AUTHENTICATION_ERROR or GSASL_OK
   * depending on whether the policy permitted the operation.
   *
   * Return value: Any valid return code, the interpretation of which
   *   depend on the @prop value.
   *
   * Since: 0.2.0
   **/
  typedef int (*Gsasl_callback_function) (Gsasl * ctx, Gsasl_session * sctx,
					  Gsasl_property prop);

  /**
   * Gsasl_mechname_limits:
   * @GSASL_MIN_MECHANISM_SIZE: Minimum size of mechanism name strings.
   * @GSASL_MAX_MECHANISM_SIZE: Maximum size of mechanism name strings.
   *
   * SASL mechanisms are named by strings, from 1 to 20 characters in
   * length, consisting of upper-case letters, digits, hyphens, and/or
   * underscores.  See also gsasl_mechanism_name_p().
   */
  typedef enum
  {
    GSASL_MIN_MECHANISM_SIZE = 1,
    GSASL_MAX_MECHANISM_SIZE = 20
  } Gsasl_mechname_limits;

  /**
   * Gsasl_qop:
   * @GSASL_QOP_AUTH: Authentication only.
   * @GSASL_QOP_AUTH_INT: Authentication and integrity.
   * @GSASL_QOP_AUTH_CONF: Authentication, integrity and confidentiality.
   *
   * Quality of Protection types (DIGEST-MD5 and GSSAPI).  The
   * integrity and confidentiality values is about application data
   * wrapping.  We recommend that you use @GSASL_QOP_AUTH with TLS as
   * that combination is generally more secure and have better chance
   * of working than the integrity/confidentiality layers of SASL.
   */
  typedef enum
  {
    GSASL_QOP_AUTH = 1,
    GSASL_QOP_AUTH_INT = 2,
    GSASL_QOP_AUTH_CONF = 4
  } Gsasl_qop;

  /**
   * Gsasl_saslprep_flags:
   * @GSASL_ALLOW_UNASSIGNED: Allow unassigned code points.
   *
   * Flags for the SASLprep function, see gsasl_saslprep().  For
   * background, see the GNU Libidn documentation.
   */
  typedef enum
  {
    GSASL_ALLOW_UNASSIGNED = 1
  } Gsasl_saslprep_flags;

  /* Library entry and exit points: version.c, init.c, done.c */
  extern _GSASL_API int gsasl_init (Gsasl ** ctx);
  extern _GSASL_API void gsasl_done (Gsasl * ctx);
  extern _GSASL_API const char *gsasl_check_version (const char *req_version);

  /* Callback handling: callback.c */
  extern _GSASL_API void gsasl_callback_set (Gsasl * ctx,
					     Gsasl_callback_function cb);
  extern _GSASL_API int gsasl_callback (Gsasl * ctx, Gsasl_session * sctx,
					Gsasl_property prop);

  extern _GSASL_API void gsasl_callback_hook_set (Gsasl * ctx, void *hook);
  extern _GSASL_API void *gsasl_callback_hook_get (Gsasl * ctx);

  extern _GSASL_API void gsasl_session_hook_set (Gsasl_session * sctx,
						 void *hook);
  extern _GSASL_API void *gsasl_session_hook_get (Gsasl_session * sctx);

  /* Property handling: property.c */
  extern _GSASL_API int gsasl_property_set (Gsasl_session * sctx,
					    Gsasl_property prop,
					    const char *data);
  extern _GSASL_API int gsasl_property_set_raw (Gsasl_session * sctx,
						Gsasl_property prop,
						const char *data, size_t len);
  extern _GSASL_API void gsasl_property_free (Gsasl_session * sctx,
					      Gsasl_property prop);
  extern _GSASL_API const char *gsasl_property_get (Gsasl_session * sctx,
						    Gsasl_property prop);
  extern _GSASL_API const char *gsasl_property_fast (Gsasl_session * sctx,
						     Gsasl_property prop);

  /* Mechanism handling: listmech.c, supportp.c, suggest.c */
  extern _GSASL_API int gsasl_client_mechlist (Gsasl * ctx, char **out);
  extern _GSASL_API int gsasl_client_support_p (Gsasl * ctx,
						const char *name);
  extern _GSASL_API const char *gsasl_client_suggest_mechanism (Gsasl * ctx,
								const char
								*mechlist);

  extern _GSASL_API int gsasl_server_mechlist (Gsasl * ctx, char **out);
  extern _GSASL_API int gsasl_server_support_p (Gsasl * ctx,
						const char *name);
  extern _GSASL_API int gsasl_mechanism_name_p (const char *mech);

  /* Authentication functions: xstart.c, xstep.c, xfinish.c */
  extern _GSASL_API int gsasl_client_start (Gsasl * ctx, const char *mech,
					    Gsasl_session ** sctx);
  extern _GSASL_API int gsasl_server_start (Gsasl * ctx, const char *mech,
					    Gsasl_session ** sctx);
  extern _GSASL_API int gsasl_step (Gsasl_session * sctx,
				    const char *input, size_t input_len,
				    char **output, size_t *output_len);
  extern _GSASL_API int gsasl_step64 (Gsasl_session * sctx,
				      const char *b64input, char **b64output);
  extern _GSASL_API void gsasl_finish (Gsasl_session * sctx);

  /* Session functions: xcode.c, mechname.c */
  extern _GSASL_API int gsasl_encode (Gsasl_session * sctx,
				      const char *input, size_t input_len,
				      char **output, size_t *output_len);
  extern _GSASL_API int gsasl_decode (Gsasl_session * sctx,
				      const char *input, size_t input_len,
				      char **output, size_t *output_len);
  extern _GSASL_API const char *gsasl_mechanism_name (Gsasl_session * sctx);

  /* Error handling: error.c */
  extern _GSASL_API const char *gsasl_strerror (int err);
  extern _GSASL_API const char *gsasl_strerror_name (int err);

  /* Internationalized string processing: stringprep.c */
  extern _GSASL_API int gsasl_saslprep (const char *in,
					Gsasl_saslprep_flags flags,
					char **out, int *stringpreprc);

  /* Crypto functions: crypto.c */

  /**
   * Gsasl_hash:
   * @GSASL_HASH_SHA1: Hash function SHA-1.
   * @GSASL_HASH_SHA256: Hash function SHA-256.
   *
   * Hash functions.  You may use gsasl_hash_length() to get the
   * output size of a hash function.
   *
   * Currently only used as parameter to
   * gsasl_scram_secrets_from_salted_password() and
   * gsasl_scram_secrets_from_password() to specify for which SCRAM
   * mechanism to prepare secrets for.
   *
   * Since: 1.10
   */
  typedef enum
  {
    /* Hash algorithm identifiers. */
    GSASL_HASH_SHA1 = 2,
    GSASL_HASH_SHA256 = 3,
  } Gsasl_hash;

  /**
   * Gsasl_hash_length:
   * @GSASL_HASH_SHA1_SIZE: Output size of hash function SHA-1.
   * @GSASL_HASH_SHA256_SIZE: Output size of hash function SHA-256.
   * @GSASL_HASH_MAX_SIZE: Maximum output size of any %Gsasl_hash_length.
   *
   * Identifiers specifying the output size of hash functions.
   *
   * These can be used when statically allocating the buffers needed
   * for, e.g., gsasl_scram_secrets_from_password().
   *
   * Since: 1.10
   */
  typedef enum
  {
    /* Output sizes of hashes. */
    GSASL_HASH_SHA1_SIZE = 20,
    GSASL_HASH_SHA256_SIZE = 32,
    GSASL_HASH_MAX_SIZE = GSASL_HASH_SHA256_SIZE
  } Gsasl_hash_length;

  extern _GSASL_API int gsasl_nonce (char *data, size_t datalen);
  extern _GSASL_API int gsasl_random (char *data, size_t datalen);

  extern _GSASL_API size_t gsasl_hash_length (Gsasl_hash hash);

  extern _GSASL_API int
    gsasl_scram_secrets_from_salted_password (Gsasl_hash hash,
					      const char *salted_password,
					      char *client_key,
					      char *server_key,
					      char *stored_key);
  extern _GSASL_API int
    gsasl_scram_secrets_from_password (Gsasl_hash hash,
				       const char *password,
				       unsigned int iteration_count,
				       const char *salt,
				       size_t saltlen,
				       char *salted_password,
				       char *client_key,
				       char *server_key, char *stored_key);

  /* Utilities: md5pwd.c, base64.c, free.c */
  extern _GSASL_API int gsasl_simple_getpass (const char *filename,
					      const char *username,
					      char **key);
  extern _GSASL_API int gsasl_base64_to (const char *in, size_t inlen,
					 char **out, size_t *outlen);
  extern _GSASL_API int gsasl_base64_from (const char *in, size_t inlen,
					   char **out, size_t *outlen);
  extern _GSASL_API int gsasl_hex_to (const char *in, size_t inlen,
				      char **out, size_t *outlen);
  extern _GSASL_API int gsasl_hex_from (const char *in, char **out,
					size_t *outlen);
  extern _GSASL_API void gsasl_free (void *ptr);

  /* Get the mechanism API. */
# include <gsasl-mech.h>

# ifdef __cplusplus
}
# endif

#endif				/* GSASL_H */
