# User namespaces and linux capabilities

Small test program which mounts a squashfs in a private mount namespace.
If capabilities are not enabled, user mappings will be at bit wonky
and all groups and users other than the current user
will appears as `nobody`/`nfsnobody`.

In most cases the program should terminate cleanly and
not leave any mount or children hanging.


- <https://www.man7.org/linux/man-pages/man7/user_namespaces.7.html>
for user namespace documentation.

- <https://man7.org/linux/man-pages/man7/capabilities.7.html>
for capability documentation.

- <https://man7.org/linux/man-pages/man2/prctl.2.html>
for `PR_SET_DUMPABLE`

### Flow

1. Check that squashfs and mount point exists
2. Unshare user and mount namespace
3. Map user uid to 0 and all other uids and gids mapped 1->1
4. Unshare user namespace
5. Map uid 0 back to original user uid and all other uids and gids mapped 1->1
6. Launch user command



### Compiling

```
gcc virt.c -o virt
gcc set_mappings.c -o set_mappings -lcap
```

### Enabling user namespaces on centos
```
echo "100" | tee /proc/sys/user/max_user_namespaces
```

### Setting Capabilities
```
sudo setcap 'cap_setuid,cap_setgid+pie' set_mappings
```
### Running
```
./virt squashfs_file mount_point cmd <optional cmd args>
```

Any existing folder is a valid mount point. 
Mounting is done in a user namespace where
the user uid is mapped to 0 so so places like
`/mnt` and `/opt` are valid.


### Why is set_mapping a separate file?
When a file with capabilities is executed
the "dumpable" attribute is set to the default value in `/proc/sys/fs/suid_dumpable`
(0 most of the time). This affects the `/proc` access rights
and they are only accesible by root. So the target process
can not have the capabilites. Might be fixable with some other
program flow (if you can reverse the dumpable for a child).

Won't do this for this simple test.

### Sequrity (mostly my speculation)
User namespaces expose a larger part of the kernel code
which might have not been as thoroughly tested,
so bug risk is higher.

