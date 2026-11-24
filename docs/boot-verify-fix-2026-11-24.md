# Boot verification: ordering-cycle fix and first clean-boot evidence (2026-11-24)

**Host:** Fedora 45, kernel `7.3.4-300.fc45.x86_64`, MTT S30 `0000:01:00.0`
**Packages:** `mtgpu-dkms` 3.1.0-12, `mtgpu-firmware` 3.1.0-4,
`musa-runtime` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit` 3.1.0-3
**Boot:** 2026-11-24 05:45 UTC (`boot id 01bb45a6-9781-45d4-962f-92b2c91e1c1e`)

This note closes the two loops that were open at the end of the
[compute-mode record](compute-mode-enabled-2026-11-24.md): the first reboot
with compute mode active, and the boot-verification service that was supposed
to capture it.

## 1. The reboot with compute mode: PASS

The host rebooted at 05:45 UTC with no blacklist in `/etc/modprobe.d` and the
regenerated initramfs. Observed directly after the boot:

| Check | Result |
|---|---|
| `mtgpu` autoloaded by udev | `initstate=live` |
| `0000:01:00.0` driver | `mtgpu` bound |
| device nodes | `/dev/mtgpu.0`, `/dev/dri/renderD130` |
| `gdm` / `graphical.target` | active, GNOME on Intel `i915` |
| kernel faults this boot | none (`BUG`/`Oops`/`Invalid cmd_type` scan clean) |
| `scripts/compute-mode-boot-verify.sh` | **14 passed, 0 failed, 0 warnings** |
| `scripts/verify-musa.sh` | **14 passed, 0 failed, 1 skipped** (OpenCL, no MT ICD — expected) |
| `musaInfo` | MTT S30, 8 SMs, 3.97 GB, 93% free |
| vector-add (mp_10), compute suite, FP64, memcpy, runtime API, Vulkan | all PASS |
| `mthreads-gmi` | **works on this boot** — see below |

`compute mode survived the reboot` is therefore confirmed on real hardware.
The `[FAIL]`-free suite result also upgrades the expected post-reboot outcome
recorded in the compute-mode doc (13/1/1) to **14/0/1**: the one previously
failing check now passes.

## 2. Why the boot-verify service produced nothing

The journal of the 05:45 boot contains:

```
systemd[1]: musa-boot-verify.service: Found ordering cycle:
  graphical.target/start after multi-user.target/start
  after musa-boot-verify.service/start - after graphical.target
systemd[1]: musa-boot-verify.service: Job musa-boot-verify.service/start
  deleted to break ordering cycle starting with musa-boot-verify.service/start
```

The unit ordered itself `After=graphical.target` while being
`WantedBy=multi-user.target`. Because `graphical.target` requires
`multi-user.target`, the chain

```
musa-boot-verify.start  --After-->  graphical.target.start
                        --Requires-->  multi-user.target.start
                        --Wants-->  musa-boot-verify.start
```

is cyclic. systemd deleted the job; the service never ran,
`/var/log/musa-fedora/` stayed empty, and no evidence was written. The
one-shot self-removal logic never executed either, so the stale unit stayed
installed (disabled-looking but enabled) until this fix.

## 3. Fixes

### 3.1 Unit ordering (`packaging/systemd/musa-boot-verify.service`)

The unit now orders itself only `After=display-manager.service` with
`Wants=display-manager.service`. That pulls GDM into the multi-user
transaction and orders the service after it, with no cycle: nothing orders
`graphical.target` before the service. The GDM/GNOME assertions remain
meaningful because the script now polls for `gdm` (see 3.3). The rationale is
documented in a comment inside the unit.

`systemd-analyze verify` passes for both the in-tree file and the installed
`/etc/systemd/system/musa-boot-verify.service`.

### 3.2 Log-directory writability (`scripts/compute-mode-boot-verify.sh`)

The script fell back to `/tmp` only when `mkdir -p` failed. It kept running
with a broken stdout when the directory existed but was not writable (the
root-owned `/var/log/musa-fedora` during a manual run as a normal user): the
`exec >>` failed, every later `log()` call duplicated onto the console, and
the log file was never created. The script now `touch`es a write-test file
first and falls back to `${TMPDIR:-/tmp}` with a stderr note when the write
fails.

### 3.3 GDM polling (`scripts/compute-mode-boot-verify.sh`)

The `gdm is active` check was a single instant query. With the unit now
ordered after `display-manager.service` it may still run before GDM reports
active, which would have been a false failure. The check now polls
`systemctl is-active gdm` for up to `WAIT_SECS` (90 s in the unit) and only
then fails.

## 4. Service reinstalled

```
bash scripts/install-boot-verify.sh install    # via pkexec
systemctl is-enabled musa-boot-verify.service  # enabled
```

The next reboot will write
`/var/log/musa-fedora/compute-mode-boot-<timestamp>.log` (plus
`compute-mode-boot-latest.log`) and remove itself again.

## 5. `mthreads-gmi` works on this clean boot

The [mthreads-gmi ioctl root cause](root-cause-mthreads-gmi-drm-query-2026-11-24.md)
recorded `Error: failed to initialize mtml.` on the 00:46 UTC boot, where the
module had been live-replaced mid-boot by the package flow. On the 05:45 UTC
clean boot — the module autoloaded once by udev, no live replacement — the
same binary reports the card:

```
$ mthreads-gmi
mthreads-gmi version : 2.0.0
    mthreads-gmi:2.0.0           Driver Version:3.1.0
0    MTT S30        |00000000:01:00.0    |0%    262MiB(4096MiB)
     Physical       |8x(8x)              |45C   NO
```

`verify-musa.sh` therefore passes its telemetry check again. The failure was
**state-dependent, not permanent**: it is associated with boots in which the
module is replaced while the previous instance already initialized hardware.
The root-cause doc is updated accordingly; treat the tool as working on clean
boots and as unvalidated in live-replacement boots.

## 6. Pending cosmetic package update

The host runs `mtgpu-dkms` 3.1.0-12; COPR publishes 3.1.0-13. The delta is a
config-file comment plus spec changelog only (no functional or binary
change), and installing it over the live driver is refused by the package
helper. Upgrade the host to -13 in the next scheduled reboot window; it is
not urgent.
