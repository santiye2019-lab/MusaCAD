/**
 * Pure retention planner for future MusaCAD R2 project cache.
 *
 * IMPORTANT: This module does not upload, list or delete remote files by itself.
 * The caller must authenticate MAI sessions, serialize per-device mutations,
 * verify every upload part, and durably commit catalog revisions BEFORE running
 * the returned deletion plan. Deleting a key before its catalog transition is
 * committed can lose a still-open project.
 */
export const MAX_OPEN_PROJECTS = 4;
const ID = /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/i;

function normalizedIds(values, label, max = MAX_OPEN_PROJECTS) {
  if (!Array.isArray(values) || values.length > max) throw new Error(label + ": invalid size");
  const ids = [];
  for (const value of values) {
    if (typeof value !== "string" || !ID.test(value))
      throw new Error(label + ": invalid project ID");
    const id = value.toLowerCase();
    if (ids.includes(id)) throw new Error(label + ": duplicate project ID");
    ids.push(id);
  }
  return ids;
}

/**
 * After an independently verified upload or an accepted open-tab revision,
 * retain ALL completed projects still open. Closed projects may be removed as
 * long as at least one open, verified project exists. If no opened project is
 * yet verified, keep the latest verified old project to survive failed uploads.
 * When no tabs are open, retain exactly the newest successful remote project.
 * "100%" must be produced by acknowledged bytes, not this retention planner.
 */
export function planProjectRetention({ completedProjects, openProjectIds }) {
  const open = normalizedIds(openProjectIds, "openProjectIds");
  if (!Array.isArray(completedProjects) || completedProjects.length > 100)
    throw new Error("completedProjects: invalid size");
  const found = new Set();
  const completed = completedProjects.map(value => {
    if (!value || typeof value !== "object" ||
        typeof value.id !== "string" || !ID.test(value.id) ||
        !Number.isSafeInteger(value.completedAtMs) || value.completedAtMs <= 0)
      throw new Error("completedProjects: invalid entry");
    const id = value.id.toLowerCase();
    if (found.has(id)) throw new Error("completedProjects: duplicate ID");
    found.add(id);
    return { id, completedAtMs: value.completedAtMs };
  });
  const readyOpen = completed.filter(p => open.includes(p.id));
  const retained = new Set(readyOpen.map(p => p.id));
  if (retained.size === 0 && completed.length > 0) {
    // Never delete the last complete backup before a replacement is verified.
    const latest = [...completed].sort((a,b) =>
      b.completedAtMs - a.completedAtMs || a.id.localeCompare(b.id))[0];
    retained.add(latest.id);
  }
  return {
    keepProjectIds: completed.filter(p => retained.has(p.id)).map(p => p.id),
    deleteProjectIds: completed.filter(p => !retained.has(p.id)).map(p => p.id),
    openProjectIds: open,
    needsPendingUpload: open.some(id => !found.has(id))
  };
}

/** Ignore late, out-of-order open-tab snapshots and support exact retries. */
export function compareManifestRevision(lastRevision, proposedRevision) {
  if (!Number.isSafeInteger(lastRevision) || lastRevision < 0 ||
      !Number.isSafeInteger(proposedRevision) || proposedRevision < 1)
    throw new Error("invalid revision");
  if (proposedRevision < lastRevision) return "stale";
  if (proposedRevision === lastRevision) return "idempotent";
  return "advance";
}
