# setting the capabilites  for the set binary
sudo setcap 'cap_setuid,cap_setgid+pie' set
# creating the squashfs
mksquashfs <dir> <squashfs_name>
echo "100" | tee /proc/sys/user/max_user_namespaces
