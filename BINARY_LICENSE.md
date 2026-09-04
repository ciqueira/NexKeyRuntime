# NexKeyRuntime Binary License

Last updated: September 4, 2026.

Copyright (c) 2026 Magno Ciqueira. All rights reserved.

This license applies only to official pre-compiled binary releases of
NexKeyRuntime (static libraries and headers distributed through GitHub
Releases for this repository). It is **not** the license for this
repository's own content — the header, schemas, docs, and examples here are
already Apache-2.0 (see [LICENSE](LICENSE)) and freely usable today. This
license only governs the compiled artifact a third-party plugin developer
would statically link into their own product.

This is not an end-user license for a finished plugin — it grants a
**developer redistribution right**: permission to link the binary into a
plugin you build and then ship to your own end users.

## Review history

- **Redistribution gate — decided.** Free to link under any Nexus plan,
  including the free Comunidade plan. Charging your own end users for a
  product that links the binary requires an active Comercial A or Comercial
  B subscription — see §2.
- **Attribution — decided.** Required — see §3.
- **Warranty disclaimer and liability limit — reviewed with counsel,
  September 2026.** Enforceable under Brazilian law for a B2B contract;
  carve-out added for fraud, wilful misconduct, and gross negligence,
  which Brazilian law never lets a contract exclude — see §6.
- **Support/SLA for third-party integrators — drafted below.** Follows the
  Nexus plan the integrator holds, the same tiers as the rest of the
  platform — see §8.
- **Governing law — reviewed with counsel, September 2026.** Brazil /
  Belo Horizonte, MG confirmed as the founder's domicile-based default.
  A severability clause was added given worldwide distribution — see §7.
- **Contact channel — decided.** The existing general contact address,
  hello@mcnexus.app — see §9.

## Terms

### 1. Scope

Applies only to compiled NexKeyRuntime binaries obtained through an official
GitHub Release of this repository. Source code in this repository remains
governed by `LICENSE` (Apache-2.0).

### 2. License Grant

Subject to these terms, you are granted a limited, non-exclusive,
non-transferable license to statically link the compiled binary into your
own plugin or application and distribute the resulting compiled product to
your end users.

This grant does not by itself require a commercial agreement — it is
available under the free Comunidade plan, the same as the rest of the SDK.
If the product you ship charges its own end users, that use requires an
active Comercial A or Comercial B subscription with Nexus, under the same
"free for free software, paid when you charge" boundary the rest of the
platform uses (see [mcnexus.app/pricing](https://mcnexus.app/pricing)).
This license does not by itself grant you an account, a tenant, or access
to the `sdkGateway` backend the binary talks to — those are provisioned
separately under the applicable plan.

### 3. Attribution

Your product must credit NexKeyRuntime — name and a link to
[github.com/ciqueira/NexKeyRuntime](https://github.com/ciqueira/NexKeyRuntime) —
somewhere reasonably visible to your end users: an about screen, a credits
or acknowledgments screen, or equivalent product documentation. The same
"wherever such third-party notices normally appear" standard already used
for this SDK's own Apache-2.0 dependencies applies here.

### 4. Restrictions

You may not:

- redistribute the NexKeyRuntime binary itself, standalone or repackaged,
  outside of a product you have compiled it into;
- reverse engineer, decompile, or disassemble the binary except to the
  extent applicable law expressly permits;
- remove or alter copyright or license notices embedded in the binary or
  its accompanying files;
- use the NexKeyRuntime name or trademarks to imply endorsement of your own
  product without permission.

### 5. Ownership

The binary is licensed, not sold. The copyright holder retains all rights,
title, and interest not expressly granted by this license.

### 6. Disclaimer of Warranty

THE BINARY IS PROVIDED "AS IS" AND "AS AVAILABLE", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
NON-INFRINGEMENT. Nexus does not warrant that the binary will be
error-free, uninterrupted, or free of security vulnerabilities, or that it
will meet your product's requirements.

To the maximum extent permitted by applicable law, Nexus's total liability
arising out of or related to this license is limited to the fees you paid
Nexus in the twelve months preceding the claim, and Nexus is not liable for
indirect, incidental, consequential, special, or punitive damages, or for
lost profits, revenue, or data — even if advised of the possibility.

This limit does not apply to liability arising from fraud, wilful
misconduct, or gross negligence; to death or personal injury caused by
gross negligence or wilful misconduct; or to liability that cannot be
excluded or limited under applicable consumer-protection or
data-protection law.

**Reviewed with counsel, September 2026.** Under Brazilian law, a liability
cap and exclusion of indirect damages are enforceable in a paritary
business-to-business contract (Civil Code, Art. 421-A, post *Lei da
Liberdade Econômica*) — this is not a consumer relationship. The carve-out
for fraud, wilful misconduct, and gross negligence above was added on
counsel's advice: Brazilian law does not allow a contract to exclude
liability for those regardless of what the contract says.

### 7. Governing Law; Severability

This license is governed by the laws of Brazil, with the courts of Belo
Horizonte, MG having jurisdiction — the founder's home jurisdiction, the
common default for a solo developer without a separate corporate entity.
This does not override mandatory consumer-protection law in a licensee's own
jurisdiction where applicable.

If any provision of this license is held invalid or unenforceable by a
court of competent jurisdiction — including under a licensee's local
consumer-protection or data-protection law — that provision will be
modified or disregarded to the minimum extent necessary, and the remainder
of this license remains in full force and effect.

**Reviewed with counsel, September 2026.** Because this license reaches
licensees worldwide via public GitHub distribution, counsel recommended
this severability clause specifically so that a foreign court striking down
one clause (e.g., under local consumer or data-protection law) doesn't
invalidate the rest of the agreement.

### 8. Support

Support follows the Nexus plan you hold, the same tiers as the rest of the
platform (see [mcnexus.app/pricing](https://mcnexus.app/pricing)):

- **Comunidade** (free): community support only.
- **Comercial A or Comercial B**: e-mail support, best-effort target of 2
  business days. Not a contractual SLA.

No additional support commitment attaches to the binary itself beyond what
your Nexus plan already includes.

### 9. Contact

Licensing questions: hello@mcnexus.app.
