# Kernel IBT status and recovery

> Historical status record. The 2026-11-13 "latest observed state" below is
> superseded by the [2026-11-20 current state](current-state-2026-11-20.md):
> `mtgpu` is not loaded and the S30 is unbound. The recovery procedure and the
> "do not disable IBT/CET" guidance remain valid. Retained as a dated record.

## Latest observed state (2026-11-13)

The host now runs `7.3.3-300.fc45.x86_64` with no `ibt=off` or `noibt` on
`/proc/cmdline`; the kernel IBT configuration is enabled. During this boot,
the unowned `mtgpu.ko` triggered a `Missing ENDBR: rmapi_alloc` control-
protection fault. That module remains stuck in `initstate=coming`. An
unowned module file is currently present in the tree, with a different hash
from the file quarantined on 2026-11-12; its origin is unknown. The S30 is
unbound. See the [live host report](live-host-recheck-2026-11-13.md).

Earlier on 2026-11-12, a separate 7.2.15 boot had `ibt=off` on its active
command line. The BLS entries and GRUB environment were backed up to
`/home/axogm/Documents/project/musa-data/host-backups/boot-config-2026-11-12.tar`.
`pkexec grubby --update-kernel=ALL --remove-args=ibt=off` was run and verified
with `pkexec grubby --info=ALL`; no persistent entry retained `ibt=off`.
That earlier 7.2.15 runtime state is historical and is not the current boot.

Earlier project notes incorrectly stated that no persistent entry contained
`ibt=off`. That was corrected by the boot-entry update above. The present
kernel boot confirms IBT is active; the MTT driver still fails its IBT check.

## Reversible correction procedure (performed)

The backup and boot-entry update below were performed. The archive was
verified with `tar -tf` before changing boot entries:

```bash
mkdir -p /home/axogm/Documents/project/musa-data/host-backups
pkexec tar -C /boot -cpf \
  /home/axogm/Documents/project/musa-data/host-backups/boot-config-2026-11-12.tar \
  loader/entries grub2/grubenv
```

```bash
pkexec grubby --update-kernel=ALL --remove-args=ibt=off
pkexec grubby --info=ALL
```

This changes boot entries but does not change the currently running kernel.
The correction was performed earlier; the current kernel already boots with
IBT active. Do not use `ibt=off` as a workaround for the MTT module fault.

If the boot-entry change must be rolled back, restore the saved entries and
GRUB environment:

```bash
pkexec tar -C / -xpf \
  /home/axogm/Documents/project/musa-data/host-backups/boot-config-2026-11-12.tar
pkexec grubby --info=ALL
```

Do not load MTT modules or run GPU tests before resolving the vendor-core
IBT/ENDBR issue. Restoring kernel IBT is a security correction, not evidence
that the MTT driver is compatible.
