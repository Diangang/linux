.. _filesystems_index:

===============================
Filesystems in the Linux kernel
===============================

This under-development manual will, some glorious day, provide
comprehensive information on how the Linux virtual filesystem (VFS) layer
works, along with the filesystems that sit below it.  For now, what we have
can be found below.

Core VFS documentation
======================

See these manuals for documentation about the VFS layer itself and how its
algorithms work.

.. toctree::
   :maxdepth: 2

   vfs
   path-lookup
   api-summary
   splice
   locking
   directory-locking
   devpts
   dnotify
   fiemap
   files
   locks
   mmap_prepare
   multigrain-ts
   mount_api
   quota
   seq_file
   sharedsubtree
   idmappings
   iomap/index

   automount-support

   caching/index

   porting

Filesystem support layers
=========================

Documentation for the support code within the filesystem layer for use in
filesystem implementations.

.. toctree::
   :maxdepth: 2

   buffer
   journalling
   fscrypt
   fsverity

Filesystems
===========

Documentation for filesystem implementations.

.. toctree::
   :maxdepth: 2

   adfs
   bfs
   configfs
   cramfs
   dax
   debugfs
   efivarfs
   inotify
   proc
   ramfs-rootfs-initramfs
   relay
   resctrl
   spufs/index
   sysfs
   tmpfs
