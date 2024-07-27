// Copyright 2024 Red Hat, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Module for migrating our internal FS state (i.e. serializing and deserializing it), with the
 * following submodules:
 * - serialized: Serialized data structures
 * - preserialization: Structures and functionality for preparing for migration (serialization),
 *                     i.e. define and construct the precursors to the eventually serialized
 *                     information that are stored alongside the associated inodes and handles they
 *                     describe
 * - serialization: Functionality for serializing
 * - deserialization: Functionality for deserializing
 */
mod deserialization;
pub(super) mod preserialization;
mod serialization;
mod serialized;

use crate::filesystem::SerializableFileSystem;
use crate::passthrough::PassthroughFs;
use preserialization::{InodeMigrationInfoConstructor, PathReconstructor};
use std::convert::{TryFrom, TryInto};
use std::fs::File;
use std::io::{self, Read, Write};
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Arc;

/// Adds serialization (migration) capabilities to `PassthroughFs`
impl SerializableFileSystem for PassthroughFs {
    fn prepare_serialization(&self, cancel: Arc<AtomicBool>) -> io::Result<()> {
        self.inodes.clear_migration_info();

        // Set this so the filesystem code knows that every node is supposed to have up-to-date
        // migration information.  For example, nodes that are created after they would have been
        // visited by the reconstructor below will not get migration info, unless the general
        // filesystem code makes an effort to set it (when the node is created).
        self.track_migration_info.store(true, Ordering::Relaxed);

        // Create the reconstructor (which reconstructs parent+filename information for each node
        // in our inode store), and run it
        let reconstructor = PathReconstructor::new(self, cancel);
        let result = reconstructor.execute();
        if result.is_err() {
            // Do not leave incomplete data behind (cancelling returns an error, too, landing here)
            self.inodes.clear_migration_info();
        }
        result
    }

    fn serialize(&self, mut state_pipe: File) -> io::Result<()> {
        self.track_migration_info.store(false, Ordering::Relaxed);

        let state = serialized::PassthroughFs::V1(self.try_into()?);
        self.inodes.clear_migration_info();
        let serialized: Vec<u8> = state.try_into()?;
        state_pipe.write_all(&serialized)?;
        Ok(())
    }

    fn deserialize_and_apply(&self, mut state_pipe: File) -> io::Result<()> {
        let mut serialized: Vec<u8> = Vec::new();
        state_pipe.read_to_end(&mut serialized)?;
        match serialized::PassthroughFs::try_from(serialized)? {
            serialized::PassthroughFs::V1(state) => state.apply(self)?,
        };
        Ok(())
    }
}
