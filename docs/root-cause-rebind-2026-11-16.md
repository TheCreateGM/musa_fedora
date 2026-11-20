# Rebind failure root cause — 2026-11-16 (current boot)

> Dated root-cause record (2026-11-16). The failure it describes was observed in
> an earlier boot. The present host has the S30 unbound and `mtgpu` blacklisted
> after prior display/driver faults (see the
> [2026-11-20 live state](current-state-2026-11-20.md)). The finding remains a
> live safety caveat: do not unbind/rebind the device.

## Symptom
- `verify-musa.sh`: 5 PASS / 5 FAIL. PCI + `mtgpu` bound + `/dev/mtgpu.0` + IOMMU OK;
  no mtgpu DRM render node, `mthreads-gmi` fails, `musaInfo`/`muMemcpyTest` fail
  (`invalid argument`), vector-add runs fail (`CreatePlatform failed!`).
- Kernel: repeated `[MISC][ERROR][mtgpu_is_driver_ready][578] mtgpu driver is not ready.`

## Cause (verified from boot journal, boot 0)
- 11:23 UTC (first probe): clean. `Initialized mtgpu 2.0.0 ... on minor 0`,
  `fb0: mtgpudrmfb`. No oops.
- 13:17 UTC: manual `pkexec ... echo 0000:01:00.0 > .../drivers/mtgpu/unbind`
  (from `musa-fedora`). Unbind itself completed cleanly (all components unbound).
- 13:19 UTC: rebind/probe #2 (`[drm] loading mtgpu card: 2`) oopsed:
  `mtgpu_pstate_notifier_call_chain+0x62`, CR2=0x10, RAX=0 —
  `mov rdi,[rax+0x10]` on a NULL entry. Call path:
  `vpu_component_bind -> vpu_report_power_state -> mtgpu_pstate_notifier_call_chain`.
  Probe aborted before DRM init; ready flag never set. Same oops recurred at 13:35
  (audio path) — consistent with a stale pstate-notifier list entry that unbind
  did not unregister (implementation is in the closed vendor `mtgpu_core.o`).
- `mtgpu_is_driver_ready` / pstate implementation: closed vendor core only;
  open `src/` + `driver-mtgpu-5.2.0/src` contain call sites only. No source fix possible.

## Recovery
- **Reboot** (fresh probe is the only known-good path). Do NOT unbind/rebind `mtgpu`
  again — it deterministically re-enters this state on this driver build.
- Reboot requires user approval (system-level change) via `pkexec`-style auth.

## Packaging note
- `packaging/rpm/SPECS/*` target MUSA **5.2.0**, which does not drive the S30 on
  this host; the working stack is mtgpu **3.1.0** + runtime **1.5.4**. Publishing the
  5.2.0 specs to Copr as a functional stack would be misleading. Copr publication
  is blocked until specs are re-pointed at the verified 3.1.0/1.5.4 stack or the
  5.2.0 stack is proven on S30. `copr-cli` is authenticated as `axogm`.
