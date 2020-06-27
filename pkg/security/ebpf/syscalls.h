#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "../../ebpf/c/bpf_helpers.h"
#include "filters.h"

struct ktimeval {
    long tv_sec;
    long tv_nsec;
};

struct syscall_cache_t {
    struct policy_t policy;

    u16 type;
    u64 pid;

    union {
        struct {
            int flags;
            umode_t mode;
            struct path *dir;
            struct dentry *dentry;
        } open;

        struct {
            umode_t mode;
            struct path *dir;
            struct dentry *dentry;
        } mkdir;

        struct {
            struct path_key_t path_key;
            int overlay_numlower;
        } unlink;

        struct {
            struct path_key_t path_key;
            int overlay_numlower;
        } rmdir;

        struct {
            struct path *src_dir;
            struct dentry *src_dentry;
            struct path_key_t random_key;
            int src_overlay_numlower;
        } rename;

        struct {
            struct dentry *dentry;
            struct path *path;
            union {
                umode_t mode;
                struct {
                    uid_t user;
                    gid_t group;
                };
                struct {
                    struct ktimeval atime;
                    struct ktimeval mtime;
                };
            };
        } setattr;
    };
};

struct bpf_map_def SEC("maps/syscalls") syscalls = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(u64),
    .value_size = sizeof(struct syscall_cache_t),
    .max_entries = 256,
    .pinning = 0,
    .namespace = "",
};

void __attribute__((always_inline)) cache_syscall(struct syscall_cache_t *syscall) {
    syscall->pid = bpf_get_current_pid_tgid();
    bpf_map_update_elem(&syscalls, &syscall->pid, syscall, BPF_ANY);
}

struct syscall_cache_t * __attribute__((always_inline)) peek_syscall() {
    u64 key = bpf_get_current_pid_tgid();
    return (struct syscall_cache_t *) bpf_map_lookup_elem(&syscalls, &key);
}

struct syscall_cache_t * __attribute__((always_inline)) pop_syscall() {
    u64 key = bpf_get_current_pid_tgid();
    struct syscall_cache_t *syscall = (struct syscall_cache_t*) bpf_map_lookup_elem(&syscalls, &key);
    if (syscall)
        bpf_map_delete_elem(&syscalls, &key);
    return syscall;
}

#endif
