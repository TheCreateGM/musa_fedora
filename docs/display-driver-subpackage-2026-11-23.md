# Display-driver subpackage — 2026-11-23

## Problem

The 2026-11-23 controlled load test (`docs/hardware-validation-2026-11-23-load-test.md`)
confirmed that when `mtgpu` registers its DRM card, Mesa loads the vendor MUSA
DRI drivers from `/usr/lib64/dri/` for the S30 and GNOME Shell/mutter segfaults
in its KMS thread (`libmutter-cogl-51.so`, `libsrv_um_MUSA.so` in the coredump).
GDM then gives up after repeated display failures and no login screen appears.

Until now the only mitigation was the boot-time `blacklist mtgpu`, which leaves
the S30 unusable for compute on a normal boot.

## Change

`packaging/rpm/SPECS/musa-runtime.spec` is bumped to `1.5.4-4` and the vendor
graphics drivers are moved from the base package into a new optional
subpackage:

- `musa-runtime` — compute runtime, headers, `libdrm_mtgpu`, Vulkan ICD,
  `mthreads-gmi`, linker/sysctl/modemprobe config. **No DRM/GBM/Xorg drivers.**
- `musa-runtime-display` — `/usr/lib64/dri/`, `/usr/lib64/gbm/`,
  `/usr/lib64/xorg/modules/drivers/`. `Requires: musa-runtime = 1.5.4-4`.

`musa-runtime-display` is **not** pulled in by `musa-toolkit` or by the base
package's `Recommends`. A compute-only install therefore cannot trigger the
Mesa/GNOME crash when the driver is deliberately loaded with headless options.
Install the subpackage only when the S30's own DP/HDMI outputs are required.

`scripts/rpm-stack.sh` now expects `musa-runtime` `1.5.4-4`.

## COPR rebuild

The same SRPM was submitted to both configured projects, covering every
enabled x86_64 chroot:

| Project | Chroots | Build ID |
|---|---|---|
| `axogm/musa-fedora` | fedora-44, fedora-45, fedora-eln, fedora-rawhide | 11377974 |
| `axogm/musa-mtt-s30` | fedora-43, fedora-44, fedora-45, fedora-eln, fedora-rawhide | 11377976 |

Both builds were watched to completion and reported `succeeded`. The per-chroot
`results.json` files were fetched from the COPR download server and every
chroot lists both `musa-runtime` and `musa-runtime-display`:

| Chroot | 11377974 (`musa-fedora`) | 11377976 (`musa-mtt-s30`) |
|---|---|---|
| fedora-43-x86_64 | — | succeeded, both RPMs |
| fedora-44-x86_64 | succeeded, both RPMs | succeeded, both RPMs |
| fedora-45-x86_64 | succeeded, both RPMs | succeeded, both RPMs |
| fedora-eln-x86_64 | succeeded, both RPMs | succeeded, both RPMs |
| fedora-rawhide-x86_64 | succeeded, both RPMs | succeeded, both RPMs |

A successful build only proves that the RPMs are produced; it does not prove
that the driver is safe to load or that the desktop survives a load.

## Verification still required

- Confirm every chroot of both builds succeeds and the expected
  `musa-runtime-display` RPM appears in the artifacts.
- On the installed Fedora 45 system (booted from disk, kernel 7.3.4), replace
  the installed `musa-runtime` 1.5.4-3 with 1.5.4-4 **without** installing the
  display subpackage, then repeat the controlled load test with
  `scripts/hardware/load-and-test.sh` (corrected to use the 1.5.4 `musaInfo`).
- Confirm GNOME Shell stays alive during that load. A reboot restores the
  blacklisted, display-only state regardless of the outcome.
