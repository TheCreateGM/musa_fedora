# Hardware validation — MTT S30, 2026-11-25

Host: Fedora Linux 45, kernel `7.3.4-300.fc45.x86_64`, 7.4 GiB RAM, 20 CPUs.
Boot started 2026-11-24 23:59; `mtgpu` loaded by udev with no blacklist
(compute mode, see [compute-mode-enabled-2026-11-24](compute-mode-enabled-2026-11-24.md)).

## Driver init (journal, this boot)

```
mtgpu 0000:01:00.0: MTGPU Firmware image 'mthreads/mtfw-gen1.bin' loaded
mtgpu 0000:01:00.0: mtgpu watchdog enabled
[drm] Initialized mtgpu 2.0.0 for 0000:01:00.0 on minor 0
```

No `BUG`, `Oops`, general-protection, or invalid-opcode entry was logged in
this boot. The `irq:NNN affinity has changed` messages are the known noise
documented in the IRQ affinity notes.

## Verification run

Command (run in a cgroup with a 3 GiB hard cap and no swap):

```sh
systemd-run --user --scope -p MemoryMax=3G -p MemorySwapMax=0 -p TasksMax=256 \
    scripts/verify-musa.sh --skip-memcpy
```

Result: **13 passed, 0 failed, 2 skipped** (exit 0).

* PASS PCI binding (`1ed5:0102` on `mtgpu`), `/dev/mtgpu.0`, render node `renderD129`
* PASS IOMMU groups (13), `libdrm_mtgpu.so.1` (packaged 3.1.0)
* PASS `mthreads-gmi` reports the MTT S30 (driver 3.1.0)
* PASS `musaInfo` (runtime 1.5.4)
* PASS `musart-smoke` on the GPU (d2h memset, async round trip)
* PASS `mcc` builds `mp_10` vector-add; 1,048,576 float adds verified on the S30
* PASS compute suite: saxpy, shared reduce, atomics, constant memory, device math, 2D grid, D2D copy, FP64
* PASS Vulkan device enumeration and host-visible buffer map
* SKIP `muMemcpyTest` (skipped by `--skip-memcpy`; see finding 1)
* SKIP Moore Threads OpenCL (no MT ICD installed)

## Findings

1. **`muMemcpyTest` needs more than 2 GiB of host RSS.** A first run under a
   2 GiB cap was OOM-killed inside its cgroup (`oom-kill` on `muMemcpyTest`,
   anon-rss ≈ 2.0 GiB), and the wrapper did not complete. The test is not
   run in the 3 GiB verification above. Run it only when at least ~4 GiB is
   free, in a capped scope, and record the result as a separate log.
2. **Parallel builds exhausted host RAM.** On 2026-11-25 02:28 many parallel
   `cc1` compiler processes triggered a global OOM. The kernel killed
   `deepai-code` and a browser content process; `mtgpu` was not involved.
   Build and test jobs on this host should run under a memory cap
   (`systemd-run --user --scope -p MemoryMax=… `) or with a bounded `-j`.
3. Git: both `musa-fedora` and `musa-data` are up to date with origin, and
   `git push --dry-run` succeeds with the current GitHub login.
4. COPR: `copr-cli` is authenticated and lists the `axogm` projects. The chroot
   state from the 2026-11-23 verification is unchanged in this session.
