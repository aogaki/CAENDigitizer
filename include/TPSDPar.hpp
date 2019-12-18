#ifndef TPSDPar_hpp
#define TPSDPar_hpp 1

#include <CAENDigitizer.h>
#include <CAENDigitizerType.h>

constexpr auto kChSize = MAX_DPP_PSD_CHANNEL_SIZE;

class TPSDPar
{
 public:
  TPSDPar()
  {
    SetBLTEvents(512);
    SetIOLevel(CAEN_DGTZ_IOLevel_NIM);
    SetSyncMode(CAEN_DGTZ_RUN_SYNC_Disabled);
    SetRecordLength(512);
    SetPreTriggerSize(100);
    SetPolarity(CAEN_DGTZ_PulsePolarityNegative);
    SetDynamicRange(0);  // 0 = 2Vpp (default), 1 = 0.5 Vpp
    SetDCOffset(0.8);

    SetTriggerHoldOff(8);
    SetPileUpRejGap(10);
    SetPileUpOption(CAEN_DGTZ_DPP_PSD_PUR_DetectOnly);
    SetAllChThreshold(100);
    SetAllChSelfTrigger(int(true));
    SetAllChChargeSensitivity(0);
    SetAllChShortGate(24);
    SetAllChLongGate(100);
    SetAllChGateOffset(8);
    SetAllChTrgValAccWindow(200);
    SetAllChBaselineSamples(2);
    SetAllChDiscrMode(CAEN_DGTZ_DPP_DISCR_MODE_CFD);
    SetAllChCFDFraction(2);
    SetAllChCFDDelay(10);

    SetDPPTriggerMode(CAEN_DGTZ_DPP_TriggerMode_Normal);
    SetDPPTriggerConfig(CAEN_DGTZ_DPP_TriggerConfig_Threshold);
    SetTriggerMode(CAEN_DGTZ_TRGMODE_ACQ_AND_EXTOUT);
    SetChMask(0b11111111);
  };
  ~TPSDPar(){};

  void SetBLTEvents(uint32_t val) { fBLTEvents = val; };
  uint32_t GetBLTEvents() { return fBLTEvents; };

  void SetIOLevel(CAEN_DGTZ_IOLevel_t val) { fIOLevel = val; };
  CAEN_DGTZ_IOLevel_t GetIOLevel() { return fIOLevel; };

  void SetSyncMode(CAEN_DGTZ_RunSyncMode_t val) { fSyncMode = val; };
  CAEN_DGTZ_RunSyncMode_t GetSyncMode() { return fSyncMode; };

  void SetRecordLength(uint32_t val) { fRecordLength = val; };
  uint32_t GetRecordLength() { return fRecordLength; };

  void SetPreTriggerSize(uint32_t val) { fPreTriggerSize = val; };
  uint32_t GetPreTriggerSize() { return fPreTriggerSize; };

  void SetPolarity(CAEN_DGTZ_PulsePolarity_t val) { fPolarity = val; };
  CAEN_DGTZ_PulsePolarity_t GetPolarity() { return fPolarity; };

  void SetDynamicRange(uint16_t val) { fDynamicRange = val; };
  uint16_t GetDynamicRange() { return fDynamicRange; };

  void SetDCOffset(double val) { fDCOffset = val; };
  double GetDCOffset() { return fDCOffset; };

  void SetTriggerHoldOff(int val) { fTriggerHoldOff = val; };
  int GetTriggerHoldOff() { return fTriggerHoldOff; };

  void SetPileUpRejGap(int val) { fPileUpRejGap = val; };
  int GetPileUpRejGap() { return fPileUpRejGap; };

  void SetPileUpOption(CAEN_DGTZ_DPP_PUR_t val) { fPileUpOption = val; };
  CAEN_DGTZ_DPP_PUR_t GetPileUpOption() { return fPileUpOption; };

  void SetThreshold(int val, uint ch) { fThreshold[ch] = val; };
  void SetAllChThreshold(int val)
  {
    for (uint i = 0; i < kChSize; i++) fThreshold[i] = val;
  };
  int GetThreshold(uint ch) { return fThreshold[ch]; };

  void SetSelfTrigger(int val, uint ch) { fSelfTrigger[ch] = val; };
  void SetAllChSelfTrigger(int val)
  {
    for (uint i = 0; i < kChSize; i++) fSelfTrigger[i] = val;
  };
  int GetSelfTrigger(uint ch) { return fSelfTrigger[ch]; };

  void SetChargeSensitivity(int val, uint ch) { fChargeSensitivity[ch] = val; };
  void SetAllChChargeSensitivity(int val)
  {
    for (uint i = 0; i < kChSize; i++) fChargeSensitivity[i] = val;
  };
  int GetChargeSensitivity(uint ch) { return fChargeSensitivity[ch]; };

  void SetShortGate(int val, uint ch) { fShortGate[ch] = val; };
  void SetAllChShortGate(int val)
  {
    for (uint i = 0; i < kChSize; i++) fShortGate[i] = val;
  };
  int GetShortGate(uint ch) { return fShortGate[ch]; };

  void SetLongGate(int val, uint ch) { fLongGate[ch] = val; };
  void SetAllChLongGate(int val)
  {
    for (uint i = 0; i < kChSize; i++) fLongGate[i] = val;
  };
  int GetLongGate(uint ch) { return fLongGate[ch]; };

  void SetGateOffset(int val, uint ch) { fGateOffset[ch] = val; };
  void SetAllChGateOffset(int val)
  {
    for (uint i = 0; i < kChSize; i++) fGateOffset[i] = val;
  };
  int GetGateOffset(uint ch) { return fGateOffset[ch]; };

  void SetTrgValAccWindow(int val, uint ch) { fTrgValAccWindow[ch] = val; };
  void SetAllChTrgValAccWindow(int val)
  {
    for (uint i = 0; i < kChSize; i++) fTrgValAccWindow[i] = val;
  };
  int GetTrgValAccWindow(uint ch) { return fTrgValAccWindow[ch]; };

  void SetBaselineSamples(int val, uint ch) { fBaselineSamples[ch] = val; };
  void SetAllChBaselineSamples(int val)
  {
    for (uint i = 0; i < kChSize; i++) fBaselineSamples[i] = val;
  };
  int GetBaselineSamples(uint ch) { return fBaselineSamples[ch]; };

  void SetDiscrMode(int val, uint ch) { fDiscrMode[ch] = val; };
  void SetAllChDiscrMode(int val)
  {
    for (uint i = 0; i < kChSize; i++) fDiscrMode[i] = val;
  };
  int GetDiscrMode(uint ch) { return fDiscrMode[ch]; };

  void SetCFDFraction(int val, uint ch) { fCFDFraction[ch] = val; };
  void SetAllChCFDFraction(int val)
  {
    for (uint i = 0; i < kChSize; i++) fCFDFraction[i] = val;
  };
  int GetCFDFraction(uint ch) { return fCFDFraction[ch]; };

  void SetCFDDelay(int val, uint ch) { fCFDDelay[ch] = val; };
  void SetAllChCFDDelay(int val)
  {
    for (uint i = 0; i < kChSize; i++) fCFDDelay[i] = val;
  };
  int GetCFDDelay(uint ch) { return fCFDDelay[ch]; };

  void SetDPPTriggerMode(CAEN_DGTZ_DPP_TriggerMode_t val)
  {
    fDPPTriggerMode = val;
  };
  CAEN_DGTZ_DPP_TriggerMode_t GetDPPTriggerMode() { return fDPPTriggerMode; };

  void SetDPPTriggerConfig(CAEN_DGTZ_DPP_TriggerConfig_t val)
  {
    fDPPTriggerConfig = val;
  };
  CAEN_DGTZ_DPP_TriggerConfig_t GetDPPTriggerConfig()
  {
    return fDPPTriggerConfig;
  };

  void SetTriggerMode(CAEN_DGTZ_TriggerMode_t val) { fTriggerMode = val; };
  CAEN_DGTZ_TriggerMode_t GetTriggerMode() { return fTriggerMode; };

  void SetChMask(uint16_t val) { fChMask = val; };
  uint16_t GetChMask() { return fChMask; };

 private:
  // For common parameters
  // Some of these should be included in the base class.  But not yet now
  uint32_t fBLTEvents;
  CAEN_DGTZ_IOLevel_t fIOLevel;
  CAEN_DGTZ_RunSyncMode_t fSyncMode;
  uint32_t fRecordLength;
  uint32_t fPreTriggerSize;
  CAEN_DGTZ_PulsePolarity_t fPolarity;
  uint16_t fDynamicRange;
  double fDCOffset;
  uint16_t fChMask;

  // For DPP trigger parameters
  CAEN_DGTZ_DPP_TriggerMode_t fDPPTriggerMode;
  CAEN_DGTZ_DPP_TriggerConfig_t fDPPTriggerConfig;
  CAEN_DGTZ_TriggerMode_t fTriggerMode;

  // For CAEN_DGTZ_DPP_PSD_Params_t;
  // I do not like the notation of it
  // This is the only reason I make following parameters
  // Using CAEN_DGTZ_DPP_PSD_Params_t is simple. probably.
  // Too many coments.  Sucks!

  int fTriggerHoldOff;  // trgho; // Samples
  /* Pile-Up rejection Mode
  CAEN_DGTZ_DPP_PSD_PUR_DetectOnly -> Only Detect Pile-Up
  CAEN_DGTZ_DPP_PSD_PUR_Enabled -> Reject Pile-Up */
  int fPileUpRejGap;                  // purgap;  // Ignored for x751, LSB
  CAEN_DGTZ_DPP_PUR_t fPileUpOption;  // purh;
  // Purity Gap in LSB units (1LSB = 0.12 mV for 2Vpp
  // Input Range, 1LSB = 0.03 mV for 0.5 Vpp Input Range )
  int fThreshold[kChSize];    // thr[kChSize]; // LSB
  int fSelfTrigger[kChSize];  // selft[kChSize];
  /* Charge sensibility: These values are from digiTes.
                         Probably for x730.  Other model is different
                         But taking 0 to 5
              Options for Input Range 2Vpp: 0->5fC/LSB; 1->20fC/LSB;
     2->80fC/LSB; 3->320fC/LSB; 4->1.28pC/LSB; 5->5.12pC/LSB Options for
     Input Range 0.5Vpp: 0->1.25fC/LSB; 1->5fC/LSB; 2->20fC/LSB;
     3->80fC/LSB; 4->320fC/LSB; 5->1.28pC/LSB */
  int fChargeSensitivity[kChSize];  // csens[kChSize];
  int fShortGate[kChSize];          // sgate[kChSize]; // Samples
  int fLongGate[kChSize];           //lgate[kChSize]; // Samples
  int fGateOffset[kChSize];         // pgate[kChSize]; // Samples
  int fTrgValAccWindow
      [kChSize];  // tvaw[kChSize];// Samples.  For coincidence mode
  /* The following parameter is used to specifiy the number of samples for
  the baseline averaging: 0 -> absolute Bl 1 -> 16samp 2 -> 64samp 3 ->
  256samp 4 -> 1024samp */
  int fBaselineSamples[kChSize];  // nsbl[kChSize]; // Samples
  /*Discrimination mode for the event selection
  CAEN_DGTZ_DPP_DISCR_MODE_LED -> Leading Edge Distrimination
  CAEN_DGTZ_DPP_DISCR_MODE_CFD -> Constant Fraction Distrimination*/
  int fDiscrMode[kChSize];    // discr[kChSize]; //only for FW > 132.32
  int fCFDFraction[kChSize];  // cfdf[kChSize]; //only for FW > 132.32
  int fCFDDelay[kChSize];     // cfdd[kChSize]; //only for FW > 132.32
};

#endif
