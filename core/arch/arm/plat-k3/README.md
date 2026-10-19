# K3 Secure Proxy handoff

Enable with `CFG_K3_SEC_PROXY_HANDOFF=y`. OP-TEE then requires a valid
transfer list containing exactly one TI consumer record for OP-TEE (consumer
ID 3); the build selects `CFG_TRANSFER_LIST=y` and `CFG_DT=y`. The record must
describe either `request` or both `request-high` and `request-low`. The current
transport uses `request` or `request-low`; it does not schedule by priority.
The consumer tag `0xfff000` is experimental and not a production upstream ID.
Place the record before `TL_TAG_FDT`; keep the FDT entry last for OP-TEE's
existing DT expansion.

The host and thread IDs must match the SYSFW secure-proxy and resource-ownership
configuration. Handoff data is configuration, not authentication: boot images
must be authenticated together by the platform's signed-image flow. The option
is unavailable on `am62lx`, which uses the mailbox transport instead.
