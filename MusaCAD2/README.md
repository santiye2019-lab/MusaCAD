# MusaCAD2 — isolated next-generation workspace

This is a **new top-level folder inside the existing MusaCAD Git repository**, developed on its own branch. The legacy `main` code remains unchanged and is a read-only candidate source for reviewed transfers.

## Source admission

- Record source commit, authors, rights, dependencies and review decision before any code transfer.
- Copy existing code only when all relevant ownership and relicensing rights are verified.
- For confirmed third-party GPL code, independently reimplement functionality; do not copy or mechanically rewrite GPL source.
- Quarantine uncertain-origin code until its rights are established.
- Keep new C11 core and its tests isolated from legacy libraries; target Android, iOS and Windows.
- Do not merge into `main` during this migration unless separately authorized.

No code has been transferred yet; no GPL-free claim is made.
