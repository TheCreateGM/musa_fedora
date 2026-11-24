# Root cause: `Invalid cmd_type: 1 or cmd: 12` (2026-11-23 boot)

Date: 2026-11-23
Host: Fedora 45 Workstation, kernel `7.3.4-300.fc45.x86_64`, boot started
2026-11-23 07:32 UTC
GPU: Moore Threads MTT S30 `0000:01:00.0` (`1ed5:0102`), bound to the live
`mtgpu` module

Scope of this record: the two kernel messages that cause
`scripts/test-gpu.sh` to refuse to run in this boot. It is a read-only
analysis plus one minimal, controlled reproduction. **No driver load/unload,
no unbind/rebind, no boot or blacklist change, and no GPU workload**
(compute, memcpy, kernel launch) was performed.

## Symptom

Three occurrences in this boot, all identical:

```
Nov 23 08:04:47 fedora kernel: mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
Nov 23 08:30:26 fedora kernel: mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
Nov 23 09:33:33 fedora kernel: mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12
```

Because the current boot's journal contains this line,
`scripts/test-gpu.sh` reports:

```
GPU_TEST_STATUS: NOT RUN
BLOCKED: this boot logged an mtgpu Invalid cmd_type error; reboot before GPU tests
```

The gate in this case is *over*-conservative: this message is a **rejected
ioctl, not a driver fault**. The proof is that it is still present in this
journal after the third occurrence and the machine shows no `BUG:`, no
`Oops:`, no NULL-pointer dereference and no taint escalation; GNOME Shell
(`/usr/bin/gnome-shell --mode=user`, pid 5168) and `gdm` remain `active`.

## What the message means

The format string lives in the *prebuilt vendor core object*, not in the
open part of the driver:

```
$ strings /tmp/mtgpu_live.ko | grep -F 'Invalid cmd_type'
Invalid cmd_type: %u or cmd: %u
Invalid cmd: %u for cmd_type: %u
KMD & UMD version is incompatible for cmd %s
```

It is emitted from `driver-s30-3.1.0/objs/x86_64/mtgpu_core.o_binary`
(confirmed by `grep -ao 'Invalid cmd_type: %u or cmd: %u'`), the proprietary
object the project links against. The open-source dispatcher is only
*declared* in `inc/mtgpu-next/mtgpu_ioctl.h`
(`extern const struct mtgpu_ioctl_desc query_cmd_ioctl_desc[MTGPU_QUERY_CMD_MAX];`),
so the bounds check itself is inside that binary.

The two printed values are `cmd_type = 1` and `cmd = 12`.
`cmd_type = 1` is `MTGPU_QUERY_CMD`:

```
$ grep -n 'define MTGPU_QUERY_CMD' inc/shared_include/linux/uapi/drm/mtgpu_drm.h
42:#define MTGPU_QUERY_CMD				0x01	/* Query commands ... */
```

## The UAPI skew (already recorded on 2026-11-16, re-verified here)

The **installed 3.1.0 UAPI** stops at value 11:

```c
/* inc/shared_include/linux/uapi/drm/mtgpu_drm.h (this tree, shipped as 3.1.0) */
	MTGPU_QUERY_CMD_DRIVER_INFO,     /* Query driver information */
	MTGPU_QUERY_CMD_MISC_INFO,       /* Query misc info */        /* == 11 */
	MTGPU_QUERY_CMD_MAX,             /* Maximum value ... */      /* == 12 */
```

So the 3.1.0 driver accepts sub-commands `0..11` and treats `12` as out of
range. The **5.2.0 UAPI** inserted two new sub-commands *before* `MAX`:

```c
/* musa-data/research/official-musa-5.2.0/extracted-dkms/.../mtgpu_drm.h:79-95 */
	MTGPU_QUERY_CMD_MISC_INFO,       /* == 11 */
	MTGPU_QUERY_CMD_MKT_NAME,        /* == 12 */   <-- the rejected value
	MTGPU_QUERY_CMD_MEM_INFO_V2,     /* == 13 */
	MTGPU_QUERY_CMD_MAX,             /* == 14 */
```

`cmd == 12` is therefore exactly `MTGPU_QUERY_CMD_MKT_NAME` ("query marketing
name"), a 5.2-era request that the 3.1.0 driver does not implement. The
number is not random and not corrupted — it is a version skew between the
userspace and the kernel.

> This is the same finding recorded in
> [`root-cause-iommu.md`](root-cause-iommu.md) item 2 on 2026-11-16
> ("The 5.2 runtime asks for 12 and the kernel logs
> `Invalid cmd_type: 1 or cmd: 12`"). This document adds the *trigger path*
> for the 2026-11-23 occurrences and a controlled reproduction.

## The trigger path: an orphan `/etc/profile.d/musa.sh`

1. `/etc/profile.d/musa.sh` exists on the host but is **not owned by any
   package** (`rpm -qf` → `file /etc/profile.d/musa.sh is not owned by any
   package`), mtime `2026-11-10 05:37:23 UTC`. Content:

   ```
   export MUSA_HOME="/usr/local/musa-5.2"
   export PATH="/usr/local/musa-5.2/bin:/usr/sbin:/usr/bin:/sbin:/bin:/root/bin"
   export LD_LIBRARY_PATH="/usr/local/musa-5.2/lib:"
   ```

2. None of the five shipped RPMs creates it (`rpm -ql musa-sdk`,
   `musa-runtime`, `musa-toolkit`, `mtgpu-dkms`, `mtgpu-firmware` → no
   `/etc/profile.d` entry; only `/etc/ld.so.conf.d/*.conf`). The vendor
   `musa_3.1.0-pc_x86.deb` ships no `profile.d` either.

3. Its content is the **shell-expanded form** of the heredoc in the
   disabled deployment draft
   `musa-data/research/disabled-deployment-drafts-2026-11-11/install-musa-sdk.sh`
   lines 60–64. Precise provenance caveat: that retained draft uses
   `<<'EOF'` (quoted) and would have written the *literal* `${PATH}` text,
   and the file is untracked — `musa-data/.gitignore:88` is `/research/*` —
   so **no revision history of the producing script is retained**. The
   installed bytes (expanded `${PATH}` → `:/usr/sbin:/usr/bin:/sbin:/bin:/root/bin`,
   expanded `${LD_LIBRARY_PATH:-}` → trailing empty field) are what an
   *unquoted* variant of that same heredoc produces in a root login shell.
   mtime proximity (05:36:28 script / 05:37:23 file, 2026-11-10) supports
   that this draft family wrote it. Treat the exact producing revision as
   unknown-but-in-this-family.

4. Consequently, in **every login shell**, an unqualified `musaInfo`
   resolves to the 5.2 binary:

   ```
   $ bash -lc 'command -v musaInfo'
   /usr/local/musa-5.2/bin/musaInfo
   ```

   `/usr/local/musa-5.2/bin/musaInfo` is itself **not owned by any package**
   (a raw SDK-deb leftover) and `readelf -d` shows `NEEDED libmusart.so.5`.
   `LD_DEBUG=libs` confirms it searches
   `/usr/local/musa-5.2/lib/...` for `libmusart.so.5` — i.e. the
   *incompatible* 5.2.0 runtime, not the packaged 1.5.4 runtime at
   `/usr/local/musa`.

5. That 5.2.0 runtime talks to the 3.1.0 `mtgpu` driver and issues
   `MTGPU_QUERY_CMD_MKT_NAME` (12). The driver rejects it, the runtime
   sees `-22`/`EINVAL`, and 5.2-era userspace then aborts.

## Controlled reproduction (2026-11-23 09:33:33 UTC)

Minimal, read-only reproduction of the exact chain — one invocation of the
already-orphaned 5.2 binary from a login shell, no test harness, no module
operation, no compute:

```
$ bash -lc 'musaInfo'
mtgpu_device_initialize: mtgpu_query_marketing_name failed (-22)
musaInfo: ../../libdrm-mt/mtgpu/mtgpu_internal.h:162: update_references: \
  Assertion `atomic_read(dst) > 0' failed.
```

`mtgpu_query_marketing_name` is the `MTGPU_QUERY_CMD_MKT_NAME` call; `-22` is
`EINVAL`, i.e. the driver-side `cmd >= MTGPU_QUERY_CMD_MAX` rejection. The
journal recorded a third `Invalid cmd_type: 1 or cmd: 12` at
`09:33:33`, one second after the invocation — a deterministic reproduction of
the 08:04:47 and 08:30:26 events.

Post-reproduction health check (this boot): no `BUG:`/`Oops`/NULL-pointer
message, `gnome-shell` still running, `gdm` still `active`, `lsmod` still
shows `mtgpu` live. This confirms the message is a **handled error path**,
not a kernel fault.

## Why the staged test binaries are *not* the cause

Audited independently (`readelf -d ~/.musa-work/test-bin/*`):

| binary | `DT_NEEDED` | `DT_RUNPATH` |
|---|---|---|
| `musa-compute` | `libmusart.so.1.5` | `/usr/local/musa/lib` |
| `musa-fp64` | `libmusart.so.1.5` | `/usr/local/musa/lib` |
| `musart-smoke` | `libmusart.so.1.5` | `/usr/local/musa/lib` |
| `musa-vector-add` | `libmusart.so.1.5` | `/usr/local/musa/lib` |
| `vk-enum` | `libc.so.6` | — |

All four MUSA binaries are pinned to the packaged 1.5.4 runtime through
`RUNPATH`, which is the ABI that matches the 3.1.0 driver. The 5.2
shadowing only affects binaries that are resolved through the login-shell
`PATH`/`LD_LIBRARY_PATH` set by the orphan `profile.d` file.

## Scope of the defect

* Not a kernel bug: the driver correctly refuses an out-of-range sub-command
  for its own UAPI.
* Not a MUSA-1.5.4 bug: 1.5.4 does not send cmd 12.
* Not a packaging bug in the five shipped RPMs: none of them writes the
  offending file.
* It **is** an environment/packaging gap on the host: an unowned, unversioned
  `profile.d` from a *disabled* deployment draft shadows the supported
  runtime system-wide.

## Fix ownership and status

* **System-wide prevention (packaging), LANDED but not installed on this
  host:** `musa-runtime` **1.5.4-5** ships
  `/etc/profile.d/zz-musa-fedora.sh`, owned by agent `98357c53`
  (`packaging/rpm/SPECS/musa-runtime.spec`, `scripts/check-musa-env.sh`).
  Verified by reading the spec diff:

  * it deliberately uses a **different filename that sorts after `musa.sh`**
    (`m…` < `z…`), so it does not have to take ownership of the orphan file;
  * it rewrites `PATH` and `LD_LIBRARY_PATH` by *filtering out and
    re-prepending* the two managed prefixes, so the ordering holds even if
    the orphan snippet runs first;
  * it is `%config(noreplace)`, so a local edit is preserved.

  Built and published by COPR: build `11379194` (`axogm/musa-fedora`) and
  `11379219` (`axogm/musa-mtt-s30`), both `succeeded` in all 9 chroots.
  *This closed the packaging hazard I had originally reported below.*
* **⚠️ Resolved hazard note (recorded for history):** I initially flagged
  that shipping a file literally named `/etc/profile.d/musa.sh` would collide
  with the existing *unowned* file on RPM install (`rpm` never replaces an
  unowned file, so the incompatible ordering would have silently survived,
  or the install could refuse). The `zz-` prefix design above removes this
  hazard without root surgery, so no `pkexec` deletion of the orphan is
  required for the prevention to work.
* **⚠️ Open (host):** the host still has `musa-runtime 1.5.4-4` installed
  (`rpm -q musa-runtime`), so `zz-musa-fedora.sh` is **not yet present
  locally** and the orphan still wins in login shells here. Installing
  `1.5.4-5` is a normal `dnf upgrade` of an already-reviewed package, but
  `mtgpu-dkms` is `3.1.0-11` while COPR has `3.1.0-12`, and the package
  helper refuses transactions that replace a *live* module — so the upgrade
  must be done in a boot where `mtgpu` is not loaded, i.e. after the planned
  reboot. Deferred deliberately; see [`compute-mode.md`](compute-mode.md).
* **Host remediation:** removing `/etc/profile.d/musa.sh` and
  `/usr/local/musa-5.2/bin/musaInfo` (or moving them aside with a backup) is
  a root-owned system change. It was **deliberately not executed** in this
  session — it needs `pkexec`, a backup, and explicit approval. Until then,
  the documented workaround is to use `source scripts/musa-env.sh`, which
  already prepends `/usr/local/musa`.

## Effect on the test gate

The gate (`scripts/test-gpu.sh`, owned by agent `1a06816d`) treats any
`Invalid cmd_type` in the current boot as a hard block. That is fail-closed
and correct as a *policy*, and it was **not** relaxed here. What this
document changes is the *interpretation*:

* blocking is justified by provenance, not by severity — the line cannot by
  itself distinguish "rejected ioctl, driver healthy" from a real fault;
* a clean reboot plus removal of the 5.2 shadowing is the correct way to
  restore a runnable test boot, not an exception carved into the script.

## Evidence index

| Claim | Command |
|---|---|
| Message present 3× this boot | `journalctl -b 0 -g 'Invalid cmd_type'` |
| Format string in live module | `xz -dc /lib/modules/$(uname -r)/extra/mtgpu.ko.xz \| strings \| grep -F 'Invalid cmd_type'` |
| Print site is the vendor core | `grep -ao 'Invalid cmd_type: %u or cmd: %u' driver-s30-3.1.0/objs/x86_64/mtgpu_core.o_binary` |
| 3.1.0 UAPI max = 12 | `sed -n '76,88p' inc/shared_include/linux/uapi/drm/mtgpu_drm.h` |
| 5.2.0 UAPI inserts MKT_NAME = 12 | `sed -n '79,95p' ../musa-data/research/official-musa-5.2.0/extracted-dkms/usr/src/mtgpu-5.2.0-server/inc/shared_include/linux/uapi/drm/mtgpu_drm.h` |
| `profile.d` orphaned | `rpm -qf /etc/profile.d/musa.sh` |
| 5.2 binary resolution | `bash -lc 'command -v musaInfo'` |
| Deterministic reproduction | `bash -lc 'musaInfo'` → `marketing_name failed (-22)` + journal 09:33:33 |
| No kernel fault after repro | `journalctl -b 0 -g 'BUG:\|Oops\|NULL pointer'`, `pgrep -a gnome-shell`, `systemctl is-active gdm` |
| Staged binaries pinned to 1.5.4 | `readelf -d ~/.musa-work/test-bin/*` |
