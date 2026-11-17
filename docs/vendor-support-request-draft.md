# Moore Threads S30 driver support request (draft)

This is a local draft only. No message has been sent.

## Subject

MTT S30 on x86_64 Fedora: IBT-safe driver core, MUSA target, and supported SDK

## Request

We are developing a reproducible MUSA environment for an MTT S30 (`1ed5:0102`)
on Fedora Linux 45 x86_64 with kernel `7.3.3-300.fc45.x86_64`. The machine uses
an Intel UHD 770 for display; the S30 is secondary and currently unbound.

The official MUSA 5.2.0 DKMS source recognizes S30 PCI ID `0x0102` and routes
it through `sudi_drvdata`. The active project S30 3.1.0 source builds against
Fedora kernel 7.3.3 but emits 5,504 compiler/objtool warning lines; that
artifact is not load-approved. Separately, inspection of the staged 5.2.0
core archive found 413 ELF members; `mt-rm.o`, which
contains `rmapi_alloc`, is the one member without an IBT GNU property. Its
`rmapi_alloc` entry starts with `push %rbp`, not `ENDBR64`. The module was not
installed or loaded. An unowned module currently on this host faulted with
`Missing ENDBR: rmapi_alloc`; its origin is unverified, so we are not
attributing that failure to the 5.2.0 package. A search of the staged
official source trees found no source definition for `rmapi_alloc`; we need a
vendor-provided IBT-safe object or corresponding source to resolve this
without altering the binary.

The older official MobiMaliang 2.1.1 desktop package also recognizes the S30,
but its bundled `mtgpu_core.o_binary` has no IBT property or `ENDBR64` and
objtool reports 362 `!ENDBR` relocations. Its source fails to compile against
kernel 7.3.3. Neither package was installed or loaded. The static findings are
recorded in
`musa-data/research/official-mobi-maliang-2.1.1/legacy-core-ibt-analysis-2026-11-12.md`.

Could Moore Threads provide or clarify the following?

1. Is there an S30 driver release with an x86_64 core object built for kernel
   IBT/CET, or complete source/build instructions to produce one? Please
   identify the exact compatible driver package and its SHA-256 checksums.
2. Which kernel versions and distributions are qualified for the S30 on
   Intel x86_64? Does any released package support Fedora, or is the
   documented 4.19–5.15 range still the applicable limit?
3. We infer `mp_10` for the S30 from Moore Threads' product filing
   (S30 uses SuDi) and its MUSA architecture guide (SuDi maps to `MP_10`).
   Please confirm that this is the supported S30 code-generation target for
   the current SDK and identify any required target flags.
4. Which MUSA runtime and SDK release is supported with the S30 driver?
   The public 5.2.0 package catalog currently identifies S5000 downloads and
   does not identify S30 as a package target.
5. What supported test sequence should establish device discovery, VRAM
   allocation, host/device transfers, synchronization, and execution of a
   minimal MUSA kernel on S30?
6. Please provide matching firmware requirements, documented install/remove
   and rollback steps, and any known restrictions when Intel graphics is the
   active display adapter.

We have not installed the Ubuntu packages on Fedora, disabled kernel IBT,
modified vendor-core metadata, or run a GPU test. We can provide the isolated
build log, package versions, and PCI details on request.

## Local evidence

- [Driver and SDK availability](vendor-sdk-availability.md)
- [Live host and isolated build report](live-host-recheck-2026-11-13.md)
- [Kernel port and objtool results](driver-porting.md)
- [Official package staging and hashes](/home/axogm/Documents/project/musa-data/research/official-musa-5.2.0/)
- Build logs are under `/home/axogm/Documents/project/musa-data/research/builds/`.
