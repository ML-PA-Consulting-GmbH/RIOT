/*
 * Copyright (C) 2023 ML!PA Consulting Gmbh
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    sys_configuration Runtime configuration module
 * @ingroup     sys
 * @brief       Provides and interface for runtime configuration of modules
 *              which must keep persistent configuration parameters
 *
 * Configuration values are accessed in a key - value manner, where a key is
 * basically a path to the correct configuration handler. Handlers are the leaf
 * nodes in an internal tree structure of handler nodes where each node carries
 * a segment of the path key. The handler operations must be implemented
 * by a configuration subsystem. The set() and get() implementations are
 * mandatory for a handler.
 *
 * A configuration subsystem must have been initialized with a storage backend
 * where configuration data is stored and can be exported to.
 * A backend must at least implement the load(), and store() functions.
 *
 * The configuration API is thread save if the configuration subtree has been locked.
 * You would usually lock the subtree, perform some set() or get() operations and check
 * for consistency using verify(). After that you would maybe export your configuration
 * to persistent storage and/or apply the configuration using apply().
 * Be sure to unlock the subtree when you are done.
 *
 * You don´t want to export or apply bad configuration values, so you should call
 * verify() before. You can specify that you want to reimport a configuration value
 * on failing verification. Besides that you can do a get() before you set() a value
 * and restore it when verification fails.
 *
 * A key consists of a static and a dynamic part. The static part identifies the handler
 * node. The dynamic part is often called "next" in the API and the handler is supposed
 * to handle it. The dynamic part is necessary to create new values if the backend
 * supports it in the export() function.
 *
 * @{
 *
 * @file
 * @brief       Runtime configuration API
 *
 * @author      Fabian Hüßler <fabian.huessler@ml-pa-com>
 */

#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <stddef.h>

#include "list.h"
#include "mutex.h"
#include "auto_init_utils.h"
#include "modules.h"

#ifdef __cplusplus
extern "C" {
#endif

#if IS_USED(MODULE_AUTO_INIT_CONFIGURATION) || defined(DOXYGEN)
/**
 * @brief   Add a configuration module to the auto-initialization array
 *
 * @param   function    Function to be called on initialization @ref auto_init_fn_t
 * @param   priority    Priority level @ref auto_init_prio_t
 */
#define AUTO_INIT_CONFIGURATION(function, priority)     _AUTO_INIT(auto_init_configuration_xfa, function, priority)
#else
#define AUTO_INIT_CONFIGURATION(...)
#endif

#if !defined(CONFIG_CONFIGURATION_DEPTH_MAX) || defined(DOXYGEN)
/**
 * @brief   A path to a configuration item must not have more than this number
 *          of segments
 */
#define CONFIG_CONFIGURATION_DEPTH_MAX      8
#endif

/**
 * @brief   @ref CONFIG_CONFIGURATION_DEPTH_MAX
 */
#define CONFIGURATION_DEPTH_MAX             CONFIG_CONFIGURATION_DEPTH_MAX

/**
 * @brief   Forward declaration of a configuration handler
 */
struct conf_handler;

/**
 * @brief   Handler prototype to set a configuration value
 *
 * This is called by @ref configuration_set() for the handler.
 * When @p val and @p size are NULL the configuration handler should delete the
 * internal configuration value or mark it as deleted but not perform the
 * backend delete operation.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 * @param[in]           val         Value to apply to the configuration item identified by the key
 * @param[in, out]      size        Size in bytes of the value to set as input,
 *                                  and 0 as output
 *
 * @return  0 on success
 */
typedef int (*conf_data_set_handler) (const struct conf_handler *handler,
                                      char *key, char *next, const void *val,
                                      size_t *size);

/**
 * @brief   Handler prototype to get a configuration value
 *
 * This is called by @ref configuration_get() for the handler.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 * @param[in]           val         Destination to write the configuration item to
 * @param[in, out]      size        Maximum size in bytes of the value get as input,
 *                                  and remaining length as output
 *
 * @return  0 on success
 */
typedef int (*conf_data_get_handler) (const struct conf_handler *handler,
                                      char *key, char *next, void *val,
                                      size_t *size);

/**
 * @brief   Handler prototype to import a configuration value from persistent storage to
 *          the internal representation location of the configuration item
 *
 * This is called by @ref configuration_import() for the handler.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 *
 * @return  0 on success
 */
typedef int (*conf_data_import_handler) (const struct conf_handler *handler,
                                         char *key, char *next);

/**
 * @brief   Handler prototype to export a configuration value from the internal
 *          representation location of the configuration item to persistent storage
 *
 * This is called by @ref configuration_export() for the handler.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 *
 * @return  0 on success
 */
typedef int (*conf_data_export_handler) (const struct conf_handler *handler,
                                         char *key, char *next);

/**
 * @brief   Handler prototype to delete a configuration value from persistent storage
 *
 * This is called by @ref configuration_delete() for the handler.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 *
 * @return  0 on success
 */
typedef int (*conf_data_delete_handler) (const struct conf_handler *handler,
                                         char *key, char *next);

/**
 * @brief   Handler prototype to verify the internal representation of a configuration item
 *
 * This is called by @ref configuration_verify() for the handler.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 *
 * @return  0 on success
 */
typedef int (*conf_data_verify_handler) (const struct conf_handler *handler,
                                         char *key, char *next);

/**
 * @brief   Handler prototype to apply the internal representation of a configuration item to
 *          the configuration subject, e.g. a sensor
 *
 * This is called by @ref configuration_apply() for the handler.
 * @ref conf_handler_ops_t::set only stores the configuration value to the internal location, and
 * @ref conf_data_apply_handler::apply applies the value to the subject.
 *
 * @param[in]           handler     Reference to the handler
 * @param[in]           key         Configuration key which belongs to the configuration handler
 * @param[in]           next        Comes after the key and the handler knows how to process
 *
 * @return  0 on success
 */
typedef int (*conf_data_apply_handler) (const struct conf_handler *handler,
                                         char *key, char *next);

/**
 * @brief   Configuration handler operations
 */
typedef struct conf_handler_ops {
    conf_data_set_handler set;              /**< Set a configuration value */
    conf_data_get_handler get;              /**< Get a configuration value */
    conf_data_import_handler import;        /**< Import a configuration value
                                                 from persistent storage */
    conf_data_export_handler export;        /**< Export a configuration value
                                                 to persistent storage */
    conf_data_delete_handler delete;        /**< Delete a configuration value
                                                 from persistent storage */
    conf_data_verify_handler verify;        /**< Verify the currently set configuration */
    conf_data_apply_handler apply;          /**< Apply the currently set configuration */
} conf_handler_ops_t;

/**
 * @brief   An intermediate node in the configuration tree
 */
typedef struct conf_handler_node {
    list_node_t node;                       /**< Every node is in a list, managed by its parent */
    struct conf_handler_node *sub_nodes;    /**< Every node has a list of subnodes */
    const char *subtree;                    /**< A segment in a key to a configuration item */
    unsigned level;                         /**< Level in the configuration tree (root = 0) */
} conf_handler_node_t;

/**
 * @brief   Static initializer for an intermediate handler node
 *
 * @param   path        Configuration path segment, e.g. "bar" in "foo/bar/bazz"
 */
#define CONF_HANDLER_NODE_INITIALIZER(path)             \
{                                                       \
    .subtree = path,                                    \
}

/**
 * @brief   A leaf node with handler operations in the configuration tree
 */
typedef struct conf_handler {
    conf_handler_node_t node;           /**< Configuration tree node */
    const conf_handler_ops_t *ops;      /**< Handler operations */
    mutex_t mutex;                      /**< Lock for unique access to the configuration item */
} conf_handler_t;

/**
 * @brief   Static initializer for a configuration handler node
 *
 * @param   path        Configuration path segment
 * @param   operations  Pointer to handler operations
 */
#define CONF_HANDLER_INITIALIZER(path, operations)      \
{                                                       \
    .node = CONF_HANDLER_NODE_INITIALIZER(path),        \
    .ops = operations,                                  \
    .mutex = MUTEX_INIT,                                \
}

/**
 * @brief   Forward declaration for a persistent configuration storage backend
 */
struct conf_backend;

/**
 * @brief   Handler prototype to load configuration data from a persistent storage backend
 *
 * This is called by the configuration handler on import.
 *
 * @param[in]           be          Reference to the backend
 * @param[in]           key         Configuration key which belongs to the configuration item
 * @param[in]           val         Import location of the configuration value
 * @param[in, out]      size        Maximum size of the value to be imported as input,
 *                                  and remaining size as output
 *
 * @return  0 on success
 */
typedef int (*conf_backend_load_handler) (const struct conf_backend *be,
                                          const char *key, void *val, size_t *size);

/**
 * @brief   Handler prototype to store configuration data to a persistent storage backend
 *
 * This is called by the configuration handler on export.
 * Backends with byte granularity can optimize storage access when only a partial value in
 * the export object must be synced. The partial data to be exported is
 * ```
 * ((const uint8_t *)val) + part_offset
 * ```
 * and the size of the partial update is @p part_size.
 *
 * @param[in]           be          Reference to the backend
 * @param[in]           key         Configuration key which belongs to the configuration item
 * @param[in]           val         Export location of the configuration value
 * @param[in, out]      size        Size of the value to be exported as input,
 *                                  and 0 as output
 * @param[in]           part_offset Offset in @p value of the actual data to be updated
 *                                  in the struct stored in the backend
 * @param[in]           part_size   Size of the value to be updated
 *
 * @return  0 on success
 */
typedef int (*conf_backend_store_handler) (const struct conf_backend *be,
                                           const char *key, const void *val, size_t *size,
                                           off_t part_offset, size_t part_size);

/**
 * @brief   Handler prototype to delete configuration data from a persistent storage backend
 *
 * This is called by the configuration handler on delete.
 *
 * @param[in]           be          Reference to the backend
 * @param[in]           key         Configuration key which belongs to the configuration item
 * @param[in]           val         Export location of the configuration value
 *
 * @return  0 on success
 */
typedef int (*conf_backend_delete_handler) (const struct conf_backend *be,
                                            const char *key);

/**
 * @brief   Configuration storage backend operations
 */
typedef struct conf_backend_ops {
    conf_backend_load_handler be_load;          /**< Backend import function */
    conf_backend_store_handler be_store;        /**< Backend export function */
    conf_backend_delete_handler be_delete;      /**< Backend delete function */
} conf_backend_ops_t;

/**
 * @brief   Configuration backend
 */
typedef struct conf_backend {
    const conf_backend_ops_t *ops;              /**< Backend operations */
} conf_backend_t;

/**
 * @brief   Get the root node of the configuration tree
 *
 * @return  Configuration root node
 */
conf_handler_node_t *configuration_get_root(void);

/**
 * @brief   Append a configuration node to the configuration tree
 *
 * @param[in]           parent          Parent configuration node
 * @param[in]           node            New configuration node which is
 *                                      either an intermediate or a leaf node
 */
void configuration_register(conf_handler_node_t *parent, conf_handler_node_t *node);

/**
 * @brief   Import a configuration value by its key from the persistent storage backend
 *
 * @param[in,out]       key             Identifying configuration key
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 * @return -ECANCELED handler error
 */
int configuration_import(char key[]);

/**
 * @brief   Export a configuration value by its key to the persistent storage backend
 *
 * This calls verify() internally and skips the keys for which verification fails.
 *
 * @param[in,out]       key             Identifying configuration key
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 * @return -ECANCELED handler error
 */
int configuration_export(char key[]);

/**
 * @brief   Delete a configuration value by its key from the persistent storage backend
 *
 * @param[in,out]       key             Identifying configuration key
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 */
int configuration_delete(char key[]);

/**
 * @brief   Set the value of a configuration item identified by key
 *
 * @param[in,out]       key             Identifying configuration key
 * @param[in]           value           New configuration value
 * @param[in]           size            Size of the configuration value
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 * @return -ECANCELED handler error
 */
int configuration_set(char key[], const void *value, size_t *size);

/**
 * @brief   Get the value of a configuration item identified by key
 *
 * @param[in,out]       key             Identifying configuration key
 * @param[out]          value           Configuration value
 * @param[in, out]      size            Maximum size of the configuration value
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 * @return -ECANCELED handler error
 */
int configuration_get(char key[], void *value, size_t *size);

/**
 * @brief   Lock a subtree of the configuration tree for unique modification permission
 *
 * @warning Do not lock overlapping subtrees!
 *
 * @param[in,out]       key             Key to identify the subtree
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 */
int configuration_lock(char key[]);

/**
 * @brief   Unlock a subtree of the configuration tree after modification
 *
 * @param[in,out]       key             Key to identify the subtree
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 */
int configuration_unlock(char key[]);

/**
 * @brief   Verify the correctness of a configuration subtree
 *
 * @param[in,out]       key             Key to identify the subtree
 * @param[in]           try_reimport    If true, try to reimport the configuration
 *                                      value if verification fails when a bad value was set
 * @return  0 on success
 * @return -ENOENT no handler found by key
 * @return -ECANCELED verification handler failed
 */
int configuration_verify(char key[], bool try_reimport);

/**
 * @brief   Apply the configuration subtree
 *
 * @param[in,out]       key             Key to identify the subtree
 *
 * @return  0 on success
 * @return -ENOENT no handler found by key
 */
int configuration_apply(char key[]);

#ifdef __cplusplus
}
#endif

#endif /* CONFIGURATION_H */
/** @} */
