# Session validation — 2026-11-23 (Fedora 45 Live USB)

This report records **only checks actually run in this session**. It does not
repeat historical 2026-11-17 GPU results as current results. No driver was
loaded, no GPU workload was run, and no system package or boot setting was
replaced on the installed Fedora system.

## Environment measured at the start

- Session: Fedora Linux 45 Workstation **Live USB** (`LiveOS_rootfs` overlay),
  kernel `7.2.11-300.fc45.x86_64`, user `liveuser` (wheel, `pkexec` available).
- Persistent home: `/run/media/liveuser/fedora/home/axogm`, mapped to the
  canonical project path `/home/axogm` with a symlink in the live overlay.
  A read-only reference copy of the installed Fedora root is available under
  `/run/media/liveuser/fedora/root` (contains the previously installed
  `mtgpu-dkms-3.1.0-11`, `mtgpu-firmware-3.1.0-2`, `musa-runtime-1.5.4-3`,
  `musa-sdk-5.2.0-6`, `musa-toolkit-3.1.0-1` and DKMS modules for kernels
  7.2.15, 7.3.3 and 7.3.4).
- GPU: Intel UHD 770 at `0000:00:02.0` bound to `i915` (desktop console) and
  Moore Threads MTT S30 at `0000:01:00.0` (`1ed5:0102`) plus audio
  `0000:01:00.1` (`1ed5:01ff`), **both unbound**. The only DRM render node,
  `renderD128`, belongs to Intel; there are no MTT device nodes.
- Secure Boot: disabled. `journalctl`/`journalctl -k` were reviewed; no current
  boot `mtgpu` probe or Oops lines. `dmesg` is restricted to root in this
  session, so the kernel journal was used instead.
- No MUSA user-space is installed in the **live** environment; the 1.5.4/5.2.0
  userspace exists only in the installed-system reference tree.

## Privilege/build-tool incident and recovery (disclosed)

Installing build tools with `pkexec dnf` filled the 1.5 GB Live USB overlay
(6 MB free), and that transaction failed while unpacking `gcc-c++` and `gh`.
DNF rolled the transaction back. I immediately removed the two optional
packages just added (`kernel-devel` 238 MB, `rpmdevtools`), after which
`dnf check` reported a clean RPM database and 280 MB of overlay space was
free. `gh` was then installed alone successfully (40 MB). No critical package
was left broken and the installed Fedora system was not touched.

Consequence: local DKMS **binary** builds cannot run in this boot because
`kernel-devel` is absent; SRPM builds work, and COPR chroots provide the
kernel headers for the authoritative binary builds.

## Real checks and results

| Check | Result |
|---|---|
| `rpmspec -P` on all five specs | PASS |
| Read-only hardware check `scripts/test/check-hardware.sh` | PCI identity PASS; `DRIVER_STATUS: unbound`; `GPU_TEST_STATUS: NOT RUN`; exit 3 (expected) |
| Gated validation `scripts/test/s30-hardware-validation.sh` (no `--load`) | `VALIDATION_STATUS: BLOCKED (setup incomplete)`; MUSA tools absent in live boot; exit 2. No load attempted. |
| Local SRPM build (`build-srpms.sh`) | **SUCCESS**: five SRPMs, `rpm -K --nosignature` digests OK |
| Vendor core integrity in the new DKMS SRPM | PASS: extracted `mtgpu_core.o_binary` SHA-256 `d0fdaceab37bce7387d376f8a87a2e9dbf210eeccf4f4f7c67f8dee83f4e8200`, byte-identical to the pinned hash required by the spec |
| Local `mtgpu-firmware` binary RPM build | PASS (`mtgpu-firmware-3.1.0-4.fc45.x86_64.rpm`) |
| Local `mtgpu-dkms` binary RPM build | Not run in this boot: real build dependency `kernel-devel` unavailable after overlay recovery; covered by COPR chroots |
| Upstream catalog / driver support matrix recheck | Confirmed: no public MUSA 5.2.0 S30 package; no Intel + Fedora + S30 support row. See `musa-data/research/upstream-verification-2026-11-23.md`. |

## COPR submissions

Both configured projects were rebuilt from the newly built SRPMs, covering
every configured x86_64 chroot: `axogm/musa-fedora` (Fedora 44/45/ELN/Rawhide)
and `axogm/musa-mtt-s30` (Fedora 43/44/45/ELN/Rawhide).

| Package | `axogm/musa-fedora` build | `axogm/musa-mtt-s30` build |
|---|---|---|
| `mtgpu-firmware` 3.1.0-4 | 11377871 | 11377877 |
| `mtgpu-dkms` 3.1.0-11 | 11377872 | 11377878 |
| `musa-runtime` 1.5.4-3 | 11377874 | 11377879 |
| `musa-sdk` 5.2.0-6 | 11377875 | 11377881 |
| `musa-toolkit` 3.1.0-3 | 11377876 | 11377882 |

Final per-build and per-chroot results are recorded below after the builds and
log inspection complete.

## Publishing state

- COPR authentication works with the existing token (`copr-cli whoami` →
  `axogm`); builds were submitted with `--config /home/axogm/.config/copr`.
- GitHub authentication **does not work**: `gh auth status` reports HTTP 401
  "Bad credentials" for the stored token, and `git ls-remote` cannot
  authenticate. Commits are being prepared locally; pushing to
  `TheCreateGM/musa_fedora` and `TheCreateGM/musa_data` requires the user to
  supply a valid GitHub token or re-run `gh auth login`. No credential was
  invented or bypassed.

## Safety boundary

The S30 remains unbound and blacklisted. The existing
`docs/maintenance-plan-mtgpu-load-2026-11-20.md` is explicitly **DRAFT — NOT
APPROVED**; nothing in this session authorizes loading `mtgpu`, disabling IBT,
rebinding the PCI device, or running GPU workloads. Build success in COPR is
not evidence that the module is safe to load or that MUSA works on this boot.
