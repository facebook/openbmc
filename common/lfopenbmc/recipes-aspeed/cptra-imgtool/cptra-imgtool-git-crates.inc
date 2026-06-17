# Hand-maintained glue for cptra_imgtool's git dependencies.
#
# Cargo.toml pulls several crates straight from git: the caliptra-sw workspace
# (caliptra-image-*, caliptra-drivers, caliptra-registers, caliptra-cfi-*, ...),
# clundin25's rust-openssl fork (MLDSA support), and korran's ufmt fork (a
# transitive dep). Without these, cargo tries to reach the network at build
# time and fails with "you are in the offline mode (--offline)".
#
# We fetch each repo with BitBake (so it works under --offline) and rewrite the
# dependency declarations to point at the unpacked trees.

SRC_URI += " \
    gitsm://github.com/chipsalliance/caliptra-sw;protocol=https;nobranch=1;name=caliptra_sw;destsuffix=caliptra-sw;type=git-dependency \
    git://github.com/clundin25/rust-openssl.git;protocol=https;nobranch=1;name=openssl;destsuffix=rust-openssl;type=git-dependency \
    git://github.com/korran/ufmt.git;protocol=https;nobranch=1;name=ufmt;destsuffix=ufmt;type=git-dependency \
"

SRCREV_FORMAT = "default_caliptra_sw_openssl_ufmt"
SRCREV_caliptra_sw = "879608b901cc60a68f4e52441e3c494605d08f35"
SRCREV_openssl = "4ada1c1d7c264e052e7bb71ecddbd196a5c2a0c7"
SRCREV_ufmt = "1d0743c1ffffc68bc05ca8eeb81c166192863f33"

# The default cargo_common_do_patch_paths emits a single [patch."<repo>"] entry
# per git SRC_URI. That does not work here, for two independent reasons:
#
#  1. Each of these repos is a Cargo *workspace* exposing many crates from
#     subdirectories (e.g. caliptra-sw exposes caliptra-image-gen at image/gen,
#     caliptra-cfi-lib at cfi/lib, ...). A single root-level patch entry makes
#     cargo fail with "found a virtual manifest ... instead of a package
#     manifest".
#
#  2. More fundamentally, a [patch."<git-url>"] entry does NOT keep cargo from
#     touching the network under --offline. To overlay a patch onto a git
#     source cargo first has to *load* that git source, which it cannot do
#     offline (it is not in the cargo git cache) -- so it still fails with
#     "can't checkout from '<url>': you are in the offline mode (--offline)".
#     openssl is even wired in via `[patch.crates-io] openssl = { git = ... }`,
#     which a [patch."<url>"] entry cannot redirect at all.
#
# Instead, since every git repo is already fetched and unpacked locally, rewrite
# each `{ git = "<repo>", rev = ... }` reference in the manifests to a plain
# `{ path = "<unpacked-dir>" }`. cargo then never needs the git source, the
# whole dependency graph resolves from on-disk paths, and the git sources
# stripped from Cargo.lock below leave every git crate as a path package -- so
# cargo keeps honouring the lockfile rather than re-resolving (which would
# require crate versions that are not vendored).
python cargo_common_do_patch_paths() {
    import os
    import re
    import shutil

    cargo_config = os.path.join(d.getVar("CARGO_HOME"), "config.toml")
    if not os.path.exists(cargo_config):
        return

    unpackdir = d.getVar("UNPACKDIR")

    # A git URL with and without a trailing ".git" denote the same repo; compare
    # in normalised form so SRC_URI, Cargo.lock and Cargo.toml all line up.
    def norm(url):
        return url[:-4] if url.endswith(".git") else url

    # Map each git-dependency repo URL -> its unpacked directory.
    src_uri = (d.getVar("SRC_URI") or "").split()
    fetcher = bb.fetch2.Fetch(src_uri, d)
    repo_dirs = {}
    for url in fetcher.urls:
        ud = fetcher.ud[url]
        if ud.type not in ("git", "gitsm"):
            continue
        destsuffix = ud.parm.get("destsuffix")
        if destsuffix is None:
            continue
        repo = "%s://%s%s" % (ud.proto, ud.host, ud.path)
        repo_dirs[norm(repo)] = os.path.join(unpackdir, destsuffix)

    lockfile = d.getVar("CARGO_LOCK_PATH")
    if not os.path.exists(lockfile):
        bb.fatal(f"{lockfile} file doesn't exist")

    # Read the [package] name out of a Cargo.toml (ignoring other [tables]).
    def package_name(path):
        in_pkg = False
        with open(path) as t:
            for line in t:
                s = line.strip()
                if s.startswith("["):
                    in_pkg = (s == "[package]")
                    continue
                if in_pkg:
                    m = re.match(r'name\s*=\s*"([^"]+)"', s)
                    if m:
                        return m.group(1)
        return None

    # Index crate name -> subdir for a fetched repo tree.
    def crate_dirs(root):
        found = {}
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [x for x in dirnames if x not in (".git", "target")]
            if "Cargo.toml" in filenames:
                pn = package_name(os.path.join(dirpath, "Cargo.toml"))
                if pn:
                    found.setdefault(pn, dirpath)
        return found

    repo_crate_dirs = {repo: crate_dirs(root) for repo, root in repo_dirs.items()}

    # cptra_imgtool ships its own vendored copy of some crates as local
    # workspace members (auth-manifest/{types,gen,app}) *and* the caliptra-sw
    # workspace ships the very same crates (caliptra-sw/auth-manifest/{...}).
    # Online cargo keeps the two apart by source (one path, one git); but once
    # caliptra-sw is consumed as on-disk paths (above) both copies become path
    # packages with the same (name, version), and cargo refuses:
    #   error: package `caliptra-auth-man-types` is specified twice in the lockfile
    #   error: package collision in the lockfile
    # They are the same upstream crate, so unify them onto the local member:
    #   * symlink the colliding copy in the fetched tree to the local member, and
    #   * redirect the fetched repo's own `path = ` references to it onto the
    #     local member's absolute path (rewrite_manifest below) -- a symlink alone
    #     is not enough, cargo identifies a path package by the literal path used
    #     to reach it, not by its canonical target, and
    #   * drop the duplicate git entry from Cargo.lock (below),
    # leaving a single package.
    local_dirs = crate_dirs(d.getVar("S"))

    collided = {}
    for crate, localpath in sorted(local_dirs.items()):
        for found in repo_crate_dirs.values():
            gitpath = found.get(crate)
            if not gitpath:
                continue
            if os.path.realpath(gitpath) == os.path.realpath(localpath):
                continue
            collided[crate] = localpath
            if os.path.islink(gitpath):
                os.unlink(gitpath)
            else:
                shutil.rmtree(gitpath)
            os.symlink(localpath, gitpath)

    # Rewrite every `{ git = "<fetched-repo>", ... }` reference into a path dep.
    # Applies to all dependency tables -- [dependencies], [workspace.dependencies]
    # and [patch.crates-io] -- across the root workspace and each fetched repo
    # (e.g. caliptra-sw's own [workspace.dependencies] pulls ufmt from git).
    def rewrite_manifest(path, redirect_collided):
        changed = False
        out = []
        with open(path) as f:
            for line in f:
                m = re.match(r'(\s*)([A-Za-z0-9_.-]+)\s*=\s*\{(.*)\}\s*(#.*)?$',
                             line)
                if m:
                    indent, key, body = m.group(1), m.group(2), m.group(3)
                    gm = re.search(r'\bgit\s*=\s*"([^"]+)"', body)
                    if gm and norm(gm.group(1)) in repo_dirs:
                        repo = norm(gm.group(1))
                        pm = re.search(r'\bpackage\s*=\s*"([^"]+)"', body)
                        crate = pm.group(1) if pm else key
                        cdir = repo_crate_dirs[repo].get(crate)
                        if cdir is None:
                            bb.fatal("cargo patch: crate '%s' (git %s, in %s) "
                                     "not found under %s"
                                     % (crate, repo, path, repo_dirs[repo]))
                        # Drop the git source selectors; keep any other attrs
                        # (features, default-features, package, ...) untouched.
                        rest = body
                        for k in ("git", "rev", "branch", "tag"):
                            rest = re.sub(r'\b%s\s*=\s*"[^"]*"\s*,?\s*' % k,
                                          "", rest)
                        rest = rest.strip().strip(",").strip()
                        attrs = ['path = "%s"' % cdir]
                        if rest:
                            attrs.append(rest)
                        out.append('%s%s = { %s }\n'
                                   % (indent, key, ", ".join(attrs)))
                        changed = True
                        continue
                    # Point a fetched repo's path dependency on a collided crate
                    # at the single local member (by its absolute path), so cargo
                    # sees one package instead of two same-named path packages.
                    if redirect_collided:
                        pm = re.search(r'\bpackage\s*=\s*"([^"]+)"', body)
                        crate = pm.group(1) if pm else key
                        loc = collided.get(crate)
                        ppath = re.search(r'\bpath\s*=\s*"([^"]+)"', body)
                        if loc and ppath:
                            newbody = (body[:ppath.start(1)] + loc
                                       + body[ppath.end(1):])
                            out.append('%s%s = {%s}\n' % (indent, key, newbody))
                            changed = True
                            continue
                out.append(line)
        if changed:
            with open(path, "w") as f:
                f.writelines(out)

    s_dir = os.path.realpath(d.getVar("S"))
    manifests = set()
    for root in [d.getVar("S")] + list(repo_dirs.values()):
        for dirpath, dirnames, filenames in os.walk(root):
            dirnames[:] = [x for x in dirnames if x not in (".git", "target")]
            if "Cargo.toml" in filenames:
                manifests.add(os.path.join(dirpath, "Cargo.toml"))
    for mani in manifests:
        under_s = os.path.realpath(mani).startswith(s_dir + os.sep)
        rewrite_manifest(mani, redirect_collided=not under_s)

    # Rewrite Cargo.lock to match the patched/unified on-disk graph:
    #  * drop "source = git..." lines so the now path-based deps resolve, and
    #  * for each unified (collided) crate, drop the now-duplicate git package
    #    block and strip the git source qualifier from references to it, leaving
    #    a single package entry.
    lockfile_orig = lockfile + ".orig"
    if not os.path.exists(lockfile_orig):
        shutil.copy(lockfile, lockfile_orig)

    with open(lockfile_orig, "r") as f:
        content = f.read()

    out_blocks = []
    for block in content.split("\n\n"):
        lines = block.split("\n")
        name = None
        is_git = False
        for ln in lines:
            m = re.match(r'name = "([^"]+)"', ln)
            if m:
                name = m.group(1)
            if ln.startswith('source = "git'):
                is_git = True
        # Drop the duplicate git-sourced package block for a unified crate.
        if name in collided and is_git:
            continue
        new_lines = []
        for ln in lines:
            if ln.startswith('source = "git'):
                continue
            for cname in collided:
                ln = re.sub(r'("%s [^"]*?) \(git\+[^"]*\)"' % re.escape(cname),
                            r'\1"', ln)
            new_lines.append(ln)
        out_blocks.append("\n".join(new_lines))

    with open(lockfile, "w") as f:
        f.write("\n\n".join(out_blocks))
}
