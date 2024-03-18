#pragma once

#include "ocSimulationWorld.h"

enum class ocTrackStoreReport
{
  Success,
  File_Not_Accessible,
  File_Contains_Garbage
};

const char *to_string(ocTrackStoreReport report);

ocTrackStoreReport load_track(
  const char *filename,
  ocSimulationWorld& track);

ocTrackStoreReport save_track(
  const char *filename,
  const ocSimulationWorld& track);
