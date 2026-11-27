// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Robert Nisipeanu
'use strict';
/**
 * A fork's versions (MGU_FORK_VERSIONS=1): the version of the repository it
 * follows with a fourth number for its own releases on it, 1 to 99:
 * upstream's 0.14.19 changed here is 0.14.19.1, then 0.14.19.2. The Store
 * and the catalog take x.y.z only, so the version in the tree and the
 * catalog folds the fourth number into the third, x.y.(z*100+n): 0.14.1901,
 * 0.14.1902, and after upstream's 0.14.20 is merged in, 0.14.2001. The tag
 * and the GitHub release's title show x.y.z.n. A version the followed
 * repository released (its plain <id>@<version> tag exists) is shown and
 * tagged as it is. Unset, every version and tag is as it always was.
 */

const MAX = 99;

/* Whether this repository releases a fork's versions. */
function forkMode(value = process.env.MGU_FORK_VERSIONS || '') {
    if (value && value !== '1') {
        throw new Error(`MGU_FORK_VERSIONS=${value}: 1 for a fork's versions, or unset`);
    }
    return value === '1';
}

/* The version of the n-th own release (1 to 99) on upstream's base x.y.z. */
function forkVersion(base, n) {
    const [x, y, z] = base.split('.').map(Number);
    if (!Number.isInteger(n) || n < 1 || n > MAX) {
        throw new Error(`${base}: a fork's release ${n} on it, past the ${MAX} a version holds`);
    }
    return `${x}.${y}.${z * 100 + n}`;
}

/* The upstream base and own release number a fork version stands for, or
 * null for one with no own release in it (a third number ending in 00). */
function forkParts(version) {
    const [x, y, p] = version.split('.').map(Number);
    const n = p % 100;
    return n ? { base: `${x}.${y}.${Math.floor(p / 100)}`, n } : null;
}

/* How a release of id@version is shown and tagged: as it is when this is
 * not a fork or the followed repository released that version (has: whether
 * a tag exists), else x.y.z.n. */
function displayVersion(id, version, has, fork = forkMode()) {
    if (!fork || has(`${id}@${version}`)) {
        return version;
    }
    const f = forkParts(version);
    return f ? `${f.base}.${f.n}` : version;
}

/* The tag a release of id@version gets. */
function releaseTag(id, version, has, fork = forkMode()) {
    return `${id}@${displayVersion(id, version, has, fork)}`;
}

module.exports = { MAX, forkMode, forkVersion, forkParts, displayVersion, releaseTag };
