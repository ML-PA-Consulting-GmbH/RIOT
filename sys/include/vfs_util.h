
#ifndef VFS_UTIL_H
#define VFS_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

int vfs_file_from_buffer(const char *name, const void *buf, size_t len);
int vfs_file_to_buffer(const char* file, void* buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* VFS_UTIL_H */
