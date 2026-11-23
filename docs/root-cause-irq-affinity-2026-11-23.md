# Root cause: repeated "mtgpu … irq:N affinity has changed" kernel messages

Date: 2026-11-23, controlled-test boot (mtgpu loaded, driver live).

## Observation

`journalctl -k -b 0 | grep -c 'affinity has changed'` → **471 messages**, all
of the form:

```
kernel: mtgpu 0000:01:00.0: irq:175 affinity has changed.
```

Distribution over the boot:

| IRQ | messages | role (per `/proc/interrupts`) |
|---|---|---|
| 175 | 423 | `IR-PCI-MSI-0000:01:00.0 1-edge mtgpu` (the actively-counted vector) |
| 178 | 36 | `4-edge mtgpu` (also counted) |
| 174,176,177,179,180,181 | 1 each | logged once at load (07:39:02) then idle |

The two busy vectors (175, 178) are the only ones with non-zero interrupt
counts in `/proc/interrupts`; their counts are spread across all 20 CPUs, and
`/proc/irq/*/smp_affinity_list` shows single-CPU masks that move over time
(e.g. 175 → CPU 5, 178 → CPU 11 at inspection time).

## Root cause

`irqbalance` is `active` (Fedora default, enabled preset). It re-evaluates
unpinned IRQs every ~10 s; each migration it applies to mtgpu's MSI vectors
makes the kernel core call the driver's affinity-change callback, and the
vendor `mtgpu` module `pr_info`s one line per change. The cadence of the
messages (10/20/60 s groups) matches irqbalance's intervals, and the churn is
concentrated on exactly the vectors irqbalance is allowed to move and that
actually receive traffic.

This is **not** a driver fault, not an interrupt storm, and not a regression:
the interrupts are being balanced across CPUs as designed, the hardware
counts advance normally, and no `Oops`, `BUG`, or NMI error accompanies the
messages. It is vendor log noise (wrong level for a routine event; the
source is the opaque vendor core, so we cannot change the level).

## Impact assessment

* Log volume only (~470 lines per long-lived loaded-driver boot).
* No effect on MUSA workload results; the 2026-11-23 load tests passed with
  these messages present.
* `rpm -V`/hardware state unaffected.

## Options (none applied — host behavior unchanged)

1. Accept the noise (current posture). The messages stop when the driver is
   unloaded/blacklisted on a normal boot.
2. Pin mtgpu's vectors away from irqbalance without banning them (keeping
   manual placement): a systemd drop-in setting
   `IRQBALANCE_BANNED_HINTS_FILE`/ban for the device, or
   `echo cpulist > /proc/irq/N/smp_affinity` after load. This suppresses the
   churn but changes interrupt placement, so it must not be tried without a
   reviewed plan on a host that intentionally loads the driver.
3. Filter journald kernel messages by pattern (cosmetic, hides a health
   signal; not recommended).

No action is required for the project's supported configuration (S30 compute
under the controlled-load procedure). Re-check this page if message volume
grows by orders of magnitude or non-175/178 vectors start churning during
idle — that would indicate a real interrupt storm instead.
