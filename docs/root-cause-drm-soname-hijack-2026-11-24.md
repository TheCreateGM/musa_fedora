# Duplicate SONAME: a 5.2.0 perf shim hijacks `libdrm_mtgpu.so.1`

**Date:** 2026-11-24
**Host:** Fedora 45, kernel `7.3.4-300.fc45.x86_64`, MTT S30
**Packages:** `musa-runtime` 1.5.4-5 (owns the DRM userspace library),
`musa-sdk` 5.2.0-6, `mtgpu-dkms` 3.1.0-12

## Symptom

`rpm -V musa-runtime` reported a link mismatch that no package update
explained:

```
$ rpm -V musa-runtime
....L....    /usr/lib64/libdrm_mtgpu.so.1
$ readlink /usr/lib64/libdrm_mtgpu.so.1
libmtperf_target.so
```

The package ships `libdrm_mtgpu.so.1 -> libdrm_mtgpu.so.1.0.0`. Something
had replaced the link target with `libmtperf_target.so`, an **unowned**
file in a system library directory:

```
$ ls -la /usr/lib64/libmtperf_target.so
-r-xr-xr-x. 1 axogm axogm 137384 Nov 11  2025 /usr/lib64/libmtperf_target.so
$ rpm -qf /usr/lib64/libmtperf_target.so
file /usr/lib64/libmtperf_target.so is not owned by any package
```

Both libraries advertise the **same SONAME**:

```
$ objdump -p /usr/lib64/libmtperf_target.so    | grep SONAME
  SONAME               libdrm_mtgpu.so.1
$ objdump -p /usr/lib64/libdrm_mtgpu.so.1.0.0 | grep SONAME
  SONAME               libdrm_mtgpu.so.1
```

`libmtperf_target.so` comes from the **5.2.0** Debian package
`libdrm-mtgpu_5.2.0_amd64.deb` (staged under
`musa-data/research/official-musa-5.2.0/packages/...`), not from the
3.1.0/1.5.4 stack that this project packages. It was copied into
`/usr/lib64` manually on 2025-11-11 and is not managed by RPM.

## Mechanism, reproduced in isolation

`ldconfig` creates the `soname -> file` link for every library that
provides a SONAME. When two files in the search path advertise the same
SONAME, it picks one; the evidence shows it picks the 5.2.0 shim regardless
of modification time. Reproducer under `/tmp` (`ldconfig -n <dir>`):

| Case | Layout | Result |
|------|--------|--------|
| A | correct symlink `-> libdrm_mtgpu.so.1.0.0` + `libmtperf_target.so` | link **repointed** to `libmtperf_target.so` |
| B | `.so.1` is a regular file copy of the real library + shim | left alone; `ldconfig: ... is not a symbolic link` |
| C | as A, but the real library touched to a newer mtime | link still repointed to the shim |

So the link is not repaired by reinstalling and running `ldconfig` while
the foreign file is present:

```
pkexec rpm -Uvh --replacepkgs --nosignature musa-runtime-1.5.4-5.fc45.x86_64.rpm
ls -la /usr/lib64/libdrm_mtgpu.so.1
lrwxrwxrwx. 1 root root 19 ... /usr/lib64/libdrm_mtgpu.so.1 -> libmtperf_target.so
```

(The `%post` of `musa-runtime` runs `/sbin/ldconfig`; any ldconfig run with
the shim present re-hijacks the link.)

## Impact

* Every process that loads `libdrm_mtgpu.so.1` gets the 5.2.0 perf shim
  instead of the packaged 3.1.0/1.5.4 library - a silent library
  substitution, not a hard error.
* `rpm -V` flags the package (the `L` above), so integrity checks fail.
* This was **not** the cause of the `mthreads-gmi` failure: that tool does
  not load `libdrm_mtgpu` at all (see
  `docs/root-cause-mthreads-gmi-drm-query-2026-11-24.md`). Both defects were
  present on the same host and were investigated independently.

## Remediation performed on the host

The foreign shim was quarantined (moved, not deleted) and the packaged link
restored:

```sh
Q=$HOME/.musa-work/quarantine/2026-11-24-5.2-drm-shim
# manifest.txt records the original path and sha256
pkexec mv /usr/lib64/libmtperf_target.so "$Q/"
pkexec ldconfig
pkexec rpm -Uvh --replacepkgs --nosignature \
  packaging/rpm/local-build/RPMS/x86_64/musa-runtime-1.5.4-5.fc45.x86_64.rpm
```

Verification after the fix:

```
$ readlink /usr/lib64/libdrm_mtgpu.so.1
libdrm_mtgpu.so.1.0.0
$ rpm -V musa-runtime && echo clean
clean
$ ./scripts/verify-musa.sh     # DRM section
PASS  libdrm_mtgpu.so.1 resolves to the packaged 3.1.0 library
```

Sha256 of the quarantined file:
`ffda62a98aacf7e20f795ceb68c133674cd2e188f7b00aebbf48a4fd0a87105a`.
Restore (only if the 5.2.0 perf shim is deliberately needed):

```sh
pkexec mv "$Q/libmtperf_target.so" /usr/lib64/ && pkexec ldconfig
# be aware: this will hijack the 1.5.4 link again
```

## Prevention in this project

* `scripts/verify-musa.sh` now has a **DRM userspace library** section: it
  requires `libdrm_mtgpu.so.1 -> libdrm_mtgpu.so.1.0.0`, and on failure scans
  the ldconfig path for other files with SONAME `libdrm_mtgpu.so.1` and
  names the culprit instead of only reporting a broken link.
* Never place a second library with this SONAME in `/usr/lib64`, `/lib64`, or
  any directory listed in `/etc/ld.so.conf*`. A perf shim belongs in a
  non-scanned directory and should be selected explicitly with
  `LD_PRELOAD`/`LD_LIBRARY_PATH`.
* `musa-runtime` owns the link and runs `ldconfig` in `%post`; because RPM
  cannot prevent a foreign SONAME provider from winning, the verification
  check is the guard that makes the state visible.

## Reproduce the check

```sh
readlink /usr/lib64/libdrm_mtgpu.so.1
rpm -V musa-runtime
for f in /usr/lib64/*.so*; do
  objdump -p "$f" 2>/dev/null | grep -q "SONAME.*libdrm_mtgpu\.so\.1" && echo "$f"
done
```