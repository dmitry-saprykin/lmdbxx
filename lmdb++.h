/* This is free and unencumbered software released into the public domain. */

#ifndef LMDBXX_H
#define LMDBXX_H

/**
 * <lmdb++.h> - C++11 wrapper for LMDB.
 *
 * @author Arto Bendiken <arto@bendiken.net>
 * @see https://sourceforge.net/projects/lmdbxx/
 */

#ifndef __cplusplus
#error "<lmdb++.h> requires a C++ compiler"
#endif

////////////////////////////////////////////////////////////////////////////////

#include <lmdb.h>    /* for MDB_*, mdb_*() */

#include <cstddef>   /* for std::size_t */
#include <cstdio>    /* for std::snprintf() */
#include <stdexcept> /* for std::runtime_error */

namespace lmdb {
  using mode = mdb_mode_t;
}

////////////////////////////////////////////////////////////////////////////////
/* Error Handling */

namespace lmdb {
  class error;
  class key_exist_error;
  class not_found_error;
}

/**
 * Base class for LMDB exception conditions.
 */
class lmdb::error : public std::runtime_error {
protected:
  const int _code;

public:
  /**
   * Throws an error based on the given LMDB return code.
   */
  [[noreturn]] static inline void raise(const char* origin, int rc);

  /**
   * Constructor.
   */
  error(const char* const origin,
        const int rc) noexcept
    : runtime_error{origin},
      _code{rc} {}

  /**
   * Returns the underlying LMDB error code.
   */
  int code() const noexcept {
    return _code;
  }

  /**
   * Returns the origin of the LMDB error.
   */
  const char* origin() const noexcept {
    return runtime_error::what();
  }

  /**
   * Returns the underlying LMDB error code.
   */
  virtual const char* what() const noexcept {
    static thread_local char buffer[1024];
    std::snprintf(buffer, sizeof(buffer),
      "%s: %s", origin(), ::mdb_strerror(code()));
    return buffer;
  }
};

/**
 * Exception class for `MDB_KEYEXIST` errors.
 */
class lmdb::key_exist_error final : public lmdb::error {
public:
  using error::error;
};

/**
 * Exception class for `MDB_NOTFOUND` errors.
 */
class lmdb::not_found_error final : public lmdb::error {
public:
  using error::error;
};

inline void
lmdb::error::raise(const char* const origin,
                   const int rc) {
  switch (rc) {
    case MDB_KEYEXIST: throw key_exist_error{origin, rc};
    case MDB_NOTFOUND: throw not_found_error{origin, rc};
    default: throw error{origin, rc};
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Environment */

namespace lmdb {
  static inline void env_create(MDB_env** env);
  static inline void env_open(MDB_env* env,
    const char* path, unsigned int flags, mode mode);
  static inline void env_close(MDB_env* env);
  static inline void env_set_flags(MDB_env* env, unsigned int flags, bool onoff);
  static inline void env_set_map_size(MDB_env* env, std::size_t size);
  static inline void env_set_max_readers(MDB_env* env, unsigned int count);
  static inline void env_set_max_dbs(MDB_env* env, MDB_dbi count);
  static inline void env_sync(MDB_env* env, bool force);
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_create(MDB_env** env) {
  const int rc = ::mdb_env_create(env);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_create", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_open(MDB_env* env,
               const char* const path,
               const unsigned int flags,
               const mode mode) {
  const int rc = ::mdb_env_open(env, path, flags, mode);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_open", rc);
  }
}

/**
 * @note never throws an exception
 */
static inline void
lmdb::env_close(MDB_env* env) {
  ::mdb_env_close(env);
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_flags(MDB_env* env,
                    const unsigned int flags,
                    const bool onoff = true) {
  const int rc = ::mdb_env_set_flags(env, flags, onoff ? 1 : 0);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_flags", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_map_size(MDB_env* env,
                       const std::size_t size) {
  const int rc = ::mdb_env_set_mapsize(env, size);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_mapsize", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_max_readers(MDB_env* env,
                          const unsigned int count) {
  const int rc = ::mdb_env_set_maxreaders(env, count);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_maxreaders", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_set_max_dbs(MDB_env* env,
                      const MDB_dbi count) {
  const int rc = ::mdb_env_set_maxdbs(env, count);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_set_maxdbs", rc);
  }
}

/**
 * @throws lmdb::error on failure
 */
static inline void
lmdb::env_sync(MDB_env* env,
               const bool force = true) {
  const int rc = ::mdb_env_sync(env, force);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_env_sync", rc);
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Transactions */

namespace lmdb {
  static inline void txn_begin(
    MDB_env* env, MDB_txn* parent, unsigned int flags, MDB_txn** txn);
  static inline MDB_env* txn_env(MDB_txn* const txn);
  static inline void txn_commit(MDB_txn* txn);
  static inline void txn_abort(MDB_txn* txn);
  static inline void txn_reset(MDB_txn* txn);
  static inline void txn_renew(MDB_txn* txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#gad7ea55da06b77513609efebd44b26920
 */
static inline void
lmdb::txn_begin(MDB_env* const env,
                MDB_txn* const parent,
                const unsigned int flags,
                MDB_txn** txn) {
  const int rc = ::mdb_txn_begin(env, parent, flags, txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_begin", rc);
  }
}

/**
 * @note never throws an exception
 * @see http://symas.com/mdb/doc/group__mdb.html#gaeb17735b8aaa2938a78a45cab85c06a0
 */
static inline MDB_env*
lmdb::txn_env(MDB_txn* const txn) {
  return ::mdb_txn_env(txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga846fbd6f46105617ac9f4d76476f6597
 */
static inline void
lmdb::txn_commit(MDB_txn* const txn) {
  const int rc = ::mdb_txn_commit(txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_commit", rc);
  }
}

/**
 * @note never throws an exception
 * @see http://symas.com/mdb/doc/group__mdb.html#ga73a5938ae4c3239ee11efa07eb22b882
 */
static inline void
lmdb::txn_abort(MDB_txn* const txn) {
  ::mdb_txn_abort(txn);
}

/**
 * @note never throws an exception
 * @see http://symas.com/mdb/doc/group__mdb.html#ga02b06706f8a66249769503c4e88c56cd
 */
static inline void
lmdb::txn_reset(MDB_txn* const txn) {
  ::mdb_txn_reset(txn);
}

/**
 * @throws lmdb::error on failure
 * @see http://symas.com/mdb/doc/group__mdb.html#ga6c6f917959517ede1c504cf7c720ce6d
 */
static inline void
lmdb::txn_renew(MDB_txn* const txn) {
  const int rc = ::mdb_txn_renew(txn);
  if (rc != MDB_SUCCESS) {
    error::raise("mdb_txn_renew", rc);
  }
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Databases */

namespace lmdb {
  // TODO
}

////////////////////////////////////////////////////////////////////////////////
/* Procedural Interface: Cursors */

namespace lmdb {
  // TODO
}

////////////////////////////////////////////////////////////////////////////////

#endif /* LMDBXX_H */
