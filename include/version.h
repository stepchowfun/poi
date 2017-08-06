/*
  This header declares information about the version.
*/

#ifndef POI_VERSION_H
#define POI_VERSION_H

namespace Poi {

  // The version string
  extern const char * const VERSION;

  // nullptr if not built from a clean commit
  extern const char * const COMMIT_HASH;

  // 'release' or 'debug'
  extern const char * const BUILD_TYPE;

}

#endif
