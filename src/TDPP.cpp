#include "TDPP.hpp"

TDPP::TDPP() : TDigitizer() {}

CAEN_DGTZ_ErrorCode TDPP::StartAcquisition()
{
  AllocateMemory();

  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStartAcquisition(fHandler);
  PrintError(err, "StartAcquisition");

  for (auto &t : fTime) t = 0;
  for (auto &t : fTimeOffset) t = 0;
  for (auto &t : fPreviousTime) t = 0;

  return err;
}

void TDPP::StopAcquisition()
{
  CAEN_DGTZ_ErrorCode err;
  err = CAEN_DGTZ_SWStopAcquisition(fHandler);
  PrintError(err, "StopAcquisition");

  FreeMemory();
  // err = CAEN_DGTZ_FreeEvent(fHandler, (void **)&fpEventStd);
  // PrintError(err, "FreeEvent");
}
