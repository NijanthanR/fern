/**
 * Fern Language Version Information
 * 
 * This is the single source of truth for version numbers.
 * Uses Semantic Versioning (https://semver.org/)
 * 
 * MAJOR.MINOR.PATCH
 * - MAJOR: Incompatible API changes
 * - MINOR: Backwards-compatible functionality
 * - PATCH: Backwards-compatible bug fixes
 */

#ifndef FERN_VERSION_H
#define FERN_VERSION_H

#define FERN_VERSION_MAJOR 0
#define FERN_VERSION_MINOR 1
#define FERN_VERSION_PATCH 0

/* String version for display */
#define FERN_VERSION_STRING "0.1.0"

/* Full version with name */
#define FERN_VERSION "fern " FERN_VERSION_STRING

#endif /* FERN_VERSION_H */
