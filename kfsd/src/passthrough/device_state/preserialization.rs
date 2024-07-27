// Copyright 2024 Red Hat, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use crate::filesystem::DirectoryIterator;
use crate::fuse;
use crate::passthrough::file_handle::{FileHandle, SerializableFileHandle};
use crate::passthrough::inode_store::{InodeData, InodeIds, StrongInodeReference};
use crate::passthrough::stat::statx;
use crate::passthrough::{self, FileOrHandle, PassthroughFs};
use crate::read_dir::ReadDir;
use crate::util::other_io_error;
use std::convert::TryInto;
use std::ffi::CStr;
use std::fs::File;
use std::io;
use std::os::unix::io::{AsRawFd, FromRawFd};
use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};
use std::sync::{Arc, Mutex};

/// Precursor to `serialized::Inode` that is constructed while serialization is being prepared, and
/// will then be transformed into the latter at the time of serialization.  To be stored in the
/// inode store, alongside each inode (i.e. in its `InodeData`).  Constructing this is costly, so
/// should only be done when necessary, i.e. when actually preparing for migration.
pub(in crate::passthrough) struct InodeMigrationInfo {
    /// Location of the inode (how the destination can find it)
    pub(in crate::passthrough) location: InodeLocation,

    /// The inode's file handle.  The destination is not supposed to open this handle, but instead
    /// compare it against the one from the inode it has opened based on `location`.
    pub(in crate::passthrough) file_handle: Option<SerializableFileHandle>,
}

pub(in crate::passthrough) enum InodeLocation {
    /// The root node: No information is stored, the destination is supposed to find this on its
    /// own (as configured by the user)
    RootNode,

    /// Inode is represented by its parent directory and its filename therein, allowing the
    /// destination to `openat(2)` it
    Path {
        parent: StrongInodeReference,
        filename: String,
    },
}

/// Precursor to `SerializableHandleRepresentation` that is constructed while serialization is
/// being prepared, and will then be transformed into the latter at the time of serialization.
/// To be stored in the `handles` map, alongside each handle (i.e. in its `HandleData`).
/// Constructing this is cheap, so can be done whenever any handle is created.
pub(in crate::passthrough) enum HandleMigrationInfo {
    /// Handle can be opened by opening its associated inode with the given `open(2)` flags
    OpenInode { flags: i32 },
}

/// Stores state for constructing serializable data for inodes using the `InodeMigrationInfo::Path`
/// variant, in order to prepare for migration.
pub(super) struct PathReconstructor<'a> {
    /// Reference to the filesystem for which to reconstruct inodes' paths.
    fs: &'a PassthroughFs,
    /// Set to true when we are supposed to cancel
    cancel: Arc<AtomicBool>,
}

/// Constructs `InodeMigrationInfo` data for every inode in the inode store.  This may take a long
/// time, and is the core part of our preserialization phase.
/// Different implementations of this trait can create different variants of the
/// `InodeMigrationInfo` enum.
pub(super) trait InodeMigrationInfoConstructor {
    /// Runs the constructor
    fn execute(self) -> io::Result<()>;
}

impl InodeMigrationInfo {
    /// Create the migration info for an inode that is collected during the `prepare_serialization`
    /// phase
    pub(in crate::passthrough) fn new(
        fs_cfg: &passthrough::Config,
        parent_ref: StrongInodeReference,
        filename: &CStr,
        file_or_handle: &FileOrHandle,
    ) -> io::Result<Self> {
        let utf8_name = filename.to_str().map_err(|err| {
            other_io_error(format!(
                "Cannot convert filename into UTF-8: {filename:?}: {err}"
            ))
        })?;

        Self::new_with_utf8_name(fs_cfg, parent_ref, utf8_name, file_or_handle)
    }

    fn new_with_utf8_name(
        fs_cfg: &passthrough::Config,
        parent_ref: StrongInodeReference,
        filename: &str,
        file_or_handle: &FileOrHandle,
    ) -> io::Result<Self> {
        let file_handle: Option<SerializableFileHandle> = if fs_cfg.migration_verify_handles {
            Some(file_or_handle.try_into()?)
        } else {
            None
        };

        Ok(InodeMigrationInfo {
            location: InodeLocation::Path {
                parent: parent_ref,
                filename: filename.to_string(),
            },
            file_handle,
        })
    }
}

impl HandleMigrationInfo {
    /// Create the migration info for a handle that will be required when serializing
    pub(in crate::passthrough) fn new(flags: i32) -> Self {
        HandleMigrationInfo::OpenInode {
            // Remove flags that make sense when the file is first opened by the guest, but which
            // we should not set when continuing to use the file after migration because they would
            // e.g. modify the file
            flags: flags & !(libc::O_CREAT | libc::O_EXCL | libc::O_TRUNC),
        }
    }
}

/// The `PathReconstructor` is an `InodeMigrationInfoConstructor` that creates `InodeMigrationInfo`
/// of the `InodeMigrationInfo::Path` variant: It recurses through the filesystem (i.e. the shared
/// directory), matching up all inodes it finds with our inode store, and thus finds the parent
/// directory node and filename for every such inode.
impl<'a> PathReconstructor<'a> {
    pub(super) fn new(fs: &'a PassthroughFs, cancel: Arc<AtomicBool>) -> Self {
        PathReconstructor { fs, cancel }
    }

    /// Recurse from the given directory inode
    fn recurse_from(&self, root_ref: StrongInodeReference) -> io::Result<()> {
        let mut dir_buf = vec![0u8; 1024];

        // We don't actually use recursion (to not exhaust the stack), but keep a list of
        // directories we still need to visit, and pop from it until it is empty and we're done
        let mut remaining_dirs = vec![root_ref];
        while let Some(inode_ref) = remaining_dirs.pop() {
            let dirfd = inode_ref.get().open_file(
                libc::O_RDONLY | libc::O_NOFOLLOW | libc::O_CLOEXEC,
                &self.fs.proc_self_fd,
            )?;

            // Read all directory entries, check them for matches in our inode store, and add any
            // directory to `remaining_dirs`
            loop {
                // Safe because we use nothing but this function on the FD
                let mut entries = unsafe { ReadDir::new_no_seek(&dirfd, dir_buf.as_mut()) }?;
                if entries.remaining() == 0 {
                    break;
                }

                while let Some(entry) = entries.next() {
                    if self.cancel.load(Ordering::Relaxed) {
                        return Err(other_io_error("Cancelled serialization preparation"));
                    }

                    if let Some(entry_inode) = self.discover(&inode_ref, &dirfd, entry.name)? {
                        // Add directories to visit to the list
                        remaining_dirs.push(entry_inode);
                    }
                }
            }
        }

        Ok(())
    }

    /// Check the given directory entry (parent + name) for matches in our inode store.  If we find
    /// any corresponding `InodeData` there, its `.migration_info` is set accordingly.
    /// For all directories (and directories only), return a strong reference to an inode in our
    /// store that can be used to recurse further.
    fn discover<F: AsRawFd>(
        &self,
        parent_reference: &StrongInodeReference,
        parent_fd: &F,
        name: &CStr,
    ) -> io::Result<Option<StrongInodeReference>> {
        let utf8_name = name.to_str().map_err(|err| {
            other_io_error(format!(
                "Cannot convert filename into UTF-8: {name:?}: {err}",
            ))
        })?;

        // Ignore these
        if utf8_name == "." || utf8_name == ".." {
            return Ok(None);
        }

        let path_fd = {
            let fd = self
                .fs
                .open_relative_to(parent_fd, name, libc::O_PATH, None)?;
            unsafe { File::from_raw_fd(fd) }
        };
        let stat = statx(&path_fd, None)?;
        let handle = self.fs.get_file_handle_opt(&path_fd, &stat)?;

        let ids = InodeIds {
            ino: stat.st.st_ino,
            dev: stat.st.st_dev,
            mnt_id: stat.mnt_id,
        };

        let is_directory = stat.st.st_mode & libc::S_IFMT == libc::S_IFDIR;

        if let Ok(inode_ref) = self.fs.inodes.claim_inode(handle.as_ref(), &ids) {
            let file_handle = if self.fs.cfg.migration_verify_handles {
                Some(match &handle {
                    Some(h) => h.into(),
                    None => FileHandle::from_fd_fail_hard(&path_fd)?.into(),
                })
            } else {
                None
            };

            let mig_info = InodeMigrationInfo {
                location: InodeLocation::Path {
                    parent: StrongInodeReference::clone(parent_reference),
                    filename: utf8_name.to_string(),
                },
                file_handle,
            };

            *inode_ref.get().migration_info.lock().unwrap() = Some(mig_info);

            return Ok(is_directory.then_some(inode_ref));
        }

        // We did not find a matching entry in our inode store.  In case of non-directories, we are
        // done.
        if !is_directory {
            return Ok(None);
        }

        // However, in case of directories, we must create an entry, so we can return it.
        // (Our inode store may still have matching entries recursively downwards from this
        // directory.  Because every node is serialized referencing its parent, this directory
        // inode may end up being recursively referenced this way, we don't know yet.
        // In case there is no such entry, the refcount will eventually return to 0 before
        // `Self::execute()` returns, dropping it from the inode store again, so it will not
        // actually end up being serialized.)

        let file_or_handle = if let Some(h) = handle.as_ref() {
            FileOrHandle::Handle(self.fs.make_file_handle_openable(h)?)
        } else {
            FileOrHandle::File(path_fd)
        };

        let mig_info = InodeMigrationInfo::new_with_utf8_name(
            &self.fs.cfg,
            StrongInodeReference::clone(parent_reference),
            utf8_name,
            &file_or_handle,
        )?;

        let new_inode = InodeData {
            inode: self.fs.next_inode.fetch_add(1, Ordering::Relaxed),
            file_or_handle,
            refcount: AtomicU64::new(1),
            ids,
            mode: stat.st.st_mode,
            migration_info: Mutex::new(Some(mig_info)),
        };

        Ok(Some(self.fs.inodes.get_or_insert(new_inode)?))
    }
}

impl InodeMigrationInfoConstructor for PathReconstructor<'_> {
    /// Recurse from the root directory (the shared directory)
    fn execute(self) -> io::Result<()> {
        if let Ok(root) = self.fs.inodes.get_strong(fuse::ROOT_ID) {
            // Root node is special in that the destination gets no information on how to find it,
            // because that is configured by the user
            *root.get().migration_info.lock().unwrap() = Some(InodeMigrationInfo {
                location: InodeLocation::RootNode,
                file_handle: if self.fs.cfg.migration_verify_handles {
                    Some((&root.get().file_or_handle).try_into()?)
                } else {
                    None
                },
            });

            self.recurse_from(root)
        } else {
            // No root node?  Then the filesystem is not mounted and we do not need to do anything.
            Ok(())
        }
    }
}
