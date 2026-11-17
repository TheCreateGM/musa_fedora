# MTT S30 hardware validation status

**No hardware bring-up procedure is approved.** The `mtgpu` build contains an
opaque vendor core object with unresolved objtool/ENDBR warnings. Do not
disable IBT/CET, load the full driver, open the binder's control node, or run
the runtime against it.

The earlier proposed `ibt=off` and module-load procedure was unsafe and has
been removed. The scripts with those names now refuse to modify boot settings
or load modules. The revert helper only removes a persistent `ibt=off`
argument from the explicitly selected kernel entry, after verifying the entry
and rechecking the result.

## Latest host state (2026-11-13)

See the [live host recheck](live-host-recheck-2026-11-13.md). The host runs
Fedora 45 kernel `7.3.3-300.fc45.x86_64`; the S30 is present but unbound, and
Intel UHD 770 (`i915`) is the active display GPU. A boot-loaded `mtgpu`
module faulted under kernel IBT at `rmapi_alloc` and remains in
`initstate=coming`. An unowned module file with a different hash from the
quarantined file is currently present in the tree; its origin is unknown.
`/dev/mtgpu.0` and an S30 render node are absent.

A fresh S30 3.1.0 build for 7.3.3 linked but emitted 5,504
compiler/objtool warning lines. The staged official
MUSA compiler passes compile-only checks. The gated GPU script blocks before
runtime access. No VM passthrough or physical GPU execution test has been
performed. QEMU/KVM is installed, but the S30 has no IOMMU group in this boot,
so safe PCI passthrough is not available without a separate host
configuration change.

See [driver loading](driver-loading.md), [driver porting](driver-porting.md),
and [IBT recovery status](ibt-off-recovery.md). The only approved checks are
read-only inventory and compile-only build validation.
