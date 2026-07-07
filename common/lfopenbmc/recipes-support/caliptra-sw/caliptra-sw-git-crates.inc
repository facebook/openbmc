# Hand-maintained glue for the workspace's git dependencies.
# Maps the caliptra-cfi and ufmt git deps declared in Cargo.toml to the
# BitBake-fetched sources so cargo can build fully offline.

SRC_URI += " \
    git://github.com/chipsalliance/caliptra-cfi.git;protocol=https;nobranch=1;name=caliptra_cfi;destsuffix=caliptra-cfi;type=git-dependency \
    git://github.com/korran/ufmt.git;protocol=https;nobranch=1;name=ufmt;destsuffix=ufmt;type=git-dependency \
"

SRCREV_FORMAT = "default_caliptra_cfi_ufmt"
SRCREV_caliptra_cfi = "a98e499d279e81ae85881991b1e9eee354151189"
SRCREV_ufmt = "1d0743c1ffffc68bc05ca8eeb81c166192863f33"

# The default cargo_common_do_patch_paths emits a single [patch."<repo>"] entry
# per git SRC_URI, named after the SRC_URI `name=` and pointing at the repo
# root. That is wrong for caliptra-cfi: its repo root is a *virtual* Cargo
# workspace (no package), and the crates the workspace depends on live in
# subdirectories (lib -> caliptra-cfi-lib-git, derive -> caliptra-cfi-derive-git).
# Without this override cargo fails with:
#   error: failed to load source for dependency `caliptra_cfi`
#   ... found a virtual manifest ... instead of a package manifest
# Override the function to emit one [patch] entry per real crate, each pointing
# at its own subdirectory.
python cargo_common_do_patch_paths() {
    import os
    import shutil

    unpackdir = d.getVar("UNPACKDIR")
    patchsets = [
        (
            "https://github.com/chipsalliance/caliptra-cfi.git",
            [
                ("caliptra-cfi-derive-git", "caliptra-cfi/derive"),
                ("caliptra-cfi-lib-git",    "caliptra-cfi/lib"),
            ],
        ),
        (
            "https://github.com/korran/ufmt.git",
            [
                ("ufmt", "ufmt"),
            ],
        ),
    ]

    cargo_config = os.path.join(d.getVar("CARGO_HOME"), "config.toml")
    with open(cargo_config, "a+") as config:
        for repo, entries in patchsets:
            print('\n[patch."%s"]' % repo, file=config)
            for name, relpath in entries:
                print('%s = { path = "%s" }' % (name, os.path.join(unpackdir, relpath)), file=config)

    # Strip git source lines from Cargo.lock so the patched path deps resolve
    # under cargo's --frozen flag (mirrors the upstream default function).
    lockfile = d.getVar("CARGO_LOCK_PATH")
    if not os.path.exists(lockfile):
        bb.fatal(f"{lockfile} file doesn't exist")

    lockfile_orig = lockfile + ".orig"
    if not os.path.exists(lockfile_orig):
        shutil.copy(lockfile, lockfile_orig)

    with open(lockfile_orig, "r") as f:
        newlines = [line for line in f.readlines()
                    if not line.startswith('source = "git')]

    with open(lockfile, "w") as f:
        f.writelines(newlines)
}
