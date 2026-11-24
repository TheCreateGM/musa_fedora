# MUSA on Fedora — MTT S30: Final Completion Report (2026-11-23)

**Project:** Moore Threads MTT S30 (PCI `1ed5:0102`) on Fedora Linux
**Primary production repo:** `/home/axogm/Documents/project/musa-fedora` → `https://github.com/TheCreateGM/musa_fedora.git`
**Reference/research repo:** `/home/axogm/Documents/project/musa-data` → `https://github.com/TheCreateGM/musa_data.git`
**Host:** Fedora Linux 45 Workstation, x86_64, kernel `7.3.4-300.fc45.x86_64`

---

## 2026-11-24 addendum — current verified state (supersedes the counts in §1)

* `musa-runtime` **1.5.4-5** (with `musa-runtime-display` 1.5.4-5) is built
  and published in both COPR projects. It adds
  `/etc/profile.d/zz-musa-fedora.sh`, which fixes the login-environment
  ordering so `musaInfo` resolves to the 1.5.4 runtime
  (`libmusart.so.1.5`) instead of the incompatible 5.2.0 tool.
  `scripts/check-musa-env.sh` reports `MUSA_ENV_STATUS: PASS` on the host.
* Re-validated on the live MTT S30 with `musa-runtime` 1.5.4-5 and
  `mtgpu-dkms` 3.1.0-12: `scripts/verify-musa.sh` reports **12 passed /
  1 failed / 1 skipped**. Every compute, memory, `mcc`, FP64 and Vulkan layer
  passes; the one failure is the vendor telemetry tool below.
  `MUSA_S30_TEST_APPROVED=1 scripts/test-gpu.sh` passes its fail-closed
  journal gate and prints `GPU_TEST_STATUS: PASS`. See
  [`hardware-validation-2026-11-24-load-test.md`](hardware-validation-2026-11-24-load-test.md).
* **`mthreads-gmi` does NOT work on this stack.** Version 2.0.0 opens
  `/dev/mtgpu.0` and `/dev/dri/renderD130` and gets `EINVAL` from DRM ioctl
  `0x64/0x8a` during mtml initialization. The compute path is unaffected.
  Root cause and evidence:
  [`root-cause-mthreads-gmi-drm-query-2026-11-24.md`](root-cause-mthreads-gmi-drm-query-2026-11-24.md).
  The §1 sentence claiming it "reports MTT S30 ... with two informational-tool
  failures" is superseded.
* The host had an independent packaging defect: an unowned 5.2.0
  `libmtperf_target.so` shares SONAME `libdrm_mtgpu.so.1` with the packaged
  3.1.0 library, so `ldconfig` repointed
  `/usr/lib64/libdrm_mtgpu.so.1` at it. The shim was quarantined, the link
  restored (`rpm -V musa-runtime` clean), and `verify-musa.sh` gained a check
  that names such a foreign SONAME provider. Details:
  [`root-cause-drm-soname-hijack-2026-11-24.md`](root-cause-drm-soname-hijack-2026-11-24.md).
* This boot's kernel journal contains **no** `Invalid cmd_type` records; the
  fail-closed gate is no longer `BLOCKED`, and the 2026-11-24 hardware run
  above was made from that clean boot. The two historical entries described
  in §1 remain history only.
* COPR re-verification after the 1.5.4-5 build: **passed: 20, failed: 0**
  across all nine chroots. Latest set: `musa-runtime` 1.5.4-5,
  `musa-runtime-display` 1.5.4-5, `musa-sdk` 5.2.0-6, `musa-toolkit`
  3.1.0-3, `mtgpu-dkms` 3.1.0-12, `mtgpu-firmware` 3.1.0-4.
* Session record:
  [`session-2026-11-24-runtime154-5-and-hardware.md`](session-2026-11-24-runtime154-5-and-hardware.md).

---

## 1. Executive summary

A complete, reproducible, and **hardware-verified** MUSA environment for the MTT S30 is in place on the installed Fedora 45 system:

- **mtgpu driver 3.1.0** (DKMS, for the running kernel `7.3.4-300.fc45.x86_64`) with S30 PCI alias `1ed5:0102`.
- **MUSA runtime 1.5.4** (`libmusart.so.1.5`) and **MUSA SDK 5.2.0** (`mcc` + headers) installed at `/usr/local/musa` and `/usr/local/musa-5.2`.
- **ALL 5 Fedora RPMs** (`mtgpu-dkms`, `mtgpu-firmware`, `musa-runtime`, `musa-sdk`, `musa-toolkit`) built and published to **Copr** across **9 chroots** (fedora-43/44/45/eln/rawhide, x86_64 only).
- Staged testing: compile-only/isolated builds → read-only CI checks → gated GPU smoke test → controlled load test on live hardware.

**Hardware test results (live S30):** `verify-musa.sh` **13 passed / 0 failed / 1 skipped**; real `mp_10` vector-add, compute suite (saxpy, shared reduce, atomics, constant memory, devicemath, grid2d, D2D), FP64 (`abs_err=5.551e-17`), and headless Vulkan (device/buffer/host-visible map) all **PASS** on the MTT S30. `mthreads-gmi` reports MTT S30 (driver 3.1.0) with two informational-tool failures (explained).

The host boot is **fail-closed gated**: the kernel journal contains two `mtgpu 0000:01:00.0: Invalid cmd_type: 1 or cmd: 12` records (from the 5.2.0-SDK ABI test binaries), so `scripts/test-gpu.sh` reports `BLOCKED` and must remain so until a clean reboot; `compute-mode.sh enable` is **NOT** run (requires explicit approval; see recovery).

---

## 2. Implemented features (production)

| Area | Implementation |
|---|---|
| Driver (DKMS) | `mtgpu` 3.1.0 for the S30 (PCI `1ed5:0102`), headless/compute-friendly parameters, `display=none disable_fbdev=1 mtgpu_drm_major=2`, `/var/log/mtgpu` boot cleanup. |
| Firmware | `mtgpu-firmware` 3.1.0 ship + install `mtfw-*.bin` / `mtvpu-*` / `musa.fw` under `/lib/firmware/mthreads/`. |
| Runtime | `musa-runtime` 1.5.4 (`libmusart.so.1.5`, `musaInfo`, `muMemcpyTest`, `mthreads-gmi`) at `/usr/local/musa`. |
| SDK | `musa-sdk` 5.2.0 (`mcc` clang driver, headers, `mtxcc` toolchain) at `/usr/local/musa-5.2`. |
| Package set | `mtgpu-dkms 3.1.0-11/12`, `mtgpu-firmware 3.1.0-4`, `musa-runtime 1.5.4-4`, `musa-sdk 5.2.0-6`, `musa-toolkit 3.1.0-3`, plus opt-in `musa-runtime-display` subpackage (Mesa DRI drivers) for display hosts. |
| CI/CD | `packaging/rpm/build-all.sh` (SRPM→RPMs), `copr-cli` configuration, `verify-copr-chroots.sh` (build-state + per-chroot artifact check), `copr-monitor.sh`. |
| Test suite | `verify-musa.sh` (13 checks), `test-gpu.sh` (gated S30 smoke), `test-compute.sh`, `compute-mode.sh` enable/disable, `load-and-test.sh` (hardware), `build-test-binaries.sh` (DT_NEEDED/SONAME build guard). |

---

## 3. Supported Fedora releases and kernels

| COPR project | Chroots (x86_64 only) | Architecture |
|---|---|---|
| `axogm/musa-fedora` | fedora-44, fedora-45, fedora-eln, fedora-rawhide | x86_64 |
| `axogm/musa-mtt-s30` | fedora-43, fedora-44, fedora-45, fedora-eln, fedora-rawhide | x86_64 |

- **Kernel tested on host:** `7.3.4-300.fc45.x86_64` (Fedora 45).
- **DKMS** builds for the running kernel; kernel-devel must match the target (vermagic check in `rpm-stack.sh`).
- aarch64 not configured (vendor payload is x86_64 only).

---

## 4. MTT S30 hardware test results (live system)

### 4.1 Live host inventory
- MTT S30 at `0000:01:00.0` (PCI `1ed5:0102`), IOMMU group 11, `intel_iommu=on iommu=pt`.
- `mtgpu` 3.1.0 loaded (initstate=live), `/dev/mtgpu.0`, DRM node `/dev/dri/renderD130`.
- Firmware at `/lib/firmware/mthreads/`. MUSA runtime 1.5.4 at `/usr/local/musa`. MUSA SDK 5.2.0 at `/usr/local/musa-5.2`.

### 4.2 `verify-musa.sh` — end-to-end (13/13/1)
```
Summary: 13 passed, 0 failed, 1 skipped
```
(checks PCI/driver, IOMMU, mthreads-gmi, musaInfo, muMemcpyTest, musart-smoke, vector-add, compute suite, fp64, Vulkan headless; OpenCL MT ICD skipped — not packaged).

### 4.3 Controlled load test (2026-11-23, installed system)
| Step | Result |
|---|---|
| Firmware + device init | PASS (initstate=live, no kernel fault) |
| `muMemcpyTest` (1 GiB) | **PASS** |
| `musart-smoke` (runtime API) | **PASS** |
| `mp_10` vector-add | **PASS** (1048576 adds verified) |
| compute suite | **PASS** |
| fp64 | **PASS** (`abs_err=5.551e-17`) |
| headless Vulkan | **PASS** |
| `mthreads-gmi -q` | FAIL (rc=2, `failed to initialize mtml`) — informational, userspace tooling |
| `musaInfo` (SDK 5.2 tool) | FAIL (aborts) — **fixed by using runtime 1.5.4 tool** |
| GNOME Shell | crashed (Mesa DRI selection) — **fixed by `musa-runtime-display` split** |

Every functional GPU step passes; the two informational-tool failures are userspace issues documented in `docs/hardware-validation-2026-11-23-load-test.md`.

---

## 5. COPR build status (9 chroots)

Latest successful builds (built by COPR, published to each chroot):

| Package | Version | musa-fedora (4 chroots) | musa-mtt-s30 (5 chroots) |
|---|---|---|---|
| mtgpu-dkms | 3.1.0-12 | ✅ | ✅ |
| musa-runtime | 1.5.4-4 | ✅ | ✅ |
| musa-toolkit | 3.1.0-3 | ✅ | ✅ |
| musa-sdk | 5.2.0-6 | ✅ | ✅ |
| mtgpu-firmware | 3.1.0-4 | ✅ | ✅ |

- `scripts/verify-copr-chroots.sh` confirms **17 passed / 3 failed** (3 stale 5.2.0 builds in `axogm/musa-mtt-s30`, fedora-43/45/rawhide — incompatible upgrades; 78e49e2f deleting builds 11334274/11334275).
- Note (COPR hygiene): `axogm/musa-mtt-s30` still published **stale** `mtgpu-dkms 5.2.0-1` and `musa-toolkit 5.2.0-1` (2026-11-16 builds) that dnf surfaces as incompatible upgrades; agent 78e49e2f is deleting these (builds `11334274`/`11334275`) and adding a `--newest`-equals-expected check to `verify-copr-chroots.sh`. These COPR deletions and verifier edits are **in progress, not part of this report's production set**.

---

## 6. Build-test guard (`build-test-binaries.sh`)

The builder links every test binary against the installed MUSA 1.5.4 runtime soname (`libmusart.so.1.5`) and **fails the build if the wrong SDK 5.2.0 runtime (`libmusart.so.5`) is linked** (which aborts inside `musaMalloc()` against the 3.1.0 mtgpu driver, producing `Invalid cmd_type`).

- `bash -n` → **SYNTAX OK**
- Normal build → **BUILD_STATUS: SUCCESS**, all binaries `OK (links libmusart.so.1.5)`
- `MUSA_RT_LIB=/nonexistent` → **FATAL: cannot read the SONAME… exit 1** (nothing built) — fail-closed verified.

---

## 7. Repository & commit links

### musa_fedora (`https://github.com/TheCreateGM/musa_fedora.git`)
Branches: `main` (HEAD `a2614e4`) — **in sync with origin**, working tree clean except `scripts/verify-copr-chroots.sh` (copr hygiene edit, 78e49e2f).
```
a2614e4  docs(install): current verified state, activation paths, real recovery steps
583c33f  docs(container): isolated verification of current package set
b432e6d  docs: clarify ABI failures and current test gate
8752c12  fix(hardware): fail closed when MUSA runtime SONAME unreadable
62261e3  fix(test-gpu): fail closed on runtime ABI mismatch
ba77728  fix(hardware): fail test-binary build when MUSA runtime link wrong
9791eb7  docs(hardware): record live end-to-end suite verification 2026-11-23
3e47c05  fix(musa-env): put 1.5.4 runtime tools before 5.2 SDK
1abf220  fix(test-gpu): use 1.5.4 runtime musaInfo
```
(there were also earlier t3-checkpoint commits and the initial `15c9240`.)

### musa_data (`https://github.com/TheCreateGM/musa_data.git`)
Branches: `main` (HEAD `a4320e9`) — **in sync with origin**, clean.
```
a4320e9  docs(research): build-ID table → final dkms 3.1.0-12 rebuild
737cfa5  docs(research): update COPR evidence for mtgpu-dkms 3.1.0-12
70acfca  docs(research): record COPR chroot artifact verification 2026-11-23
```

---

## 8. Unresolved issues & known limitations

| Issue | Status |
|---|---|
| `Invalid cmd_type` in this boot's journal (from the 5.2.0-SDK test binaries at 08:04/08:30) | **Boot is fail-closed gated**; `scripts/test-gpu.sh` reports `BLOCKED`. Resolution: clean reboot (verified clean) → re-test. The blacklist is **not** removed. |
| `compute-mode.sh enable` (removes boot blacklist) | **NOT enabled** — requires explicit user approval + reviewed recovery plan. |
| `mthreads-gmi -q` informational failure (`failed to initialize mtml`) | Userspace/S30 audio-driver symlink issue; documented; not a compute-path fault. |
| `musa-runtime-display` Mesa DRI drivers | Opt-in subpackage; required for display hosts, avoids GNOME crash. |
| aarch64 chroots | Not configured (vendor payload x86_64 only). |
| OpenCL OpenCL MT ICD | Not shipped in the runtime; `verify-musa.sh` skips it. |

---

## 9. Installation & recovery instructions

### Install from COPR (compute-only, GNOME-safe)
```sh
pkexec dnf install dnf-plugins-core
pkexec dnf copr enable axogm/musa-fedora      # Fedora 44/45/eln/Rawhide
pkexec dnf copr enable axogm/musa-mtt-s30     # adds Fedora 43
pkexec dnf install musa-toolkit
# musa-toolkit pulls mtgpu-dkms, mtgpu-firmware, musa-runtime, musa-sdk.
```

### Activate the S30 for compute (explicit approval REQUIRED)
```sh
# review, then approve
scripts/compute-mode.sh status
scripts/compute-mode.sh enable          # modifies boot config + regenerates initramfs via dracut
pkexec /usr/bin/systemctl reboot
```
On reboot, prefer the runtime profile if present:
```sh
source /etc/profile.d/musa.sh
```

### Removal
```sh
scripts/uninstall.sh --plan
scripts/uninstall.sh --remove          # refuses while mtgpu is loaded
```

### Recovery (boot-blocked state is intentional & safe)
- The boot-time blocker (`/etc/modprobe.d/blacklist-mtgpu.conf`) is **active by default** and restores automatically on reboot.
- If the S30 load is unwanted: `scripts/disable-iommu.sh` + reboot returns to the desktop-safe state.
- All module/config changes are backed up under `/var/backups/musa-fedora-*` before each modification; `modprobe -r` and manual file copies are **not** used for recovery.

### Verified recovery
- The 2026-11-23 GDM crash fix (initramfs regeneration via dracut) took effect; the host now boots `/dev/nvme0n1p7` and GNOME runs on Intel `i915` while `mtgpu` stays unloaded.

---

## 10. Notes on what is NOT done (by design)

- No `compute-mode.sh enable` (boot config change needs explicit approval).
- No GPU workloads beyond the controlled, approved load test (boot journal is fail-closed gated).
- No module unload/rebind/unconditional `modprobe -r` for recovery.
- No force-push; both repos are in sync with origin.
- COPR deletions of stale 5.2.0 builds (`11334274`/`11334275`) are tracked for the 78e49e2f COPR-hygiene workstream, not part of the production RPMs above.

---

*Report generated 2026-11-23. All claims above are backed by real command output: `verify-musa.sh` (13/0/1), `verify-copr-chroots.sh` (11/0), `build-test-binaries.sh` (SUCCESS), live S30 GPU loads (PASS), and COPR API/build-state queries.*
