# Live baseline — 2026-11-27 session 5

Read-only inventory and non-device checks from the live Fedora 45 host. This
record supersedes older host-state tables that describe the S30 as unbound;
it does **not** supersede historical workload results or claim tests were run
in this session.

## System and device

| Item | Observed |
|---|---|
| Host | Dell OptiPlex Tower 7010; Intel Core i5-13500, 20 logical CPUs, 7.4 GiB RAM |
| OS/kernel | Fedora Linux 45 Workstation, x86_64; `7.3.4-300.fc45.x86_64` |
| Target GPU | Moore Threads MTT S30, PCI `1ed5:0102`, BDF `0000:01:00.0`; 4 GiB BAR2; IOMMU group 11 |
| Other graphics | Intel UHD 770 `8086:4680` drives the desktop using `i915` |
| Active MTT stack | `mtgpu` 3.1.0 DKMS module is loaded (`initstate=live`) and bound to S30; `mtsnd` is loaded for audio |
| Device nodes | `/dev/mtgpu.0`; S30 DRM `card0` and `renderD129`; virtual MVGAL is a separate `card2`/`renderD130` and must not be mistaken for the S30 |
| Firmware/packages | Firmware `mtgpu-firmware 3.1.0-5`; `musa-runtime 1.5.4-8`; `musa-sdk 5.2.0-7`; `musa-toolkit 3.1.0-6`; `musa-cuda-compat 1.0.0-2` |
| Security | SELinux enforcing; Secure Boot disabled; IOMMU enabled (`intel_iommu=on iommu=pt`); no IBT-disabling boot argument |
| Resources | Root/home btrfs 442 GiB total, 240 GiB available; 7.4 GiB RAM with 2.6 GiB available at inventory time |

The GPU driver and packages were already installed before this session. No
host package, kernel, initramfs, firmware, module, service, or boot setting was
changed.

## Current-boot diagnostic findings

- Kernel log: the `mtgpu` module initialized and loaded firmware
  `mthreads/mtfw-gen1.bin`. At boot it logged failure to create
  `/var/log/mtgpu/event_report` and its per-device directory (`-ENOENT`), then
  `xid_record file create fd failed`. Existing project analysis attributes
  this to driver initialization in the initramfs before `/var/log` is
  available; the event-report failure is not itself evidence of failed GPU
  initialization. See [XID/initramfs analysis](root-cause-mtgpu-xid-initramfs-2026-11-27.md).
- Kernel log has no `BUG`, Oops, general-protection fault, invalid-opcode,
  `mtgpu Invalid cmd_type`, or GPU XID/fault in this boot. The `xid_record`
  message above is the exception and is separately explained.
- Two Ollama child `llama-server` processes dumped core at 06:55 UTC. The
  backtrace is `libggml-cuda.so` -> `cudaGetDeviceCount` in Ollama's bundled
  CUDA 13 runtime -> system `libcuda.so.1` -> `muGetExportTable` in
  `libmusa.so.1.5.4`, where execution faulted. This is an unsupported
  CUDA-13-vs-MUSA-1.5.4 compatibility path in Ollama GPU discovery, not an
  `mtgpu` kernel fault. Ollama subsequently discovered the S30 via its Vulkan
  backend; this does not establish successful CUDA/MUSA inference. No attempt
  was made to repeat the crash or alter Ollama's service.
- The separate project application `apps/musa-infer` is a custom MUSA-kernel
  GGUF program linked to the project CUDA-compat shim. Its previously recorded
  TinyLlama and compare results (see
  [hardware validation](hardware-validation-2026-11-27-reverify.md)) are prior
  test evidence; they were **not** repeated in this session and are distinct
  from Ollama's CUDA backend.
- The system MUSA profile check passes: `musaInfo` resolves to runtime 1.5.4,
  `mcc` resolves to SDK 5.2.0, and login `LD_LIBRARY_PATH` orders runtime 1.5.4
  before SDK 5.2. The coding-agent launcher adds its own temporary mount
  library directory to its process environment; this is not the environment
  configured for Ollama's system service.
- Running SDK 5.2 `musaInfo` directly reports “API call is not supported”
  against the installed 3.1.0 driver. Do not interpret that as failure of the
  supported 1.5.4 runtime, nor substitute the 5.2 runtime for 1.5.4.

## Checks performed this session

| Check | Result |
|---|---|
| Read-only system/PCI/package/module/repository inventory | Complete |
| `scripts/snapshot-host-state.sh --label pre-musa-project-task` | Complete; local snapshot at `../../musa-data/host-backups/pre-pre-musa-project-task-20261127-073828` (kept out of Git) |
| `scripts/check-musa-env.sh` | PASS |
| `scripts/verify-copr-chroots.sh` | **24 passed, 0 failed**; Fedora 43/44/45, ELN, Rawhide x86_64 in both projects |
| `scripts/test-toolchain.sh` | PASS compile-only; compiler emitted fatbin and explicit `mp_10` target; no executable run |
| CUDA source strict conversion and reverse round-trip | PASS, byte-identical; no device code run |
| `bash -n` on project/package shell scripts | PASS |
| `rpmspec -P` for all RPM specs | PASS for every spec; the `mtgpu-dkms` changelog weekday was corrected to `Fri Nov 27 2026` and re-parsed without warnings |
| SHA/digest checks for eight local Fedora 45 RPMs | PASS |
| `scripts/test-gpu.sh` | **NOT RUN** (exit 3): explicit `MUSA_S30_TEST_APPROVED=1` gate is unset |
| GPU compute, LLM, VA-API, CUDA-compat hardware suites | Not run this session |

No isolated VM image/domain is configured. QEMU/KVM is installed and `/dev/kvm`
is present, but no VM image exists; the GPU is already bound to the host and
there is no tested S30 passthrough guest. Per the task's VM-before-hardware
requirement, this session did not set the hardware-test approval variable or
run a GPU workload. A generic Fedora VM without the S30 passed through could
validate package installation but could not validate GPU behavior.

## Repository/authentication state

- Production checkout: `/home/axogm/Documents/project/musa-fedora`, branch
  `main`, clean at `4196eb2`; remote `origin/main` matched at inventory time.
- Reference checkout: `/home/axogm/Documents/project/musa-data`, branch
  `main`, clean at `02ee83b`; remote `origin/main` matched at inventory time.
- GitHub CLI authenticated as `nnsupermax`; COPR CLI authenticated as `axogm`.
- Git reported existing dangling objects and temporary/garbage object files in
  the large production repository object store. No repair, prune, or cleanup
  was attempted because the branch and remote revisions agree and the working
  tree is clean.

## Scope and remaining items

This baseline verifies installed software, PCI binding, build/source checks,
and COPR publication only. It does not newly validate device discovery via the
1.5.4 runtime, memory allocation, kernel execution, inference, performance,
reboot recovery, or VM package installation. The S30 is operating on Fedora 45
but remains outside the public vendor-qualified OS/platform matrix. Ollama's
CUDA 13 path and the MUSA `cuGetExportTable` shim behavior require a separately
reviewed compatibility investigation before any attempted fix or service
change. See the more detailed research note in
[reference research note](../../musa-data/research/live-baseline-2026-11-27-session5.md).
